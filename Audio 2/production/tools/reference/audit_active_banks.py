#!/usr/bin/env python3
"""Measure active global-ROM banking against a start-latched counterfactual.

The original full-board PCM must remain bit-identical. Extra decoder histories
are observation only; their mixed output is never labelled native or arcade.
"""
import argparse
import json
from pathlib import Path
import subprocess
from reproduce_full_board_references import HERE,PACKAGE,sha

def replace_once(text,old,new):
    if text.count(old)!=1:raise ValueError('Audited source shape changed: '+old[:80])
    return text.replace(old,new)

def instrument_header(text):
    text=replace_once(text,'unsigned volume,playing;','unsigned volume,playing,audit_bank; int audit_signal,audit_index,audit_current;')
    text=replace_once(text,'static const int reference_oki_steps',
      'static uint64_t audit_changed_byte_reads[4],audit_changed_nibbles[4],audit_changed_pcm[4],audit_bank_mismatch_reads[4];\n'
      'static uint32_t audit_first_changed_address[4];\nstatic const int reference_oki_steps')
    text=replace_once(text,'.volume=value&15,.playing=1}', '.volume=value&15,.playing=1,.audit_bank=o->bank}')
    text=replace_once(text,'o->voices[v].playing=0;o->voices[v].current=0;',
                     'o->voices[v].playing=0;o->voices[v].current=0;o->voices[v].audit_current=0;')
    text=replace_once(text,'if(!p->playing){p->current=0;continue;}',
                     'if(!p->playing){p->current=0;p->audit_current=0;continue;}')
    extra='''
        unsigned relative=(p->start+p->position/2)&0x3ffff;
        unsigned fixed_byte=o->rom[p->audit_bank*0x40000+relative];
        unsigned fixed_n=(p->position&1)?fixed_byte&15:fixed_byte>>4;
        if(o->bank!=p->audit_bank)audit_bank_mismatch_reads[v]++;
        if(byte!=fixed_byte){if(!audit_changed_byte_reads[v])audit_first_changed_address[v]=relative;audit_changed_byte_reads[v]++;}
        if(n!=fixed_n)audit_changed_nibbles[v]++;
        int fixed_step=reference_oki_steps[p->audit_index];
        int fixed_delta=fixed_step/8+((fixed_n&1)?fixed_step/4:0)+((fixed_n&2)?fixed_step/2:0)+((fixed_n&4)?fixed_step:0);
        p->audit_signal+=(fixed_n&8)?-fixed_delta:fixed_delta;
        if(p->audit_signal>2047)p->audit_signal=2047;
        if(p->audit_signal< -2048)p->audit_signal=-2048;
        p->audit_index+=shift[fixed_n&7];if(p->audit_index<0)p->audit_index=0;if(p->audit_index>48)p->audit_index=48;
        p->audit_current=p->audit_signal*16;
        if(p->current!=p->audit_current)audit_changed_pcm[v]++;
'''
    text=replace_once(text,'p->current=p->signal*16;if(++p->position>=p->count)p->playing=0;',
                     'p->current=p->signal*16;'+extra+'if(++p->position>=p->count)p->playing=0;')
    text=replace_once(text,'#endif',
      'static int audit_latched_mix(const reference_oki_t *o)\n'
      '{int sum=0;for(unsigned v=0;v<4;v++)sum+=o->voices[v].audit_current*reference_oki_levels[o->voices[v].volume]/32;return sum;}\n#endif')
    return text

def instrument_capture(text):
    text=replace_once(text,'uint64_t clipped,peak,last_oki_status;',
                     'uint64_t clipped,peak,last_oki_status; uint64_t audit_mix_changes,audit_mix_squared_error,audit_mix_peak_difference,audit_first_mix_frame,audit_last_mix_frame;')
    extra='''
                int64_t fixed=((int64_t)b->output[0]*10813+(int64_t)b->output[1]*10813+
                    (int64_t)audit_latched_mix(&b->oki)*21627)>>15;
                if(fixed>32767)fixed=32767;if(fixed< -32768)fixed=-32768;
                int64_t difference=mixed-fixed;
                if(difference){if(!b->audit_mix_changes)b->audit_first_mix_frame=b->frames;b->audit_last_mix_frame=b->frames;b->audit_mix_changes++;b->audit_mix_squared_error+=(uint64_t)(difference*difference);}
                uint64_t absolute=(uint64_t)(difference<0?-difference:difference);
                if(absolute>b->audit_mix_peak_difference)b->audit_mix_peak_difference=absolute;
'''
    text=replace_once(text,'uint64_t magnitude=(uint64_t)(mixed<0?-mixed:mixed);',extra+'uint64_t magnitude=(uint64_t)(mixed<0?-mixed:mixed);')
    extra='''
    char audit_path[4096];snprintf(audit_path,sizeof audit_path,"%s.bank-audit.json",argv[7]);
    FILE *audit=fopen(audit_path,"w");if(!audit)return 11;
    fprintf(audit,"{\\\"mixed_frames_different\\\":%llu,\\\"mixed_squared_error_pcm_units\\\":%llu,\\\"mixed_peak_difference_pcm_units\\\":%llu,\\\"first_different_output_frame\\\":%llu,\\\"last_different_output_frame\\\":%llu,\\\"voices\\\":[",
      (unsigned long long)b->audit_mix_changes,(unsigned long long)b->audit_mix_squared_error,(unsigned long long)b->audit_mix_peak_difference,(unsigned long long)b->audit_first_mix_frame,(unsigned long long)b->audit_last_mix_frame);
    for(unsigned v=0;v<4;v++)fprintf(audit,"%s{\\\"voice\\\":%u,\\\"bank_mismatch_nibble_reads\\\":%llu,\\\"changed_byte_reads\\\":%llu,\\\"changed_nibbles\\\":%llu,\\\"decoded_pcm_samples_different\\\":%llu,\\\"first_changed_relative_address\\\":%u}",v?",":"",v,
      (unsigned long long)audit_bank_mismatch_reads[v],(unsigned long long)audit_changed_byte_reads[v],(unsigned long long)audit_changed_nibbles[v],(unsigned long long)audit_changed_pcm[v],audit_first_changed_address[v]);
    fprintf(audit,"]}\\n");fclose(audit);
'''
    text=replace_once(text,'free(samples);free(b);return valid?0:7;',extra+'free(samples);free(b);return valid?0:7;')
    return text

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--model-run',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='cc');a=p.parse_args()
    a.model_run=a.model_run.resolve();a.out=a.out.resolve()
    if a.out==PACKAGE or PACKAGE in a.out.parents:p.error('Private audit output must stay outside source package')
    original=json.loads((a.model_run/'mix-reference-report.json').read_text());capture=original['reference_capture']
    for key,value in capture['output_sha256'].items():
        if sha(capture[key])!=value:raise ValueError('Original full-board capture identity mismatch')
    a.out.mkdir(parents=True,exist_ok=False)
    header=HERE/'reference_oki.h';source=HERE/'capture_board_replay.c'
    before={str(p):sha(p) for p in [header,source,Path(__file__).resolve()]}
    (a.out/'reference_oki.h').write_text(instrument_header(header.read_text()))
    (a.out/'capture_board_replay.c').write_text(instrument_capture(source.read_text()))
    core=PACKAGE/'src/reference_driver';deps=a.model_run/'deps';exe=a.out/'capture_bank_audit'
    build=[a.cc,'-O3','-std=c99','-I',str(HERE),'-I',str(core),'-I',str(deps),
        str(a.out/'capture_board_replay.c'),str(core/'z80.c'),str(deps/'opm.c'),'-lm','-o',str(exe)]
    subprocess.run(build,check=True,capture_output=True,text=True)
    command=list(capture['commands'][0]);command[0]=str(exe)
    for index,name in [(5,'trace.csv'),(6,'audio.pcm'),(7,'capture.json'),(8,'deliveries.csv')]:command[index]=str(a.out/name)
    subprocess.run(command,check=True,capture_output=True,text=True)
    result=json.loads((a.out/'capture.json.bank-audit.json').read_text())
    result.update(kind='ACTIVE_GLOBAL_BANK_VS_START_LATCHED_COUNTERFACTUAL',
      original_mixed_pcm_unchanged=sha(a.out/'audio.pcm')==capture['output_sha256']['pcm'],
      original_trace_unchanged=sha(a.out/'trace.csv')==capture['output_sha256']['trace'],
      original_reference_report_sha256=sha(a.model_run/'mix-reference-report.json'),
      source_sha256=before,source_stable=all(sha(Path(p))==v for p,v in before.items()),
      instrumented_source_sha256={name:sha(a.out/name) for name in ['reference_oki.h','capture_board_replay.c']},
      build_command=build,run_command=command,
      scope='Only the ROM bank-selection policy differs in parallel decoder histories; no native/DSP/analog inference is implied.')
    (a.out/'bank-audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
    return 0 if result['original_mixed_pcm_unchanged'] and result['original_trace_unchanged'] and result['source_stable'] else 1

if __name__=='__main__':raise SystemExit(main())
