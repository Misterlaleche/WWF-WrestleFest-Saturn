"""Retired partial-audio image builder. Complete catalogue is required first."""
from pathlib import Path
import argparse,hashlib,json,os,re
from cold_overlay import Elf,Overlay,file_sha

def build(a):
    raise RuntimeError('Diagnostic image generation is retired: the user requires all18 music tracks and all effects integrated and verified before any new BIN/CUE. A complete release gate is still pending; no output was created.')


def _unreviewed_legacy_diagnostic(a):
    workspace=Path(__file__).resolve().parents[5]
    out=a.out.resolve();allowed=(workspace/'runtime18/entregas').resolve()
    assert out.is_relative_to(allowed) and 'R04' not in out.name.upper(),out
    assert re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{1,100}',a.name)
    out.mkdir(parents=True,exist_ok=True)
    binpath=out/(a.name+'.bin');cuepath=out/(a.name+'.cue');temporary=out/(a.name+'.bin.partial')
    assert not any(p.exists()for p in (binpath,cuepath,temporary)), 'Refusing to overwrite a trial image'
    validation=json.loads(a.validation.read_text())
    rows=validation['observations']
    assert validation.get('load_success') and validation.get('existing_media_unchanged')
    assert max(r.get('game_frame',0)for r in rows)>=600,'Combat not reached in validation'
    combat=[r for r in rows if r.get('game_frame',0)>=300 and r.get('native_profile')==0x4d46]
    assert combat and any(r.get('fm_active_mask',0)and r.get('audio_rms',0)>0 for r in combat),'No observed in-game FM music'
    assert all(r.get('music_state')==2 and r.get('music_error')==0 and r.get('mcu_state')==2 and not r.get('audio_diagnostics',{}).get('disabled',True)for r in combat)
    assert not any(r.get('fatal_owner',0)for r in rows),'Guest fatal during validation'
    assert all(not r['audio_diagnostics'].get(key,0)for r in combat for key in ('reason','dropped_sfx','dropped_music','ignored'))
    memory=[r['allocator_live_stats']for r in rows if r.get('allocator_live_stats',{}).get('captures',0)>0]
    assert memory and all(not r['faults']and r['private_free']>=4096 and r['private_largest_free']>=4096 for r in memory),'Private heap gate not observed'
    assert file_sha(a.runtime)==validation['elf']['sha256'],'ELF symbols/metadata were not validated'
    assert len(re.findall(r'(?mi)^\s*TRACK\s',a.cue.read_text()))==1,'Only the validated single data track is supported'
    runtime=Elf(a.runtime);binary=runtime.binary()
    replacements={'A.BIN':binary,'MUSIC.ELF':a.music.read_bytes(),'SCSP.ELF':a.scsp.read_bytes()}
    overlay=Overlay(a.cue,replacements);boot=overlay.patch_boot(runtime,binary);overlay.verify()
    assert boot==validation['boot_header_changes'],'Unvalidated boot header/stack changes'
    expected={x['name']:x['new_sha256']for x in validation['replacements']}
    assert all(hashlib.sha256(data).hexdigest()==expected[name]for name,data in replacements.items()),'Unvalidated component'
    assert overlay.original_sha==validation['existing_media_sha256'],'Different source disc'
    streamed=hashlib.sha256()
    with temporary.open('xb')as output:
        for pos in range(0,overlay.size,1024*1024):
            chunk=overlay.read(pos,min(1024*1024,overlay.size-pos));output.write(chunk);streamed.update(chunk)
        output.flush();os.fsync(output.fileno())
    assert temporary.stat().st_size==overlay.size and file_sha(temporary)==streamed.hexdigest()
    assert file_sha(overlay.path)==overlay.original_sha,'Source media changed'
    temporary.rename(binpath)
    cuepath.write_text(f'FILE "{binpath.name}" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n',encoding='ascii',newline='\r\n')
    reopened=Overlay(cuepath,{})
    assert reopened.manifest()==overlay.manifest(),'Reopened filesystem differs'
    assert reopened.payload(0)==overlay.payload(0),'Reopened IPL differs'
    for lba,expected_sector in overlay.sectors.items():
        actual=reopened.raw(lba)
        assert actual==expected_sector and actual==reopened.sector(actual[16:2064],lba)
    result=dict(status='DIAGNOSTIC_NATIVE_AUDIO_TEST_NOT_FINAL',user_authorized_bin_cue=True,R04_modified=False,
        physical_audio_certified=False,source_media=str(overlay.path),source_media_sha256=overlay.original_sha,
        bin=str(binpath),bin_bytes=overlay.size,bin_sha256=file_sha(binpath),cue=str(cuepath),cue_sha256=file_sha(cuepath),
        validation_report=str(a.validation.resolve()),known_fm_drops=max(r.get('dropped_fm',0)for r in rows),
        runtime_elf_sha256=file_sha(a.runtime),music_elf_sha256=file_sha(a.music),scsp_elf_sha256=file_sha(a.scsp),
        replacements=overlay.replacements,boot_header_changes=boot,all_changed_sectors_ecc_verified=True,
        source_media_unchanged=True,image_reopened_and_component_hashes_verified=True,
        generated_media_software_boot_test='PENDING_DO_NOT_PACKAGE_YET',first_data_track='MODE1/2352',CDDA_tracks=0)
    (out/'MANIFEST.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('cue','runtime','music','scsp','out','validation'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--name',required=True);build(p.parse_args())
