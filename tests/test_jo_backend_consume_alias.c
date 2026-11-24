/* A byte pointer may inspect the changing backend itself. The optimized
 * bulk path must retain the scalar observation order for such callers. */
#define main existing_consume_main
#include "test_jo_backend_consume.c"
#undef main
#include <stddef.h>
#ifndef WF_JO_TEST_RESET_CACHE
#error "Use test_jo_backend_counters.py to supply an identical initial cache"
#endif

static void aliased_run(result_t *out,unsigned bulk,size_t offset,unsigned alias_sources)
{
    memset(out,0,sizeof *out);memset(stage,0xa5,sizeof stage);
    memset(reads,0,sizeof reads);read_count=action=triggered=trigger_min_slot=0;epoch=1;
    wf_jo_test_reset_texture_cache();
    wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);
    assert(ops->begin_frame(&backend,48,0,0)==WF_OK);
    for(unsigned i=0;i<48;i++){sources[i]=(uint16_t)(1000u+i);touched[i]=1;}
    assert(wf_jo_backend_protect_rom_tiles(&backend,sources,touched,1,48)==WF_OK);
    assert(scalar(48)==WF_OK&&ops->end_frame(&backend)==WF_OK);
    assert(ops->begin_frame(&backend,48,0,0)==WF_OK);
    assert(wf_jo_backend_protect_rom_tiles(&backend,sources,touched,1,48)==WF_OK);
    backend.texture_reused=UINT32_MAX-19u;
    const uint8_t *active=(const uint8_t *)&backend+offset;
    const uint16_t *ids=alias_sources?backend.rom_source:sources;
    memset(reads,0,sizeof reads);read_count=0;
    if(bulk)out->consume=wf_jo_backend_consume_rom_tiles(&backend,ids,active,1,48,read_tile);
    else{
        out->consume=WF_OK;
        for(unsigned slot=0;slot<48;slot++){
            wf_status_t status=active[slot]==1?
                wf_saturn_yaul_upload_rom_tile(&backend,(uint16_t)slot,ids[slot],read_tile):
                wf_jo_backend_skip_tile(&backend,(uint16_t)slot);
            if(status!=WF_OK){out->consume=status;break;}
        }
    }
    out->read_count=read_count;out->emitted=backend.emitted_tiles;
    out->expected=backend.expected_tiles;out->active=backend.active;
    out->bytes=backend.texture_bytes;out->transfers=backend.texture_transfers;
    out->reused=backend.texture_reused;
    memcpy(out->reads,reads,sizeof reads);memcpy(out->sources,backend.rom_source,sizeof out->sources);
    memcpy(out->dirty,backend.dirty,sizeof out->dirty);memcpy(out->resident,backend.resident,sizeof out->resident);
    memcpy(out->identity,backend.rom_identity,sizeof out->identity);memcpy(out->meta,backend.crop_meta,sizeof out->meta);
    memcpy(out->height,backend.crop_height,sizeof out->height);memcpy(out->stage,stage,sizeof stage);
    if(out->consume==WF_OK)out->end=ops->end_frame(&backend);
    ops->abort_frame(&backend);
}

int main(void)
{
    ops=wf_saturn_yaul_render_ops();unsigned cases=0;
    static result_t reference,candidate;
    /* Exclude the host-pointer prefix so every byte observed is deterministic
     * on both runs; visit every remaining valid48-byte backend window. */
    for(unsigned alias=0;alias<2;alias++)
        for(size_t offset=offsetof(wf_saturn_yaul_backend_t,expected_tiles);
            offset+48u<=sizeof backend;offset++){
            aliased_run(&reference,0,offset,alias);aliased_run(&candidate,1,offset,alias);
            if(memcmp(&reference,&candidate,sizeof reference)){
                fprintf(stderr,"bulk alias mismatch offset=%zu sources=%u\n",offset,alias);assert(0);
            }
            ++cases;
        }
    printf("PASS aliased bulk/scalar observations: %u backend byte windows, source alias, counter wrap\n",cases);
    return 0;
}
