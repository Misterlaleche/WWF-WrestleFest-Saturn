#!/usr/bin/env python3
"""Check the actual promoted SDK sorter against a frozen original and stable order.

No shortcut is inserted into the source under test. Both extracted bodies are
compiled with ASan/UBSan; equal keys retain submission order. Histogram scratch
contents are not an API, but all four arrays retain their boundary guards.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REFERENCE_BODY_SHA256 = 'ca98242296dd3ef737cb083182449ea4e879a75d7b62bc2956242ab5a5a0d987'
HEADER = r'''#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define JO_VDP1_SORT_IN_C 1
#define JO_VDP1_RADIX 256
#define JO_ZERO(x) ((x)=0)
#define JO_DIV_BY_256(x) ((x)>>8)
#define jo_memset memset
#define MAX 4096
struct guarded { uint32_t pre; unsigned short data[MAX]; uint32_t post; };
static struct guarded guard_low,guard_high,sort_a,sort_b;
#define __jo_vdp1_radix_low guard_low.data
#define __jo_vdp1_radix_high guard_high.data
#define __jo_vdp1_sort_a sort_a.data
#define __jo_vdp1_sort_b sort_b.data
/* Actual histogram extent is256; extraction below preserves that bound. */
typedef struct {unsigned count;const unsigned short *keys;} jo_vdp1_frame;
'''
DRIVER = r'''
static unsigned short keys[MAX],saved[MAX],expected[MAX];
static uint32_t rng=0x52871349u;static unsigned cases;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static int compare(const void *x,const void *y){unsigned a=*(const unsigned short*)x,b=*(const unsigned short*)y;return keys[a]!=keys[b]?(keys[a]>keys[b]?-1:1):(a>b)-(a<b);}
static void reset(void){struct guarded *all[]={&guard_low,&guard_high,&sort_a,&sort_b};for(unsigned j=0;j<4;j++){all[j]->pre=0xdeadbeefu;all[j]->post=0x31415926u;memset(all[j]->data,0xa5,sizeof all[j]->data);}}
static void guards(void){struct guarded *all[]={&guard_low,&guard_high,&sort_a,&sort_b};for(unsigned j=0;j<4;j++){assert(all[j]->pre==0xdeadbeefu);assert(all[j]->post==0x31415926u);}}
static void run(unsigned n,unsigned kind){
 unsigned uniform=random32()&65535u;for(unsigned i=0;i<n;i++){
  unsigned r=random32();keys[i]=(unsigned short)((kind==0||kind==7)?uniform:kind==1?200u:kind==2?(r&255u):kind==3?((r&255u)<<8):kind==4?(i/3u):kind==5?(n-i)/3u:r);
  expected[i]=(unsigned short)i;
 }
 if(kind==7&&n)keys[n-1]^=1u;
 qsort(expected,n,sizeof expected[0],compare);jo_vdp1_frame frame={n,keys};reset();unsigned short *a=baseline(&frame);memcpy(saved,a,n*sizeof *a);guards();assert(!memcmp(saved,expected,n*sizeof *a));
 reset();a=candidate(&frame);guards();assert(!memcmp(a,saved,n*sizeof *a));++cases;
}
int main(void){for(unsigned n=0;n<=512;n++)for(unsigned k=0;k<8;k++)run(n,k);for(unsigned i=0;i<1600;i++)run(random32()%(MAX+1u),i%8u);for(unsigned k=0;k<8;k++)run(MAX,k);printf("PASS %u cases: exact stable order, all-equal/mixed/empty/max counts and guards\n",cases);}
'''


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extract(text):
    marker = 'static unsigned short                   *jo_vdp1_radix_sort('
    assert text.count(marker) == 1, 'Expected exactly one SDK sorter'
    start = text.index(marker)
    right = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[right] == '{') - (text[right] == '}')
        right += 1
    return text[start:right]


def rename(body, name):
    return body.replace('jo_vdp1_radix_sort', name).replace(
        'sizeof(__jo_vdp1_radix_low)', '256u*sizeof(unsigned short)').replace(
        'sizeof(__jo_vdp1_radix_high)', '256u*sizeof(unsigned short)')


def check_bodies(reference, actual):
    assert hashlib.sha256(reference.encode()).hexdigest() == REFERENCE_BODY_SHA256, 'Frozen reference drift'
    assert actual != reference, 'Production sorter must differ from original reference'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT/'sdk/jo_engine/vdp1_command_pipeline.c')
    parser.add_argument('--reference', type=Path, default=ROOT/'tests/fixtures/sdk/vdp1_radix_sort_reference.c')
    parser.add_argument('--output', type=Path, default=ROOT/'build/sdk-uniform-sort')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = args.source.resolve()
    reference_path = args.reference.resolve()
    identity = sha(source)
    reference_identity = sha(reference_path)
    actual = extract(source.read_text())
    reference = extract(reference_path.read_text())
    check_bodies(reference, actual)
    commands = []

    def run(name, candidate, must_pass):
        unit = out/(name+'.c')
        unit.write_text(HEADER+rename(reference, 'baseline')+'\n'+rename(candidate, 'candidate')+'\n'+DRIVER)
        binary = out/name
        command = [os.environ.get('CC', 'cc'), '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-fsanitize=address,undefined', '-fno-pie', '-no-pie', str(unit), '-o', str(binary)]
        subprocess.run(command, check=True)
        result = subprocess.run([str(binary)], env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1'), capture_output=True, text=True)
        (out/(name+'.log')).write_text(result.stdout+result.stderr)
        if must_pass:
            result.check_returncode()
        else:
            assert result.returncode != 0, 'Semantic mutant unexpectedly passed: '+name
            assert 'Assertion' in result.stderr or 'assertion' in result.stderr, 'Mutant must fail a semantic assertion'
        commands.append(dict(name=name, command=command, returncode=result.returncode, expected_pass=must_pass, result=result.stdout+result.stderr))
        return result.stdout

    output = run('actual-sort', actual, True)
    # These mutants must affect the actual selected body, never a synthesized copy.
    key = 'keys[i] == first_key'
    index = '__jo_vdp1_sort_a[i] = (unsigned short)i;'
    assert actual.count(key) == 1 and actual.count(index) == 1
    run('negative-partial-key', actual.replace(key, '(keys[i] & 255u) == (first_key & 255u)'), False)
    run('negative-unstable-equal', actual.replace(index, '__jo_vdp1_sort_a[i] = (unsigned short)(n-1u-i);'), False)
    rejected_original = False
    try:
        check_bodies(reference, reference)
    except AssertionError:
        rejected_original = True
    assert rejected_original, 'Wrong-source guard must reject the original body as production'
    assert sha(source) == identity and sha(reference_path) == reference_identity
    report = dict(status='PASS', source=str(source), source_sha256=identity,
                  actual_body_sha256=hashlib.sha256(actual.encode()).hexdigest(),
                  reference_source=str(reference_path), reference_sha256=reference_identity,
                  reference_body_sha256=REFERENCE_BODY_SHA256, actual_and_reference_differ=True,
                  source_not_transformed=True, wrong_source_negative_rejected=rejected_original,
                  test_sha256=sha(Path(__file__)), commands=commands,
                  limitations=['Extracted C-sort body, not SH-2 target timing',
                               'Histogram scratch contents may differ; result order and guards must match'])
    (out/'summary.json').write_text(json.dumps(report, indent=2)+'\n')
    print(output, end='')
    print('PASS two semantic mutants and original-as-production negative')


if __name__ == '__main__':
    main()
