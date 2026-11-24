/* Included after the unchanged native oracle fixtures. The actual main.c,
 * frontend bridge, clock, runtime and translated owners remain linked. Only
 * physical time/video scheduling are deterministic observers, as in the full
 * native oracle. This is a clock/admission test, not a GPU ownership model. */

static void same_field_admission(void)
{
    assert(!awaiting_ack && !rt.g.frame_counter && !arcade_clock.pending);
    const unsigned field=host_fields;
    arcade_clock.last_field=field;
    arcade_clock.pending=arcade_clock.max_pending=3u;
    frontend.combat_coin_clock.last_field=field;
    frontend.combat_coin_clock.pending=0;
    unsigned irq2=frontend.combat_coin_irqs,irq3=frontend.vblank_irqs;
    unsigned front_ticks=frontend.arcade_ticks,committed=wf_jo_diagnostic.draws;
    for(unsigned i=0;i<3u;i++){
        unsigned old_allow=focal_allow_calls;
        game_frame();
        assert(!failed && host_fields==field && running && awaiting_ack);
        assert(arcade_clock.pending==2u-i && rt.g.frame_counter==i);
        assert(wf_jo_diagnostic.draws==committed+i+1u);
        assert(frontend.combat_coin_irqs==irq2 && frontend.vblank_irqs==irq3);
        assert(frontend.arcade_ticks==front_ticks);
        if(i<2u){
            assert(focal_allow_calls==old_allow+1u && focal_hint);
            jo_core_synch_callbacks();
            assert(host_fields==field && !focal_hint);
        }else assert(focal_allow_calls==old_allow && !focal_hint);
    }
    /* Even an unsolicited extra callback at this exact physical field cannot
     * ACK the last frame or generate another tick/IRQ with no admitted debt. */
    wf_runtime_t before=rt;
    for(unsigned i=0;i<4u;i++)game_frame();
    assert(!failed && !memcmp(&before,&rt,sizeof rt));
    assert(wf_jo_diagnostic.draws==committed+3u && !arcade_clock.pending);
    assert(frontend.combat_coin_irqs==irq2 && frontend.vblank_irqs==irq3);
    assert(frontend.arcade_ticks==front_ticks && !focal_hint);
    printf("PASS same_field: three admitted frames, two ACKs, zero extra IRQs; four no-debt callbacks unchanged\n");
}

static void original_wait_65(void)
{
    static const unsigned residues[]={0u,1u,1558u,1559u,39166u,39167u};
    unsigned cases=0,coin_groups=0;
    for(unsigned close=0u;close<(TEST_WAIT_CLOSURE?2u:1u);close++)
    for(unsigned stride=1u;stride<=4u;stride++)for(unsigned ri=0;ri<6u;ri++){
        unsigned residue=residues[ri];
        unsigned closure=close*3u;
        focal_epoch=close;focal_drain_fields=closure;
        if(stride==4u && ri==5u)host_fields=UINT32_MAX-10u;
        host_stride=stride;
        unsigned start=host_fields,old_sync=focal_explicit_syncs,old_drain=focal_drain_completed;
        arcade_clock=(wf_jo_clock_t){start,residue,3u,3u};
        frontend.combat_coin_clock=(wf_jo_clock_t){start,residue,0u,0u};
        unsigned irqs=frontend.combat_coin_irqs,irq3=frontend.vblank_irqs;
        unsigned ticks=frontend.arcade_ticks,draws=wf_jo_diagnostic.draws;
        wf_runtime_t before=rt;
        /* Explicit original waits must discard a previously armed SDK hint. */
        jo_core_set_frame_overlap(true);
        assert(wait_21e6(&rt,64u,NULL)==WF_OK);
        unsigned closure_periods=(unsigned)(((uint64_t)closure*37609u+residue)/39168u);
        unsigned wait_residue=(unsigned)(((uint64_t)closure*37609u+residue)%39168u);
        uint64_t needed=65u*UINT64_C(39168)-wait_residue;
        uint64_t quantum=(uint64_t)stride*37609u;
        unsigned syncs=(unsigned)((needed+quantum-1u)/quantum);
        unsigned elapsed=host_fields-start;
        uint64_t scaled=(uint64_t)elapsed*37609u+residue;
        unsigned periods=(unsigned)(scaled/39168u);
        assert(focal_drain_completed==old_drain+close && !focal_epoch);
        assert(focal_explicit_syncs-old_sync==syncs && elapsed==closure+syncs*stride);
        assert(periods>=closure_periods+65u && periods<closure_periods+65u+stride);
        assert(arcade_clock.last_field==host_fields && arcade_clock.remainder==scaled%39168u);
        assert(arcade_clock.pending==3u+periods-65u && !focal_hint);
        assert(!memcmp(&before,&rt,sizeof rt));
        assert(frontend.combat_coin_irqs==irqs && frontend.vblank_irqs==irq3);
        assert(frontend.arcade_ticks==ticks && wf_jo_diagnostic.draws==draws);
        /* IRQ2 debt belongs to its independent physical clock. One later
         * service consumes each group once; repeated same-field service cannot
         * regenerate groups or resample its input snapshot. */
        assert(wf_jo_frontend_combat_coins(&frontend)==WF_OK);
        assert(frontend.combat_coin_irqs==irqs+17u*periods);
        unsigned queries=focal_input_queries;
        for(unsigned i=0;i<3u;i++)assert(wf_jo_frontend_combat_coins(&frontend)==WF_OK);
        assert(frontend.combat_coin_irqs==irqs+17u*periods && focal_input_queries==queries);
        assert(frontend.vblank_irqs==irq3 && frontend.arcade_ticks==ticks);
        ++cases;coin_groups+=periods;
    }
    host_stride=1u;
    printf("PASS wait64: %u phase/stride/wrap cases, closure0%s then exactly65 periods, %u coin groups serviced once\n",
           cases,TEST_WAIT_CLOSURE?"/3":"",coin_groups);
}

static void accepted_start(unsigned direct_zero)
{
#if TEST_OVERLAP_VARIANT
    /* A real cabinet coin pulse, then physical START. Keep the gameplay image
     * produced by the three actual native frames; do not synthesize actors. */
    unsigned credits=rt.g.credit_count_004f;
    focal_pad_keys=JO_KEY_C;host_fields+=2u;
    assert(wf_jo_frontend_combat_coins(&frontend)==WF_OK);
    focal_pad_keys=0;host_fields+=2u;
    assert(wf_jo_frontend_combat_coins(&frontend)==WF_OK);
    assert(rt.g.credit_count_004f==credits+1u && rt.g.credited_route==0u);
    credits=rt.g.credit_count_004f;
    const unsigned start=host_fields,residue=39167u,pending=direct_zero?0u:4u;
    arcade_clock=(wf_jo_clock_t){start,residue,pending,pending};
    frontend.combat_coin_clock=(wf_jo_clock_t){start,residue,0u,0u};
    const unsigned irqs=frontend.combat_coin_irqs,irq3=frontend.vblank_irqs;
    const unsigned ticks=frontend.arcade_ticks,native_tick=rt.g.frame_counter;
    const unsigned generation=frontend.driver.generation,draws=wf_jo_diagnostic.draws;
    const unsigned debt=wf_jo_diagnostic.handoff_debt_total,handovers=wf_jo_diagnostic.handoff_count;
    const unsigned old_drain=focal_drain_completed,old_start=focal_start_8ae_calls;
    focal_epoch=TEST_START_EPOCH;focal_drain_fields=direct_zero?0u:3u;
    focal_transport_pending=1;focal_start_drain_seen=0;
    jo_core_set_frame_overlap(true);focal_pad_keys=JO_KEY_START;
    if(direct_zero){
        unsigned accepted=123u;
        assert(wf_jo_frontend_combat_start(&frontend,&accepted)==WF_OK && accepted==1u);
    }else game_frame();
    assert(!failed && focal_drain_completed==old_drain+TEST_START_EPOCH && focal_start_8ae_calls==old_start+1u);
    assert(!focal_epoch && !focal_hint && !frontend.handed_off);
    assert(frontend.driver.phase==WF_R18_DRIVER_MODE_PLATE && frontend.driver.pc==0x52beu);
    assert(frontend.driver.generation==generation+1u && rt.g.credit_count_004f==credits-1u);
    assert(rt.g.frame_counter==native_tick && wf_jo_diagnostic.draws==draws);
    assert(frontend.arcade_ticks==ticks && frontend.combat_coin_irqs==irqs && frontend.vblank_irqs==irq3);
    unsigned anchor_fields=TEST_START_EPOCH?focal_drain_fields:0u;
    uint64_t scaled=(uint64_t)anchor_fields*37609u+residue;
    unsigned admitted=pending+(unsigned)(scaled/39168u);
    unsigned expected_debt=admitted?admitted-1u:0u;
    assert(host_fields-start==TEST_START_TRANSPORT+anchor_fields+focal_injected_boundary);
    assert(arcade_clock.last_field==start+TEST_START_TRANSPORT+anchor_fields && arcade_clock.remainder==scaled%39168u && !arcade_clock.pending);
    assert(frontend.combat_start_debt==expected_debt);
    if(!direct_zero){
        assert(!awaiting_ack && !running && frontend_state==1u);
        assert(wf_jo_diagnostic.handoff_count==handovers+1u);
        assert(wf_jo_diagnostic.handoff_debt_total==debt+expected_debt);
    }
    wf_jo_clock_t next_owner=arcade_clock;
    assert(wf_jo_clock_advance(&next_owner,host_fields));
    uint64_t full_scaled=(uint64_t)(host_fields-start-TEST_START_TRANSPORT)*37609u+residue;
    assert(next_owner.remainder==full_scaled%39168u);
    assert(next_owner.pending==(unsigned)(full_scaled/39168u-scaled/39168u));
    printf("PASS accepted_START: direct_zero=%u elapsed=%u debt=%u; drain before8AE, no intervening IRQ/tick/ACK\n",
           direct_zero,host_fields-start,expected_debt);
#else
    (void)direct_zero;
#endif
}

int main(int argc,char **argv)
{
    setvbuf(stdout,NULL,_IONBF,0);
    assert(argc==2||argc==3);host_output=argv[1];selected_royal=1;
    unsigned direct_zero=argc==3;
    void *ram=mmap((void *)0x00200000u,0x100000u,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    assert(ram==(void *)0x00200000u);
    wf_host_assets_init();wf_jo_assets_set_service_hook(service_during_assets,NULL);
    selected_demo=1;loaded=1;wf_jo_clock_start(&arcade_clock,host_fields);
    assert(init_platform());assert(init_frontend());
    while(frontend_state&&!failed){game_frame();if(frontend_state)__jo_core_synch();}
    assert(!failed && host_entry_seen && !frontend_state);
    same_field_admission();
    if(!direct_zero&&!TEST_ATTRACT)original_wait_65();
    if(!rt.g.credited_route&&!TEST_WAIT_CLOSURE)accepted_start(direct_zero);
    else puts("Paid Royal route retained; accepted START exercised separately on original attract entry");
    puts("PASS focal actual-main clock/admission review");
    return 0;
}
