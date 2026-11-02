/* Confirm that observation leaves the original CPU, RAM, IO and write trace
 * unchanged. ROM is supplied privately; no extracted table is a test fixture.
 */
#define WF_MUSIC_STATE_PROBE_NO_MAIN
#include "../tools/reference/probe_music_state.c"
#include <assert.h>

static z80 comparable_cpu(const z80 *source)
{
    z80 c=*source;
    c.read_byte=NULL;c.write_byte=NULL;c.port_in=NULL;c.port_out=NULL;c.userdata=NULL;
    return c;
}

static void same_state(wf_ctx_t *plain,observed_t *observed)
{
    wf_ctx_t *b=&observed->original;
    z80 a_cpu=comparable_cpu(&plain->cpu),b_cpu=comparable_cpu(&b->cpu);
    assert(!memcmp(&a_cpu,&b_cpu,sizeof a_cpu));
    assert(!memcmp(plain->ram,b->ram,sizeof plain->ram));
    assert(plain->latch==b->latch&&plain->ym_addr==b->ym_addr);
    assert(plain->ym_status==b->ym_status&&plain->ym_tb==b->ym_tb);
    assert(plain->ym_ctl==b->ym_ctl&&plain->bank==b->bank&&plain->next_tb==b->next_tb);
}

static void run_until(wf_ctx_t *plain,observed_t *observed,uint64_t end)
{
    unsigned checkpoint=0;
    while(now(plain)<end){
        z80_step(&plain->cpu);update_timer(plain);observe_step(observed);
        if(++checkpoint==4096){same_state(plain,observed);checkpoint=0;}
    }
    same_state(plain,observed);
}

static void same_trace(FILE *a,FILE *b)
{
    unsigned char left[4096],right[4096];size_t n,m;
    fflush(a);fflush(b);rewind(a);rewind(b);
    do{n=fread(left,1,sizeof left,a);m=fread(right,1,sizeof right,b);
        assert(n==m&&!memcmp(left,right,n));}while(n);
    assert(!ferror(a)&&!ferror(b));
}

int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    for(unsigned command=1;command<=18;command++){
        wf_ctx_t *plain=calloc(1,sizeof *plain);observed_t *observed=calloc(1,sizeof *observed);
        assert(plain&&observed&&load(argv[1],plain->rom,sizeof plain->rom));
        memcpy(observed->original.rom,plain->rom,sizeof plain->rom);
        plain->log=tmpfile();observed->original.log=tmpfile();
        assert(plain->log&&observed->original.log);
        z80_init(&plain->cpu);plain->cpu.read_byte=rb;plain->cpu.write_byte=wb;
        plain->cpu.port_in=pin;plain->cpu.port_out=pout;plain->cpu.userdata=plain;
        observed->original.cpu=plain->cpu;observed->original.cpu.write_byte=observed_write;
        observed->original.cpu.userdata=observed;
        run_until(plain,observed,2*WF_CLK);
        observed->origin=now(plain);observed->capturing=1;
        plain->latch=observed->original.latch=(uint8_t)command;
        z80_gen_nmi(&plain->cpu);z80_gen_nmi(&observed->original.cpu);
        run_until(plain,observed,observed->origin+4*WF_CLK);
        plain->latch=observed->original.latch=0x1f;
        z80_gen_nmi(&plain->cpu);z80_gen_nmi(&observed->original.cpu);
        observed->fade_sent_cycle=now(plain)-observed->origin;
        run_until(plain,observed,observed->origin+12*WF_CLK);
        same_trace(plain->log,observed->original.log);
        assert(observed->fade_clear_cycle&&plain->ram[6]==0);
        fclose(plain->log);fclose(observed->original.log);free(plain);free(observed);
    }
    puts("PASS: all18 original commands,4s score+1F+8s tail; CPU/RAM/IO/timer and complete write traces unchanged by observation");
    return 0;
}
