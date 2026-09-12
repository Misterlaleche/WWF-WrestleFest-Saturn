#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wp12_romexact.h"
#include "wp12_renderer_contract.h"

static void base_ctx(wp12_ctx_t *c, wp12_mode_t mode)
{
    memset(c,0,sizeof(*c));
    c->mode=mode;
    c->global_1c0162=0;
    c->global_1c0066=0x08;
    for (unsigned i=0;i<WP12_WRESTLER_RECORDS;i++) {
        c->wrestlers[i].active=(i<6);
        c->wrestlers[i].identity_word_plus56=(uint16_t)i;
    }
}

static void supply_c98(wp12_ctx_t *c)
{
    static const uint16_t tagxy[4][2]={{0x240,0x150},{0x220,0x160},{0x2c0,0x150},{0x2e0,0x160}};
    static const uint16_t rxy[6][2]={{0x240,0x120},{0x2d0,0x120},{0x230,0x150},{0x2c0,0x150},{0x240,0x180},{0x2b0,0x180}};
    static const uint16_t ro[6]={0x8000,0,0x8000,0,0x8000,0};
    c->referee.active=true; c->referee.type_plus02=0x000c; c->referee.x_plus06=0x0280;
    c->referee.y_plus0a=0x0198; c->referee.z_plus0e=0x0140; c->referee.primary_plus20=0x8000;
    c->referee.plus34=0; c->global_1c1697=1;
    if (c->mode==WP12_MODE_TAG) {
        for (unsigned i=0;i<4;i++) { c->wrestlers[i].x_plus06=tagxy[i][0]; c->wrestlers[i].y_plus0a=tagxy[i][1]; c->wrestlers[i].z_plus0e=0x0140; }
    } else {
        for (unsigned i=0;i<6;i++) {
            c->wrestlers[i].active=true; c->wrestlers[i].orientation_plus2e=ro[i]; c->wrestlers[i].draw_plus04=ro[i];
            c->wrestlers[i].x_plus06=rxy[i][0]; c->wrestlers[i].y_plus0a=rxy[i][1]; c->wrestlers[i].z_plus0e=0x0140;
            c->wrestlers[i].plus32=1; c->wrestlers[i].plus34=0; c->wrestlers[i].primary_plus20=0; c->wrestlers[i].plusb6=0x8000;
        }
    }
}

static int find_kind(const wp12_batch_t *b, wp12_op_kind_t k)
{
    for (unsigned i=0;i<b->count;i++) if (b->ops[i].kind==k) return (int)i;
    return -1;
}

static unsigned count_audio(const wp12_batch_t *b, uint16_t cmd)
{
    unsigned n=0;
    for (unsigned i=0;i<b->count;i++) if (b->ops[i].kind==WP12_OP_AUDIO_RAW && b->ops[i].arg==cmd) n++;
    return n;
}

static void begin_and_confirm(wp12_ctx_t *c)
{
    wp12_batch_t b;
    assert(wp12_begin_paid_route(c,&b)==WP12_NEED_COMMON_SETUP);
    assert(find_kind(&b,WP12_OP_CALL_7B70)>=0);
    assert(find_kind(&b,WP12_OP_CALL_C98)>find_kind(&b,WP12_OP_CALL_7B70));
    supply_c98(c);
    assert(wp12_confirm_common_setup(c,true,true)==WP12_OK);
}

static void test_activation_predicate(void)
{
    assert(wp12_tag_activation_command(0)==0x3032);
    assert(wp12_tag_activation_command(4)==0x3032);
    assert(wp12_tag_activation_command(5)==0x3031);
    assert(wp12_tag_activation_command(11)==0x3031);
}

static void test_c08_tag_block(void)
{
    wp12_ctx_t c; wp12_batch_t b;
    base_ctx(&c,WP12_MODE_TAG); c.global_1c0160=0x0040; c.wrestlers[0].active=true; c.wrestlers[2].active=true;
    assert(wp12_begin_paid_route(&c,&b)==WP12_NEED_COMMON_SETUP);
    assert((c.global_1c0160&0x40)!=0);
    for (unsigned i=0;i<4;i++) assert(c.wrestlers[i].plus104==0x8000);

    base_ctx(&c,WP12_MODE_TAG); c.global_1c0160=0x0040; c.wrestlers[2].active=false;
    assert(wp12_begin_paid_route(&c,&b)==WP12_NEED_COMMON_SETUP);
    assert((c.global_1c0160&0x40)==0);
}

static void test_optional_interstitial(void)
{
    wp12_ctx_t c; wp12_batch_t b;
    base_ctx(&c,WP12_MODE_TAG); c.global_1c0163=4;
    assert(wp12_begin_paid_route(&c,&b)==WP12_NEED_COMMON_SETUP);
    assert(find_kind(&b,WP12_OP_CALL_AE20)>=0);
    assert(find_kind(&b,WP12_OP_CALL_264E2)>find_kind(&b,WP12_OP_CALL_AE20));
}

static void test_setup_validation(void)
{
    wp12_ctx_t c; wp12_batch_t b;
    base_ctx(&c,WP12_MODE_ROYAL); assert(wp12_begin_paid_route(&c,&b)==WP12_NEED_COMMON_SETUP); supply_c98(&c);
    c.wrestlers[0].primary_plus20=1;
    assert(wp12_confirm_common_setup(&c,true,true)==WP12_BLOCKED_BAD_SETUP);
}

static void test_referee_entry_offset_and_skip_order(void)
{
    wp12_ctx_t c; wp12_batch_t b; wp12_iteration_input_t in={true,true,0};
    base_ctx(&c,WP12_MODE_TAG); begin_and_confirm(&c);
    assert(c.referee.x_plus06==0x0280);
    assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    assert(c.referee.x_plus06==0x0300); /* A654 one-time +$80 when global163==0 */
    assert(find_kind(&b,WP12_OP_RENDER_REFEREE)>=0);
    assert(find_kind(&b,WP12_OP_VIDEO_SYNC_2836)>find_kind(&b,WP12_OP_RENDER_REFEREE));
    assert(find_kind(&b,WP12_OP_RESOURCE_TEARDOWN_2B58)>find_kind(&b,WP12_OP_VIDEO_SYNC_2836));
    assert(count_audio(&b,0x3132)==1);
    assert(c.phase==WP12_PHASE_HANDOFF);
}

static void test_global163_no_ref_offset_and_ffff(void)
{
    wp12_ctx_t c; wp12_batch_t b; wp12_iteration_input_t in={false,true,0};
    base_ctx(&c,WP12_MODE_TAG); c.global_1c0163=1; begin_and_confirm(&c);
    assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    assert(c.referee.x_plus06==0x0280);
    assert(c.presenter.draw_plus04==0xffff || c.phase==WP12_PHASE_HANDOFF);
}

static void test_identity_one_physical_byte(void)
{
    wp12_wrestler_t a; memset(&a,0,sizeof(a)); a.identity_word_plus56=0xab04;
    assert(wp12_identity57(&a)==4);
    assert(wp12_tag_identity_window_threshold(wp12_identity57(&a))==0x80);
    assert(wp12_tag_identity_voice_command(wp12_identity57(&a))==0x3173);
    assert(wp12_tag_activation_command(wp12_identity57(&a))==0x3032);
}

static void run_until_state(wp12_ctx_t *c, uint8_t target)
{
    wp12_batch_t b; wp12_iteration_input_t in={false,true,0};
    unsigned guard=10000;
    while (c->phase==WP12_PHASE_VISIBLE && c->presenter.state_plus1d!=target && guard--) {
        wp12_status_t s=wp12_visible_iteration(c,&in,&b);
        assert(s==WP12_OK);
    }
    assert(guard>0);
}

static void test_voice73_skip(void)
{
    wp12_ctx_t c;
    base_ctx(&c,WP12_MODE_TAG); c.wrestlers[0].identity_word_plus56=4; begin_and_confirm(&c);
    run_until_state(&c,0x04);
    run_until_state(&c,0x05);
    assert(c.global_1c0168==0x8000);
    run_until_state(&c,0x0a); /* 05 terminal adds 4 then +1 */
    assert(c.presenter.state_plus1d==0x0a);
}

static void test_id0a_second_record_activation(void)
{
    wp12_ctx_t c; wp12_batch_t b; wp12_iteration_input_t in={false,true,0};
    base_ctx(&c,WP12_MODE_TAG); begin_and_confirm(&c);
    c.presenter.state_plus1d=0x05; c.presenter.init_latch=false; c.wrestlers[0].identity_word_plus56=0x000a;
    c.wrestlers[1].local_word_plus1c=0;
    assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    assert((c.wrestlers[0].local_word_plus1c&0xff)==1);
    assert((c.wrestlers[1].local_word_plus1c&0xff)==1);
}

static void test_local_program_threshold_plus_one(void)
{
    wp12_ctx_t c; wp12_batch_t b; wp12_iteration_input_t in={false,true,0};
    base_ctx(&c,WP12_MODE_TAG); begin_and_confirm(&c);
    c.presenter.state_plus1d=0x07; /* avoid RNG/controller complexity; instant */
    c.wrestlers[0].identity_word_plus56=0;
    c.wrestlers[0].local_word_plus1c=1;
    c.wrestlers[0].counter_plus22=0; c.wrestlers[0].phase_plus24=0;
    for (unsigned i=0;i<6;i++) { assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK); assert(c.wrestlers[0].phase_plus24==0); }
    assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK); /* 7th callback exceeds threshold 6 */
    assert(c.wrestlers[0].phase_plus24==1);
}

static void test_presenter_state15_cycles(void)
{
    wp12_ctx_t c; wp12_batch_t b; wp12_iteration_input_t in={false,true,0};
    base_ctx(&c,WP12_MODE_TAG); begin_and_confirm(&c);
    c.presenter.active=true; c.presenter.flags_plus00=0x8001; c.presenter.state_plus1d=0x15; c.presenter.y_half_units=0x0178*2;
    for (unsigned i=0;i<48;i++) assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    assert((c.presenter.draw_plus04&0xff)==11);
    assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    assert((c.presenter.draw_plus04&0xff)==8);
}

static void test_royal_traversal_continues(void)
{
    wp12_ctx_t c; wp12_batch_t b; wp12_iteration_input_t in={false,false,0};
    base_ctx(&c,WP12_MODE_ROYAL); begin_and_confirm(&c);
    c.wrestlers[0].counter_plus22=63; c.wrestlers[1].counter_plus22=63;
    assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    assert(c.wrestlers[0].counter_plus22==64 && c.wrestlers[1].counter_plus22==64);
    assert(count_audio(&b,0x312f)>=3); /* two actor terminals + common teardown */
    assert(c.phase==WP12_PHASE_HANDOFF);
}

static void test_batch_handoff_order(void)
{
    wp12_ctx_t c; wp12_batch_t b; wp12_iteration_input_t in={true,true,0};
    int i_teardown,i_a,i_b,i_c,i_8ae6,i_f8c;
    base_ctx(&c,WP12_MODE_TAG); begin_and_confirm(&c);
    assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    i_teardown=find_kind(&b,WP12_OP_RESOURCE_TEARDOWN_2B58);
    i_8ae6=find_kind(&b,WP12_OP_CALL_8AE6); i_f8c=find_kind(&b,WP12_OP_HANDOFF_F8C);
    i_a=i_b=i_c=-1;
    for (unsigned i=(unsigned)i_teardown;i<b.count;i++) if (b.ops[i].kind==WP12_OP_AUDIO_RAW) {
        if (b.ops[i].arg==0x3100 && i_a<0) i_a=(int)i;
        else if (b.ops[i].arg==0x3120 && i_b<0) i_b=(int)i;
        else if (b.ops[i].arg==0x312f && i_c<0) i_c=(int)i;
    }
    assert(i_a>=0 && i_b==i_a+1 && i_c==i_b+1);
    assert(i_8ae6>i_c && i_f8c>i_8ae6);
}

static void test_renderer_contract(void)
{
    wp12_ctx_t c; wp12_render_item_t item; wp12_op_t op={WP12_OP_RENDER_WRESTLER,0,0,0};
    memset(&c,0,sizeof(c)); c.wrestlers[0].x_plus06=0x240; c.wrestlers[0].y_plus0a=0x150; c.wrestlers[0].z_plus0e=0x140; c.wrestlers[0].draw_plus04=0x59;
    assert(wp12_renderer_item_from_op(&c,&op,&item));
    assert(item.machine_x==0x240 && item.machine_y==0x150 && item.selector_word==0x59);
}


static void test_natural_durations(void)
{
    wp12_ctx_t c; wp12_batch_t b; wp12_iteration_input_t in={false,true,0};
    unsigned guard;

    base_ctx(&c,WP12_MODE_TAG);
    c.wrestlers[0].identity_word_plus56=0;
    c.wrestlers[1].identity_word_plus56=1;
    c.wrestlers[2].identity_word_plus56=2;
    c.wrestlers[3].identity_word_plus56=3;
    begin_and_confirm(&c);
    guard=5000;
    while (c.phase==WP12_PHASE_VISIBLE && guard--) {
        assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    }
    assert(guard>0);
    assert(c.visible_updates==1441u); /* 989 + 65 + 129 + 129 + 129 */
    assert(c.phase==WP12_PHASE_HANDOFF);

    base_ctx(&c,WP12_MODE_ROYAL);
    begin_and_confirm(&c);
    guard=1000;
    while (c.phase==WP12_PHASE_VISIBLE && guard--) {
        assert(wp12_visible_iteration(&c,&in,&b)==WP12_OK);
    }
    assert(guard>0);
    assert(c.visible_updates==64u);
    assert(c.phase==WP12_PHASE_HANDOFF);
}

int main(void)
{
    test_activation_predicate();
    test_c08_tag_block();
    test_optional_interstitial();
    test_setup_validation();
    test_referee_entry_offset_and_skip_order();
    test_global163_no_ref_offset_and_ffff();
    test_identity_one_physical_byte();
    test_voice73_skip();
    test_id0a_second_record_activation();
    test_local_program_threshold_plus_one();
    test_presenter_state15_cycles();
    test_royal_traversal_continues();
    test_batch_handoff_order();
    test_renderer_contract();
    test_natural_durations();
    puts("WP12 R02 host/adversarial tests: PASS");
    return 0;
}
