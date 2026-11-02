/* Generated from pinned MIT Z80 core and user ROM. General interpreter fallback remains. */
 #include "wf_music_seq.h"
extern uint16_t wf_cold_rw(z80*,uint16_t);
extern void wf_cold_ww(z80*,uint16_t,uint16_t);

extern void wf_cold_process_interrupts(z80*);
static inline void __attribute__((section(".wf_hotcode"))) wf_hot_process_tail(z80*z){
 if(z->iff_delay||z->nmi_pending||(z->int_pending&&z->iff1))wf_cold_process_interrupts(z);
}

 #ifdef __GNUC__
 #define WF_SMALL __attribute__((optimize("Os")))
 #else
 #define WF_SMALL
 #endif
 #ifdef __GNUC__
 
 #endif
 static inline uint8_t __attribute__((section(".wf_hotcode"))) wf_memory_read(z80 *z,uint16_t address){
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;
 if(address<0xc000)return v->rom[address];
 if(address<0xc800)return v->ram[address-0xc000];
 return z->read_byte(z->userdata,address);
}
static inline void __attribute__((section(".wf_hotcode"))) wf_memory_write(z80 *z,uint16_t address,uint8_t value){
 if(address>=0xc000&&address<0xc800){((wf_music_seq_t *)z->userdata)->ram[address-0xc000]=value;return;}
 z->write_byte(z->userdata,address,value);
}
#include "z80.h"

// MARK: timings
static const uint8_t cyc_00[256] __attribute__((section(".wf_hotdata_const"))) = {4, 10, 7, 6, 4, 4, 7, 4, 4, 11, 7, 6, 4, 4,
    7, 4, 8, 10, 7, 6, 4, 4, 7, 4, 12, 11, 7, 6, 4, 4, 7, 4, 7, 10, 16, 6, 4, 4,
    7, 4, 7, 11, 16, 6, 4, 4, 7, 4, 7, 10, 13, 6, 11, 11, 10, 4, 7, 11, 13, 6,
    4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4,
    4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4,
    7, 4, 7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7,
    4, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, 4,
    4, 4, 4, 4, 4, 7, 4, 5, 10, 10, 10, 10, 11, 7, 11, 5, 10, 10, 0, 10, 17, 7,
    11, 5, 10, 10, 11, 10, 11, 7, 11, 5, 4, 10, 11, 10, 0, 7, 11, 5, 10, 10, 19,
    10, 11, 7, 11, 5, 4, 10, 4, 10, 0, 7, 11, 5, 10, 10, 4, 10, 11, 7, 11, 5, 6,
    10, 4, 10, 0, 7, 11};

static const uint8_t cyc_ed[256] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 12,
    12, 15, 20, 8, 14, 8, 9, 12, 12, 15, 20, 8, 14, 8, 9, 12, 12, 15, 20, 8, 14,
    8, 9, 12, 12, 15, 20, 8, 14, 8, 9, 12, 12, 15, 20, 8, 14, 8, 18, 12, 12, 15,
    20, 8, 14, 8, 18, 12, 12, 15, 20, 8, 14, 8, 8, 12, 12, 15, 20, 8, 14, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 16, 8, 8, 8, 8, 16, 16, 16, 16, 8, 8, 8, 8,
    16, 16, 16, 16, 8, 8, 8, 8, 16, 16, 16, 16, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8};

static const uint8_t cyc_ddfd[256] __attribute__((section(".wf_hotdata_const"))) = {4, 4, 4, 4, 4, 4, 4, 4, 4, 15, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 15, 4, 4, 4, 4, 4, 4, 4, 14, 20, 10, 8, 8,
    11, 4, 4, 15, 20, 10, 8, 8, 11, 4, 4, 4, 4, 4, 23, 23, 19, 4, 4, 15, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 19, 4, 4, 4, 4, 4, 8, 8, 19, 4, 4, 4, 4, 4, 8,
    8, 19, 4, 4, 4, 4, 4, 8, 8, 19, 4, 8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8,
    8, 19, 8, 19, 19, 19, 19, 19, 19, 4, 19, 4, 4, 4, 4, 8, 8, 19, 4, 4, 4, 4,
    4, 8, 8, 19, 4, 4, 4, 4, 4, 8, 8, 19, 4, 4, 4, 4, 4, 8, 8, 19, 4, 4, 4, 4,
    4, 8, 8, 19, 4, 4, 4, 4, 4, 8, 8, 19, 4, 4, 4, 4, 4, 8, 8, 19, 4, 4, 4, 4,
    4, 8, 8, 19, 4, 4, 4, 4, 4, 8, 8, 19, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 14, 4, 23, 4,
    15, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 10, 4, 4, 4, 4,
    4, 4};

// MARK: helpers

// get bit "n" of number "val"
#define GET_BIT(n, val) (((val) >> (n)) & 1)

static inline uint8_t __attribute__((section(".wf_hotcode"))) rb(z80* const z, uint16_t addr) {
  return wf_memory_read(z,addr);
}

static inline void __attribute__((section(".wf_hotcode"))) wb(z80* const z, uint16_t addr, uint8_t val) {
  wf_memory_write(z,addr,val);
}

static inline uint16_t __attribute__((section(".wf_hotcode"))) rw(z80* const z, uint16_t addr) {

  wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;
  if(addr<0xbfffu)return (uint16_t)(((unsigned)v->rom[addr+1]<<8)|v->rom[addr]);
  if(addr>=0xc000u&&addr<0xc7ffu){
   unsigned at=addr-0xc000u;
   return (uint16_t)(((unsigned)v->ram[at+1]<<8)|v->ram[at]);
  }
  return wf_cold_rw(z,addr);

}

static inline void __attribute__((section(".wf_hotcode"))) ww(z80* const z, uint16_t addr, uint16_t val) {

  if(addr>=0xc000u&&addr<0xc7ffu){
   wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;unsigned at=addr-0xc000u;
   v->ram[at]=(uint8_t)val;v->ram[at+1]=(uint8_t)(val>>8);return;
  }
  wf_cold_ww(z,addr,val);

}

static inline void __attribute__((section(".wf_hotcode"))) pushw(z80* const z, uint16_t val) {
  z->sp -= 2;
  ww(z, z->sp, val);
}

static inline uint16_t __attribute__((section(".wf_hotcode"))) popw(z80* const z) {
  z->sp += 2;
  return rw(z, z->sp - 2);
}

static inline uint8_t __attribute__((section(".wf_hotcode"))) nextb(z80* const z) {
  return rb(z, z->pc++);
}

static inline uint16_t __attribute__((section(".wf_hotcode"))) nextw(z80* const z) {
  z->pc += 2;
  return rw(z, z->pc - 2);
}

static inline uint16_t __attribute__((section(".wf_hotcode"))) get_bc(z80* const z) {
  return (z->b << 8) | z->c;
}

static inline uint16_t __attribute__((section(".wf_hotcode"))) get_de(z80* const z) {
  return (z->d << 8) | z->e;
}

static inline uint16_t __attribute__((section(".wf_hotcode"))) get_hl(z80* const z) {
  return (z->h << 8) | z->l;
}

static inline void __attribute__((section(".wf_hotcode"))) set_bc(z80* const z, uint16_t val) {
  z->b = val >> 8;
  z->c = val & 0xFF;
}

static inline void __attribute__((section(".wf_hotcode"))) set_de(z80* const z, uint16_t val) {
  z->d = val >> 8;
  z->e = val & 0xFF;
}

static inline void __attribute__((section(".wf_hotcode"))) set_hl(z80* const z, uint16_t val) {
  z->h = val >> 8;
  z->l = val & 0xFF;
}

static inline uint8_t __attribute__((section(".wf_hotcode"))) get_f(z80* const z) {
  uint8_t val = 0;
  val |= z->cf << 0;
  val |= z->nf << 1;
  val |= z->pf << 2;
  val |= z->xf << 3;
  val |= z->hf << 4;
  val |= z->yf << 5;
  val |= z->zf << 6;
  val |= z->sf << 7;
  return val;
}

static inline void __attribute__((section(".wf_hotcode"))) set_f(z80* const z, uint8_t val) {
  z->cf = (val >> 0) & 1;
  z->nf = (val >> 1) & 1;
  z->pf = (val >> 2) & 1;
  z->xf = (val >> 3) & 1;
  z->hf = (val >> 4) & 1;
  z->yf = (val >> 5) & 1;
  z->zf = (val >> 6) & 1;
  z->sf = (val >> 7) & 1;
}

// increments R, keeping the highest byte intact
static inline void __attribute__((section(".wf_hotcode"))) inc_r(z80* const z) {
  z->r = (z->r & 0x80) | ((z->r + 1) & 0x7f);
}

// returns if there was a carry between bit "bit_no" and "bit_no - 1" when
// executing "a + b + cy"
static inline bool __attribute__((section(".wf_hotcode"))) carry(int bit_no, uint16_t a, uint16_t b, bool cy) {
  int32_t result = a + b + cy;
  int32_t carry = result ^ a ^ b;
  return carry & (1 << bit_no);
}

// returns the parity of byte: 0 if number of 1 bits in `val` is odd, else 1
static const uint8_t wf_parity4[16] __attribute__((section(".wf_hotdata_const")))={1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1};
static inline bool __attribute__((section(".wf_hotcode"))) parity(uint8_t val){return wf_parity4[(val^(val>>4))&15u]!=0;}

static WF_SMALL void exec_opcode(z80* const z, uint8_t opcode);
static WF_SMALL void exec_opcode_cb(z80* const z, uint8_t opcode);
static WF_SMALL void exec_opcode_dcb(
    z80* const z, const uint8_t opcode, const uint16_t addr);
static WF_SMALL void exec_opcode_ed(z80* const z, uint8_t opcode);
static WF_SMALL void exec_opcode_ddfd(z80* const z, uint8_t opcode, uint16_t* const iz);

// MARK: opcodes
// jumps to an address
static inline void __attribute__((section(".wf_hotcode"))) jump(z80* const z, uint16_t addr) {
  z->pc = addr;
  z->mem_ptr = addr;
}

// jumps to next word in memory if condition is true
static inline void __attribute__((section(".wf_hotcode"))) cond_jump(z80* const z, bool condition) {
  const uint16_t addr = nextw(z);
  if (condition) {
    jump(z, addr);
  }
  z->mem_ptr = addr;
}

// calls to next word in memory
static inline void __attribute__((section(".wf_hotcode"))) call(z80* const z, uint16_t addr) {
  pushw(z, z->pc);
  z->pc = addr;
  z->mem_ptr = addr;
}

// calls to next word in memory if condition is true
static inline void __attribute__((section(".wf_hotcode"))) cond_call(z80* const z, bool condition) {
  const uint16_t addr = nextw(z);
  if (condition) {
    call(z, addr);
    z->cyc += 7;
  }
  z->mem_ptr = addr;
}

// returns from subroutine
static inline void __attribute__((section(".wf_hotcode"))) ret(z80* const z) {
  z->pc = popw(z);
  z->mem_ptr = z->pc;
}

// returns from subroutine if condition is true
static inline void __attribute__((section(".wf_hotcode"))) cond_ret(z80* const z, bool condition) {
  if (condition) {
    ret(z);
    z->cyc += 6;
  }
}

static inline void __attribute__((section(".wf_hotcode"))) jr(z80* const z, int8_t displacement) {
  z->pc += displacement;
  z->mem_ptr = z->pc;
}

static inline void __attribute__((section(".wf_hotcode"))) cond_jr(z80* const z, bool condition) {
  const int8_t b = nextb(z);
  if (condition) {
    jr(z, b);
    z->cyc += 5;
  }
}

// ADD Byte: adds two bytes together
static inline uint8_t __attribute__((section(".wf_hotcode"))) addb(z80* const z, uint8_t a, uint8_t b, bool cy) {
  const uint8_t result = a + b + cy;
  z->sf = result >> 7;
  z->zf = result == 0;
  z->hf = carry(4, a, b, cy);
  z->pf = carry(7, a, b, cy) != carry(8, a, b, cy);
  z->cf = carry(8, a, b, cy);
  z->nf = 0;
  z->xf = GET_BIT(3, result);
  z->yf = GET_BIT(5, result);
  return result;
}

// SUBstract Byte: substracts two bytes (with optional carry)
static inline uint8_t __attribute__((section(".wf_hotcode"))) subb(z80* const z, uint8_t a, uint8_t b, bool cy) {
  uint8_t val = addb(z, a, ~b, !cy);
  z->cf = !z->cf;
  z->hf = !z->hf;
  z->nf = 1;
  return val;
}

// ADD Word: adds two words together
static inline uint16_t __attribute__((section(".wf_hotcode"))) addw(z80* const z, uint16_t a, uint16_t b, bool cy) {
  uint8_t lsb = addb(z, a, b, cy);
  uint8_t msb = addb(z, a >> 8, b >> 8, z->cf);

  uint16_t result = (msb << 8) | lsb;
  z->zf = result == 0;
  z->mem_ptr = a + 1;
  return result;
}

// SUBstract Word: substracts two words (with optional carry)
static inline uint16_t subw(z80* const z, uint16_t a, uint16_t b, bool cy) {
  uint8_t lsb = subb(z, a, b, cy);
  uint8_t msb = subb(z, a >> 8, b >> 8, z->cf);

  uint16_t result = (msb << 8) | lsb;
  z->zf = result == 0;
  z->mem_ptr = a + 1;
  return result;
}

// adds a word to HL
static inline void __attribute__((section(".wf_hotcode"))) addhl(z80* const z, uint16_t val) {
  bool sf = z->sf;
  bool zf = z->zf;
  bool pf = z->pf;
  uint16_t result = addw(z, get_hl(z), val, 0);
  set_hl(z, result);
  z->sf = sf;
  z->zf = zf;
  z->pf = pf;
}

// adds a word to IX or IY
static inline void __attribute__((section(".wf_hotcode"))) addiz(z80* const z, uint16_t* reg, uint16_t val) {
  bool sf = z->sf;
  bool zf = z->zf;
  bool pf = z->pf;
  uint16_t result = addw(z, *reg, val, 0);
  *reg = result;
  z->sf = sf;
  z->zf = zf;
  z->pf = pf;
}

// adds a word (+ carry) to HL
static inline void adchl(z80* const z, uint16_t val) {
  uint16_t result = addw(z, get_hl(z), val, z->cf);
  z->sf = result >> 15;
  z->zf = result == 0;
  set_hl(z, result);
}

// substracts a word (+ carry) to HL
static inline void sbchl(z80* const z, uint16_t val) {
  const uint16_t result = subw(z, get_hl(z), val, z->cf);
  z->sf = result >> 15;
  z->zf = result == 0;
  set_hl(z, result);
}

// increments a byte value
static inline uint8_t __attribute__((section(".wf_hotcode"))) inc(z80* const z, uint8_t a) {
  bool cf = z->cf;
  uint8_t result = addb(z, a, 1, 0);
  z->cf = cf;
  return result;
}

// decrements a byte value
static inline uint8_t __attribute__((section(".wf_hotcode"))) dec(z80* const z, uint8_t a) {
  bool cf = z->cf;
  uint8_t result = subb(z, a, 1, 0);
  z->cf = cf;
  return result;
}

// MARK: bitwise

// executes a logic "and" between register A and a byte, then stores the
// result in register A
static inline void __attribute__((section(".wf_hotcode"))) land(z80* const z, uint8_t val) {
  const uint8_t result = z->a & val;
  z->sf = result >> 7;
  z->zf = result == 0;
  z->hf = 1;
  z->pf = parity(result);
  z->nf = 0;
  z->cf = 0;
  z->xf = GET_BIT(3, result);
  z->yf = GET_BIT(5, result);
  z->a = result;
}

// executes a logic "xor" between register A and a byte, then stores the
// result in register A
static inline void __attribute__((section(".wf_hotcode"))) lxor(z80* const z, const uint8_t val) {
  const uint8_t result = z->a ^ val;
  z->sf = result >> 7;
  z->zf = result == 0;
  z->hf = 0;
  z->pf = parity(result);
  z->nf = 0;
  z->cf = 0;
  z->xf = GET_BIT(3, result);
  z->yf = GET_BIT(5, result);
  z->a = result;
}

// executes a logic "or" between register A and a byte, then stores the
// result in register A
static inline void lor(z80* const z, const uint8_t val) {
  const uint8_t result = z->a | val;
  z->sf = result >> 7;
  z->zf = result == 0;
  z->hf = 0;
  z->pf = parity(result);
  z->nf = 0;
  z->cf = 0;
  z->xf = GET_BIT(3, result);
  z->yf = GET_BIT(5, result);
  z->a = result;
}

// compares a value with register A
static inline void __attribute__((section(".wf_hotcode"))) cp(z80* const z, const uint8_t val) {
  subb(z, z->a, val, 0);

  // the only difference between cp and sub is that
  // the xf/yf are taken from the value to be substracted,
  // not the result
  z->yf = GET_BIT(5, val);
  z->xf = GET_BIT(3, val);
}

// 0xCB opcodes
// rotate left with carry
static inline uint8_t cb_rlc(z80* const z, uint8_t val) {
  const bool old = val >> 7;
  val = (val << 1) | old;
  z->sf = val >> 7;
  z->zf = val == 0;
  z->pf = parity(val);
  z->nf = 0;
  z->hf = 0;
  z->cf = old;
  z->xf = GET_BIT(3, val);
  z->yf = GET_BIT(5, val);
  return val;
}

// rotate right with carry
static inline uint8_t cb_rrc(z80* const z, uint8_t val) {
  const bool old = val & 1;
  val = (val >> 1) | (old << 7);
  z->sf = val >> 7;
  z->zf = val == 0;
  z->nf = 0;
  z->hf = 0;
  z->cf = old;
  z->pf = parity(val);
  z->xf = GET_BIT(3, val);
  z->yf = GET_BIT(5, val);
  return val;
}

// rotate left (simple)
static inline uint8_t cb_rl(z80* const z, uint8_t val) {
  const bool cf = z->cf;
  z->cf = val >> 7;
  val = (val << 1) | cf;
  z->sf = val >> 7;
  z->zf = val == 0;
  z->nf = 0;
  z->hf = 0;
  z->pf = parity(val);
  z->xf = GET_BIT(3, val);
  z->yf = GET_BIT(5, val);
  return val;
}

// rotate right (simple)
static inline uint8_t __attribute__((section(".wf_hotcode"))) cb_rr(z80* const z, uint8_t val) {
  const bool c = z->cf;
  z->cf = val & 1;
  val = (val >> 1) | (c << 7);
  z->sf = val >> 7;
  z->zf = val == 0;
  z->nf = 0;
  z->hf = 0;
  z->pf = parity(val);
  z->xf = GET_BIT(3, val);
  z->yf = GET_BIT(5, val);
  return val;
}

// shift left preserving sign
static inline uint8_t __attribute__((section(".wf_hotcode"))) cb_sla(z80* const z, uint8_t val) {
  z->cf = val >> 7;
  val <<= 1;
  z->sf = val >> 7;
  z->zf = val == 0;
  z->nf = 0;
  z->hf = 0;
  z->pf = parity(val);
  z->xf = GET_BIT(3, val);
  z->yf = GET_BIT(5, val);
  return val;
}

// SLL (exactly like SLA, but sets the first bit to 1)
static inline uint8_t cb_sll(z80* const z, uint8_t val) {
  z->cf = val >> 7;
  val <<= 1;
  val |= 1;
  z->sf = val >> 7;
  z->zf = val == 0;
  z->nf = 0;
  z->hf = 0;
  z->pf = parity(val);
  z->xf = GET_BIT(3, val);
  z->yf = GET_BIT(5, val);
  return val;
}

// shift right preserving sign
static inline uint8_t cb_sra(z80* const z, uint8_t val) {
  z->cf = val & 1;
  val = (val >> 1) | (val & 0x80); // 0b10000000
  z->sf = val >> 7;
  z->zf = val == 0;
  z->nf = 0;
  z->hf = 0;
  z->pf = parity(val);
  z->xf = GET_BIT(3, val);
  z->yf = GET_BIT(5, val);
  return val;
}

// shift register right
static inline uint8_t __attribute__((section(".wf_hotcode"))) cb_srl(z80* const z, uint8_t val) {
  z->cf = val & 1;
  val >>= 1;
  z->sf = val >> 7;
  z->zf = val == 0;
  z->nf = 0;
  z->hf = 0;
  z->pf = parity(val);
  z->xf = GET_BIT(3, val);
  z->yf = GET_BIT(5, val);
  return val;
}

// tests bit "n" from a byte
static inline uint8_t __attribute__((section(".wf_hotcode"))) cb_bit(z80* const z, uint8_t val, uint8_t n) {
  const uint8_t result = val & (1 << n);
  z->sf = result >> 7;
  z->zf = result == 0;
  z->yf = GET_BIT(5, val);
  z->hf = 1;
  z->xf = GET_BIT(3, val);
  z->pf = z->zf;
  z->nf = 0;
  return result;
}

static inline void ldi(z80* const z) {
  const uint16_t de = get_de(z);
  const uint16_t hl = get_hl(z);
  const uint8_t val = rb(z, hl);

  wb(z, de, val);

  set_hl(z, get_hl(z) + 1);
  set_de(z, get_de(z) + 1);
  set_bc(z, get_bc(z) - 1);

  // see https://wikiti.brandonw.net/index.php?title=Z80_Instruction_Set
  // for the calculation of xf/yf on LDI
  const uint8_t result = val + z->a;
  z->xf = GET_BIT(3, result);
  z->yf = GET_BIT(1, result);

  z->nf = 0;
  z->hf = 0;
  z->pf = get_bc(z) > 0;
}

static inline void ldd(z80* const z) {
  ldi(z);
  // same as ldi but HL and DE are decremented instead of incremented
  set_hl(z, get_hl(z) - 2);
  set_de(z, get_de(z) - 2);
}

static inline void cpi(z80* const z) {
  bool cf = z->cf;
  const uint8_t result = subb(z, z->a, rb(z, get_hl(z)), 0);
  set_hl(z, get_hl(z) + 1);
  set_bc(z, get_bc(z) - 1);
  z->xf = GET_BIT(3, result - z->hf);
  z->yf = GET_BIT(1, result - z->hf);
  z->pf = get_bc(z) != 0;
  z->cf = cf;
  z->mem_ptr += 1;
}

static inline void cpd(z80* const z) {
  cpi(z);
  // same as cpi but HL is decremented instead of incremented
  set_hl(z, get_hl(z) - 2);
  z->mem_ptr -= 2;
}

static void in_r_c(z80* const z, uint8_t* r) {
  *r = z->port_in(z, z->c);
  z->zf = *r == 0;
  z->sf = *r >> 7;
  z->pf = parity(*r);
  z->nf = 0;
  z->hf = 0;
}

static void ini(z80* const z) {
  uint8_t val = z->port_in(z, z->c);
  wb(z, get_hl(z), val);
  set_hl(z, get_hl(z) + 1);
  z->b -= 1;
  z->zf = z->b == 0;
  z->nf = 1;
  z->mem_ptr = get_bc(z) + 1;
}

static void ind(z80* const z) {
  ini(z);
  set_hl(z, get_hl(z) - 2);
  z->mem_ptr = get_bc(z) - 2;
}

static void outi(z80* const z) {
  z->port_out(z, z->c, rb(z, get_hl(z)));
  set_hl(z, get_hl(z) + 1);
  z->b -= 1;
  z->zf = z->b == 0;
  z->nf = 1;
  z->mem_ptr = get_bc(z) + 1;
}

static void outd(z80* const z) {
  outi(z);
  set_hl(z, get_hl(z) - 2);
  z->mem_ptr = get_bc(z) - 2;
}

static void daa(z80* const z) {
  // "When this instruction is executed, the A register is BCD corrected
  // using the  contents of the flags. The exact process is the following:
  // if the least significant four bits of A contain a non-BCD digit
  // (i. e. it is greater than 9) or the H flag is set, then $06 is
  // added to the register. Then the four most significant bits are
  // checked. If this more significant digit also happens to be greater
  // than 9 or the C flag is set, then $60 is added."
  // > http://z80-heaven.wikidot.com/instructions-set:daa
  uint8_t correction = 0;

  if ((z->a & 0x0F) > 0x09 || z->hf) {
    correction += 0x06;
  }

  if (z->a > 0x99 || z->cf) {
    correction += 0x60;
    z->cf = 1;
  }

  const bool substraction = z->nf;
  if (substraction) {
    z->hf = z->hf && (z->a & 0x0F) < 0x06;
    z->a -= correction;
  } else {
    z->hf = (z->a & 0x0F) > 0x09;
    z->a += correction;
  }

  z->sf = z->a >> 7;
  z->zf = z->a == 0;
  z->pf = parity(z->a);
  z->xf = GET_BIT(3, z->a);
  z->yf = GET_BIT(5, z->a);
}

static inline uint16_t displace(
    z80* const z, uint16_t base_addr, int8_t displacement) {
  const uint16_t addr = base_addr + displacement;
  z->mem_ptr = addr;
  return addr;
}



// MARK: interface
// initialises a z80 struct. Note that read_byte, write_byte, port_in, port_out
// and userdata must be manually set by the user afterwards.


// executes the next instruction in memory + handles interrupts


// outputs to stdout a debug trace of the emulator


// function to call when an NMI is to be serviced


// function to call when an INT is to be serviced


// executes a non-prefixed opcode


// executes a DD/FD opcode (IZ = IX or IY)


// executes a CB opcode


// executes a displaced CB opcode (DDCB or FDCB)


// executes a ED opcode




 #ifdef __GNUC__
 
 #endif
 static WF_SMALL __attribute__((section(".wf_hotcode"))) bool wf_prefixed_fast(z80 *z)
 {
  wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;uint16_t pc=z->pc;
  if(pc>=0xbffcu||z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||z->halted)return false;
  unsigned prefix=v->rom[pc];if(prefix!=0xdd&&prefix!=0xfd)return false;
  unsigned op=v->rom[pc+1],length=2;uint16_t *iz=prefix==0xdd?&z->ix:&z->iy;
  bool memory=(op>=0x70&&op<=0x77&&op!=0x76)||op==0x36||op==0x46||op==0x4e||op==0x56||op==0x5e||op==0x66||op==0x6e||op==0x7e;
  if(memory)length=op==0x36?4:3;
  else if(op==0x21)length=4;
  else if(op!=0x23&&op!=0xe1&&op!=0xe5&&op!=0x09)return false;
  z->pc=(uint16_t)(pc+length);z->cyc+=memory?19u:(op==0x23?10u:((op==0x09||op==0xe5)?15u:14u));z->r=(uint8_t)((z->r&128u)|((z->r+2u)&127u));
  if(memory){
   uint16_t address=(uint16_t)(*iz+(int8_t)v->rom[pc+2]);z->mem_ptr=address;
   if(op==0x36)wb(z,address,v->rom[pc+3]);
   else if(op>=0x70&&op<=0x77){
    uint8_t value=op==0x70?z->b:op==0x71?z->c:op==0x72?z->d:op==0x73?z->e:op==0x74?z->h:op==0x75?z->l:z->a;wb(z,address,value);
   }else{
    uint8_t value=rb(z,address);
    switch(op){case 0x46:z->b=value;break;case 0x4e:z->c=value;break;case 0x56:z->d=value;break;case 0x5e:z->e=value;break;case 0x66:z->h=value;break;case 0x6e:z->l=value;break;default:z->a=value;break;}
   }
  }else if(op==0x21)*iz=(uint16_t)(v->rom[pc+2]|(v->rom[pc+3]<<8));
  else if(op==0x23)++*iz;
  else if(op==0xe1)*iz=popw(z);
  else if(op==0xe5)pushw(z,*iz);
  else addiz(z,iz,get_bc(z));
  return true;
 }

#ifdef WF_COST_PROFILE
uint64_t wf_cost_hits[2];
#define WF_COST_HIT(n) (++wf_cost_hits[n])
#else
#define WF_COST_HIT(n) ((void)0)
#endif
/* Experimental exact hot-path lift. Only entered with no pending interrupt,
 * no timer/target boundary in the complete path, and enough instruction budget. */

static WF_SMALL __attribute__((section(".wf_hotcode"))) void wf_cost_mod(unsigned *de,unsigned control,unsigned delta,unsigned old_hl,bool second,unsigned *steps,unsigned *ticks,unsigned *refresh,unsigned *stack4)
{
 if(((control&1u)!=0)!=second){
  unsigned result=*de+delta;
  if(result>65535u){*de=old_hl;*steps+=7;*ticks+=65;*refresh+=8;return;}
  *steps+=11;*ticks+=79;*refresh+=12;
  if(result>=0x6000u){result=0x5fff;*steps+=2;*ticks+=27;*refresh+=2;*stack4=second?0x0976:0x0953;}
  *de=result;
 }else{
  int result=(int)*de-(int)delta;
  *steps+=13;*ticks+=94;*refresh+=15;
  if(result<0){result=0;*steps+=2;*ticks+=27;*refresh+=2;*stack4=second?0x0980:0x095d;}
  if(result>=0x6000){result=0x5fff;*steps+=2;*ticks+=27;*refresh+=2;*stack4=second?0x0987:0x0964;}
  *de=(unsigned)result;
 }
}
static WF_SMALL __attribute__((section(".wf_hotcode"))) unsigned wf_cost_macro(z80 *z,unsigned budget,uint32_t limit)
{
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;
 if(z->pc==0x08c1&&budget>=57u){
  unsigned ch=v->ram[0x65];
  if(ch<8u&&z->sp>=0xc408&&z->sp<=0xc7fe){
   unsigned d=z->a,e=v->ram[0xb2+ch],offset=v->ram[0xaa+ch],steps=57,ticks=441,refresh=58,stack4=0;
   if(offset&0x80){
    offset&=0x7f;steps+=5;refresh+=5;
    if(d<offset){d=e=0;ticks+=40;}else{d-=offset;ticks+=29;}
   }else d=(uint8_t)(d+offset);
   unsigned de=(d<<8)|e;
   for(unsigned mod=0;mod<2;mod++){
    unsigned at=(mod?0x112:0xd2)+8*ch;
    if(v->ram[at]){
     steps+=7;ticks+=56;refresh+=7;
     unsigned delta=((unsigned)v->ram[at+2]<<8)|v->ram[at+3];
     wf_cost_mod(&de,v->ram[at+1],delta,0xc000+at+3,mod!=0,&steps,&ticks,&refresh,&stack4);
    }
   }
   d=de>>8;e=de&255;
   if(d>0x60){++steps;ticks+=5;++refresh;}
   if(budget>=steps&&(int32_t)(limit-(uint32_t)z->cyc)>=(int32_t)ticks){
    WF_COST_HIT(0);
    uint16_t sp=z->sp,back=(uint16_t)(v->ram[sp-0xc000]|((unsigned)v->ram[sp-0xbfff]<<8));
    v->ram[sp-0xc002]=0x20;v->ram[sp-0xc001]=0x09;
    if(stack4){v->ram[sp-0xc004]=(uint8_t)stack4;v->ram[sp-0xc003]=(uint8_t)(stack4>>8);}
    z->a=0x60;cp(z,(uint8_t)d);
    d=v->rom[0xf61+d];v->ram[0x9a+ch]=(uint8_t)d;v->ram[0xa2+ch]=(uint8_t)e;
    z->a=(uint8_t)e;z->b=0;z->c=(uint8_t)ch;z->d=(uint8_t)d;z->e=(uint8_t)e;z->h=0xc0;z->l=(uint8_t)(0xa2+ch);
    z->yf=0;z->hf=0;z->xf=0;z->nf=0;z->cf=0;
    z->pc=back;z->mem_ptr=back;z->sp=sp+2;z->cyc+=ticks;
    z->r=(uint8_t)((z->r&0x80)|((z->r+refresh)&127u));
    return steps;
   }
  }
 }
 if(z->pc==0x0f4b&&budget>=12u&&(int32_t)(limit-(uint32_t)z->cyc)>=81&&!(v->status&0x80)){
  WF_COST_HIT(1);
  /* EX AF,AF'; status read; AND80; JR NZ untaken; address/data stores.
   * Keep the observable CPU state and exact timestamp at both I/O stores. */
  z->a_=z->a;z->f_=get_f(z);set_f(z,0x54);
  z->a=z->d;z->pc=0x0f57;z->mem_ptr=0xc802;z->cyc+=48;
  z->r=(uint8_t)((z->r&0x80)|((z->r+6u)&127u));wb(z,0xc800,z->a);z->mem_ptr=(uint16_t)((z->a<<8)|1u);
  z->a=z->e;z->pc=0x0f5f;z->cyc+=33;
  z->r=(uint8_t)((z->r&0x80)|((z->r+6u)&127u));wb(z,0xc801,z->a);z->mem_ptr=(uint16_t)((z->a<<8)|2u);
  return 12;
 }
 return 0;
}

/* Exact setup+YM-write segments. End at the observable C801 store, before
 * EX AF,AF' and RET. The caller still commits timer/target boundaries there.
 * Uses pinned-core helpers and the same RAM adapter as the AOT executor. */
static WF_SMALL __attribute__((section(".wf_hotcode"))) unsigned wf_score_pitchwrite(z80 *z,unsigned budget,uint32_t limit)
{
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;
 unsigned pc=z->pc;
 if((pc!=0x0a8f&&pc!=0x0aa0)||budget<21u||
    (int32_t)(limit-(uint32_t)z->cyc)<161||
    (v->status&0x80)||z->sp<0xc402||z->sp>0xc7fe)return 0;
 unsigned ch=v->ram[0x65];if(ch>=8)return 0;
 unsigned base=pc==0x0a8f?0x28:0x30;
 unsigned at=pc==0x0a8f?0x9a:0xa2;
 uint16_t back=pc==0x0a8f?0x0aa0:0x0ab1;
 z->b=0;z->c=(uint8_t)ch;z->h=0xc0;z->l=(uint8_t)(at+ch);
 z->e=v->ram[at+ch];
 z->a=addb(z,(uint8_t)ch,(uint8_t)base,0);z->d=z->a;
 z->cyc+=80;z->r=(uint8_t)((z->r&0x80)|((z->r+9u)&127u));
 z->pc=0x0f4b;z->mem_ptr=0x0f4b;pushw(z,back);
 z->a_=z->a;z->f_=get_f(z);set_f(z,0x54);
 z->a=z->d;z->pc=0x0f57;z->mem_ptr=0xc802;z->cyc+=48;
 z->r=(uint8_t)((z->r&0x80)|((z->r+6u)&127u));
 wb(z,0xc800,z->a);z->mem_ptr=(uint16_t)((z->a<<8)|1u);
 z->a=z->e;z->pc=0x0f5f;z->cyc+=33;
 z->r=(uint8_t)((z->r&0x80)|((z->r+6u)&127u));
 wb(z,0xc801,z->a);z->mem_ptr=(uint16_t)((z->a<<8)|2u);
 return 21;
}

/* Exact 24-bit modulation accumulators at 0991 and 0A0B. */
static WF_SMALL __attribute__((section(".wf_hotcode"))) unsigned wf_score_modupdate(z80 *z,unsigned budget,uint32_t limit)
{
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;
 if((z->pc!=0x0991&&z->pc!=0x0a0b)||budget<11||
    z->sp<0xc408||z->sp>0xc7fe)return 0;
 unsigned ch=v->ram[0x65];if(ch>=8)return 0;
 bool second=z->pc==0x0a0b;
 unsigned at=(second?0x112:0xd2)+8*ch,steps=11,ticks=79,refresh=11;
 bool active=v->ram[at]!=0,subtract=false,clamp=false;
 if(active){
  unsigned value=((unsigned)v->ram[at+2]<<16)|((unsigned)v->ram[at+3]<<8)|v->ram[at+4];
  unsigned delta=((unsigned)v->ram[at+5]<<16)|((unsigned)v->ram[at+6]<<8)|v->ram[at+7];
  subtract=second||v->ram[at+1]<2;
  if(subtract){
   clamp=value<delta;
   steps=second?27:31;ticks=second?298:326;refresh=second?37:41;
   if(clamp){steps+=5;ticks+=80;refresh+=9;}
  }else{
   clamp=(((value+delta)&0xffffffu)>>16)>=0x60;
   steps=33;ticks=337;refresh=43;
   if(clamp){steps+=4;ticks+=61;refresh+=7;}
  }
 }
 if(budget<steps||(int32_t)(limit-(uint32_t)z->cyc)<(int32_t)ticks)return 0;
 uint16_t sp=z->sp,back=(uint16_t)(v->ram[sp-0xc000]|((unsigned)v->ram[sp-0xbfff]<<8));
 z->b=0;z->c=(uint8_t)(8*ch);z->h=(uint8_t)((0xc000+at)>>8);z->l=(uint8_t)at;
 z->a=0;set_f(z,0x54);
 if(active){
  uint16_t iy=(uint16_t)(0xc001+at);
  v->ram[sp-0xc002]=(uint8_t)iy;v->ram[sp-0xc001]=(uint8_t)(iy>>8);
  z->iy=iy;z->h=(uint8_t)(iy>>8);z->l=(uint8_t)iy;
  bool cy=false;
  for(unsigned off=4;off>=2;--off){
   z->b=v->ram[at+off+3];
   z->a=subtract?subb(z,v->ram[at+off],z->b,cy):addb(z,v->ram[at+off],z->b,cy);
   cy=z->cf;v->ram[at+off]=z->a;
  }
  if(!subtract){z->b=0x60;cp(z,0x60);}
  if(clamp){
   if(subtract){v->ram[at+1]=1;v->ram[at+2]=v->ram[at+3]=v->ram[at+4]=0;}
   else{v->ram[at+2]=0x5f;v->ram[at+3]=v->ram[at+4]=0xff;}
  }
 }
 z->pc=back;z->mem_ptr=back;z->sp=sp+2;z->cyc+=ticks;
 z->r=(uint8_t)((z->r&0x80)|((z->r+refresh)&127u));
 return steps;
}

/* Common channel entry: test music-active, decrement note countdown, branch. */
static WF_SMALL __attribute__((section(".wf_hotcode"))) unsigned wf_score_countdown(z80 *z,unsigned budget,uint32_t limit)
{
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;
 if(z->pc==0x08a7){
  unsigned ch=v->ram[0x65];
  if(ch>=8||budget<8||(int32_t)(limit-(uint32_t)z->cyc)<73||z->sp<0xc402||z->sp>0xc7fe)return 0;
  z->a=addb(z,(uint8_t)ch,(uint8_t)ch,0);z->b=0;z->c=z->a;
  z->h=0xc0;z->l=0xba;addhl(z,(uint16_t)(2*ch));
  z->a=v->ram[0xba+2*ch];
  z->cyc+=73;z->r=(uint8_t)((z->r&0x80)|((z->r+8)&127u));
  z->pc=0x08c1;z->mem_ptr=0x08c1;pushw(z,0x08b6);return 8;
 }
 if(z->pc!=0x080d)return 0;
 if(!v->ram[6]){
  if(budget<3||(int32_t)(limit-(uint32_t)z->cyc)<28||z->sp<0xc000||z->sp>0xc7fe)return 0;
  uint16_t sp=z->sp,back=(uint16_t)(v->ram[sp-0xc000]|((unsigned)v->ram[sp-0xbfff]<<8));
  z->a=0;set_f(z,0x54);z->sp=sp+2;z->pc=back;z->mem_ptr=back;z->cyc+=28;
  z->r=(uint8_t)((z->r&0x80)|((z->r+3)&127u));return 3;
 }
 unsigned ch=v->ram[0x65];if(ch>=8||budget<10||(int32_t)(limit-(uint32_t)z->cyc)<88)return 0;
 z->a=(uint8_t)ch;z->b=0;z->c=(uint8_t)ch;z->h=0xc0;z->l=(uint8_t)(0x66+ch);z->cf=0;
 uint8_t next=dec(z,v->ram[0x66+ch]);v->ram[0x66+ch]=next;
 z->pc=next?0x08a7:0x0820;z->mem_ptr=0x08a7;z->cyc+=88;
 z->r=(uint8_t)((z->r&0x80)|((z->r+10)&127u));return 10;
}


/* Exact 0F2D..0F4A unsigned 8x8 multiplication and RET. No I/O.
 * Complete admission precedes every CPU/RAM mutation. */
WF_SMALL __attribute__((section(".wf_hotcode")))
unsigned wf_score_multiply(z80 *z,unsigned budget,uint32_t limit)
{
 if(z->pc!=0x0f2d||z->nmi_pending||(z->int_pending&&z->iff1)||
    z->iff_delay||z->halted||z->sp<0xc004||z->sp>0xc7fe)return 0;
 unsigned pop=z->e;
 pop-=((pop>>1)&0x55u);
 pop=(pop&0x33u)+((pop>>2)&0x33u);
 pop=(pop+(pop>>4))&15u;
 unsigned steps=60u+pop,ticks=511u+6u*pop;
 if(budget<steps||(int32_t)(limit-(uint32_t)z->cyc)<(int32_t)ticks)return 0;
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;
 unsigned at=z->sp-0xc000u,product=(unsigned)z->b*z->e;
 uint16_t back=(uint16_t)(v->ram[at]|((unsigned)v->ram[at+1]<<8));
 v->ram[at-2]=get_f(z);v->ram[at-1]=z->a;
 v->ram[at-4]=z->l;v->ram[at-3]=z->h;
 z->b=(uint8_t)(product>>8);z->c=(uint8_t)product;z->e=0;
 z->sp+=2;z->pc=back;z->mem_ptr=back;z->cyc+=ticks;
 z->r=(uint8_t)((z->r&128u)|((z->r+86u+pop)&127u));
 return steps;
}

/* Isolated experiment. Exact pinned ED B0 at037C, never a generic LDIR.
 * Precondition: z belongs to the compatible wf_music_seq_t memory adapter.
 * Source10F0..123A is ROM; destinationC057..C1A1 is ordinary RAM.
 * 330 repeated21T iterations + final16T; no I/O or interrupt boundary. */
WF_SMALL __attribute__((section(".wf_hotcode")))
unsigned wf_z80_startup_copy(z80*z,unsigned budget,uint32_t limit)
{
 if(z->pc!=0x037c||budget<331u||(int32_t)(limit-(uint32_t)z->cyc)<6946||
    z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||z->halted||
    z->b!=1||z->c!=0x4b||z->h!=0x10||z->l!=0xf0||z->d!=0xc0||z->e!=0x57)return 0;
 wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;
 for(unsigned i=0;i<331u;i++)v->ram[0x57u+i]=v->rom[0x10f0u+i];
 const uint8_t sum=(uint8_t)(z->a+v->rom[0x123a]);
 z->xf=(sum>>3)&1u;z->yf=(sum>>1)&1u;z->hf=0;z->nf=0;z->pf=0;
 set_hl(z,0x123b);set_de(z,0xc1a2);set_bc(z,0);
 z->pc=0x037e;z->mem_ptr=0x037d;
 z->r=(uint8_t)((z->r&128u)|((z->r+662u)&127u));
 z->cyc=(uint32_t)((uint32_t)z->cyc+6946u);
 return 331u;
}

/* 0E6A DD56,5E,CALL0F4B plus the existing exact12-step YM writer.
 * Called only by the common fast dispatcher with its IRQ/PC admission.
 * All local admission precedes reads, stack stores and CPU mutation. */
WF_SMALL __attribute__((section(".wf_hotcode")))
unsigned wf_score_instrument_pre(z80*z,unsigned budget,uint32_t limit)
{
 wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;
 uint16_t hl=get_hl(z);
 if(budget<15u||(int32_t)(limit-(uint32_t)z->cyc)<124||
    z->sp<0xc002||z->sp>0xc800||z->ix>=0xc800||hl>=0xc800||(v->status&128u))return 0;
 uint8_t d=rb(z,z->ix),e=rb(z,hl);
 unsigned at=z->sp-0xc002u;
 z->d=d;z->e=e;
 v->ram[at]=0x71;v->ram[at+1]=0x0e;
 z->sp-=2;z->pc=0x0f4b;z->mem_ptr=0x0f4b;z->cyc+=43u;
 z->r=(uint8_t)((z->r&128u)|((z->r+4u)&127u));
 unsigned n=wf_cost_macro(z,budget-3u,limit);
 wf_hot_process_tail(z);
 return n+3u;
}

 WF_SMALL __attribute__((section(".wf_hotcode"))) unsigned wf_z80_control_fast_step(z80 *z,unsigned budget,uint32_t limit)
 {
  if(!budget||(int32_t)(limit-(uint32_t)z->cyc)<=0||
     z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||z->halted)return 0;
  switch(z->pc){
   case 0x0e6a:return wf_score_instrument_pre(z,budget,limit);
   /* Separate continuation: never crosses the preceding C801 callback. */
   case 0x0f5f:{
    if(budget<5u||z->sp<0xc000||z->sp>0xc7fe)return 0;
    bool taken=z->b!=1u;unsigned ticks=taken?43u:38u;
    if((int32_t)(limit-(uint32_t)z->cyc)<(int32_t)ticks)return 0;
    wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;unsigned at=z->sp-0xc000u;
    if(v->ram[at]!=0x71||v->ram[at+1]!=0x0e)return 0;
    uint8_t a=z->a,f=get_f(z);
    z->a=z->a_;set_f(z,z->f_);z->a_=a;z->f_=f;
    z->sp+=2;++z->ix;set_hl(z,(uint16_t)(get_hl(z)+1u));--z->b;
    z->pc=taken?0x0e6a:0x0e76;z->mem_ptr=taken?0x0e6a:0x0e71;
    z->cyc+=ticks;z->r=(uint8_t)((z->r&128u)|((z->r+6u)&127u));
    return 5;
   }

   case 0x037c:return wf_z80_startup_copy(z,budget,limit);
   case 0x0f2d:return wf_score_multiply(z,budget,limit);
   case 0x08c1:case 0x0f4b:return wf_cost_macro(z,budget,limit);
   case 0x0a8f:case 0x0aa0:return wf_score_pitchwrite(z,budget,limit);
   case 0x0991:case 0x0a0b:return wf_score_modupdate(z,budget,limit);
   case 0x080d:case 0x08a7:return wf_score_countdown(z,budget,limit);
   default:return 0;
  }
 }
 

 
bool wf_z80_native_compatible(const uint8_t *p,size_t n){
 if(!p||n<0xc000)return false;
 uint32_t crc=0xffffffffu;for(unsigned i=0;i<0xc000;i++){crc^=p[i];for(unsigned b=0;b<8;b++)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}
 return (crc^0xffffffffu)==0x6270a5c0u;
}


static const uint8_t wf_scalar_admission[256] __attribute__((section(".wf_hotdata_const")))={
 1,1,0,0,1,1,1,0,1,1,0,0,0,0,1,0,
 1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
 1,1,0,1,0,0,0,0,1,0,0,0,0,0,0,0,
 1,0,1,0,0,1,0,0,1,0,1,0,1,1,1,0,
 0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,
 0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,0,
 0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,
 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,
 0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,
 1,0,0,1,0,0,1,0,1,1,1,0,0,1,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
 0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
};

 WF_SMALL unsigned wf_z80_scalar_step(z80*z,unsigned budget,uint32_t limit)
 {
  if(!budget||(int32_t)(limit-(uint32_t)z->cyc)<=0||
     z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||z->halted)return 0;
  wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;uint16_t pc=z->pc;
  if(pc>=0xc000u)return 0;
  unsigned op=v->rom[pc];
  if(op==0xdd||op==0xfd){if(!wf_prefixed_fast(z))return 0;wf_hot_process_tail(z);return 1;}
  if(op==0xcb){
   if(pc>=0xbfffu)return 0;unsigned sub=v->rom[pc+1];
   if(sub!=0x19&&sub!=0x38&&sub!=0x23&&sub!=0x47&&sub!=0x7f)return 0;
   z->pc=(uint16_t)(pc+2);z->cyc+=8;inc_r(z);inc_r(z);
   switch(sub){
    case 0x19:z->c=cb_rr(z,z->c);break;
    case 0x38:z->b=cb_srl(z,z->b);break;
    case 0x23:z->e=cb_sla(z,z->e);break;
    case 0x47:cb_bit(z,z->a,0);break;
    case 0x7f:cb_bit(z,z->a,7);break;
   }
   wf_hot_process_tail(z);return 1;
  }
  if(!wf_scalar_admission[op])return 0;
  z->pc=(uint16_t)(pc+1);z->cyc+=cyc_00[op];inc_r(z);
  switch(op){
  /* Original scalar helpers; one instruction and the same timing/IRQ tail. */
  case 0x01: set_bc(z, nextw(z)); break;
  case 0x04: z->b = inc(z, z->b); break;
  case 0x0e: z->c = nextb(z); break;
  case 0x38: cond_jr(z, z->cf == 1); break;
  case 0x56: z->d = rb(z, get_hl(z)); break;
  case 0x82: z->a = addb(z, z->a, z->d, 0); break;
  case 0x90: z->a = subb(z, z->a, z->b, 0); break;
  case 0xba: cp(z, z->d); break;
  case 0xc6: z->a = addb(z, z->a, nextb(z), 0); break;

  case 0x00: break;
case 0x05: z->b = dec(z, z->b); break;
case 0x06: z->b = nextb(z); break;
case 0x08: {
    uint8_t a = z->a;
    uint8_t f = get_f(z);

    z->a = z->a_;
    set_f(z, z->f_);

    z->a_ = a;
    z->f_ = f;
  } break;
case 0x09: addhl(z, get_bc(z)); break;
case 0x10: cond_jr(z, --z->b != 0); break;
case 0x11: set_de(z, nextw(z)); break;
case 0x16: z->d = nextb(z); break;
case 0x20: cond_jr(z, z->zf == 0); break;
case 0x21: set_hl(z, nextw(z)); break;
case 0x23: set_hl(z, get_hl(z) + 1); break;
case 0x28: cond_jr(z, z->zf == 1); break;
case 0x30: cond_jr(z, z->cf == 0); break;
case 0x32: {
    const uint16_t addr = nextw(z);
    wb(z, addr, z->a);
    z->mem_ptr = (z->a << 8) | ((addr + 1) & 0xFF);
  } break;
case 0x35: {
    uint8_t result = dec(z, rb(z, get_hl(z)));
    wb(z, get_hl(z), result);
  } break;
case 0x3a: {
    const uint16_t addr = nextw(z);
    z->a = rb(z, addr);
    z->mem_ptr = addr + 1;
  } break;
case 0x3c: z->a = inc(z, z->a); break;
case 0x3d: z->a = dec(z, z->a); break;
case 0x3e: z->a = nextb(z); break;
case 0x47: z->b = z->a; break;
case 0x4f: z->c = z->a; break;
case 0x57: z->d = z->a; break;
case 0x5e: z->e = rb(z, get_hl(z)); break;
case 0x5f: z->e = z->a; break;
case 0x77: wb(z, get_hl(z), z->a); break;
case 0x7a: z->a = z->d; break;
case 0x7e: z->a = rb(z, get_hl(z)); break;
case 0x87: z->a = addb(z, z->a, z->a, 0); break;
case 0xa7: land(z, z->a); break;
case 0xaf: lxor(z, z->a); break;
case 0xb8: cp(z, z->b); break;
case 0xc0: cond_ret(z, z->zf == 0); break;
case 0xc3: jump(z, nextw(z)); break;
case 0xc8: cond_ret(z, z->zf == 1); break;
case 0xc9: ret(z); break;
case 0xca: cond_jump(z, z->zf == 1); break;
case 0xcd: call(z, nextw(z)); break;
case 0xe6: land(z, nextb(z)); break;
case 0xf1: {
    uint16_t val = popw(z);
    z->a = val >> 8;
    set_f(z, val & 0xFF);
  } break;
case 0xf5: pushw(z, (z->a << 8) | get_f(z)); break;
  }
  wf_hot_process_tail(z);return 1;
 }

/* Isolated compact dispatcher experiment. Existing fast/scalar routines retain
 * their own budget, signed cycle-limit and interrupt admission checks. Every
 * memory-mapped I/O callback increments io_epoch before observable effects.
 * Return after any such activity so seq_run recomputes timer control/limit.
 * Returning at 003A preserves the outer exact idle-loop skip opportunity. */
#if defined(__GNUC__)
#define WF_SCALAR_ALWAYS_INLINE static inline __attribute__((always_inline,optimize("Os")))
#elif defined(_MSC_VER)
#define WF_SCALAR_ALWAYS_INLINE static __forceinline
#else
#define WF_SCALAR_ALWAYS_INLINE static inline
#endif
WF_SCALAR_ALWAYS_INLINE unsigned wf_z80_scalar_inline(z80*z,unsigned budget,uint32_t limit)
 {
  if(!budget||(int32_t)(limit-(uint32_t)z->cyc)<=0||
     z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||z->halted)return 0;
  wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;uint16_t pc=z->pc;
  if(pc>=0xc000u)return 0;
  unsigned op=v->rom[pc];
  if(op==0xdd||op==0xfd){if(!wf_prefixed_fast(z))return 0;wf_hot_process_tail(z);return 1;}
  if(op==0xcb){
   if(pc>=0xbfffu)return 0;unsigned sub=v->rom[pc+1];
   if(sub!=0x19&&sub!=0x38&&sub!=0x23&&sub!=0x47&&sub!=0x7f)return 0;
   z->pc=(uint16_t)(pc+2);z->cyc+=8;inc_r(z);inc_r(z);
   switch(sub){
    case 0x19:z->c=cb_rr(z,z->c);break;
    case 0x38:z->b=cb_srl(z,z->b);break;
    case 0x23:z->e=cb_sla(z,z->e);break;
    case 0x47:cb_bit(z,z->a,0);break;
    case 0x7f:cb_bit(z,z->a,7);break;
   }
   wf_hot_process_tail(z);return 1;
  }
  if(!wf_scalar_admission[op])return 0;
  z->pc=(uint16_t)(pc+1);z->cyc+=cyc_00[op];inc_r(z);
  switch(op){
  /* Original scalar helpers; one instruction and the same timing/IRQ tail. */
  case 0x01: set_bc(z, nextw(z)); break;
  case 0x04: z->b = inc(z, z->b); break;
  case 0x0e: z->c = nextb(z); break;
  case 0x38: cond_jr(z, z->cf == 1); break;
  case 0x56: z->d = rb(z, get_hl(z)); break;
  case 0x82: z->a = addb(z, z->a, z->d, 0); break;
  case 0x90: z->a = subb(z, z->a, z->b, 0); break;
  case 0xba: cp(z, z->d); break;
  case 0xc6: z->a = addb(z, z->a, nextb(z), 0); break;

  case 0x00: break;
case 0x05: z->b = dec(z, z->b); break;
case 0x06: z->b = nextb(z); break;
case 0x08: {
    uint8_t a = z->a;
    uint8_t f = get_f(z);

    z->a = z->a_;
    set_f(z, z->f_);

    z->a_ = a;
    z->f_ = f;
  } break;
case 0x09: addhl(z, get_bc(z)); break;
case 0x10: cond_jr(z, --z->b != 0); break;
case 0x11: set_de(z, nextw(z)); break;
case 0x16: z->d = nextb(z); break;
case 0x20: cond_jr(z, z->zf == 0); break;
case 0x21: set_hl(z, nextw(z)); break;
case 0x23: set_hl(z, get_hl(z) + 1); break;
case 0x28: cond_jr(z, z->zf == 1); break;
case 0x30: cond_jr(z, z->cf == 0); break;
case 0x32: {
    const uint16_t addr = nextw(z);
    wb(z, addr, z->a);
    z->mem_ptr = (z->a << 8) | ((addr + 1) & 0xFF);
  } break;
case 0x35: {
    uint8_t result = dec(z, rb(z, get_hl(z)));
    wb(z, get_hl(z), result);
  } break;
case 0x3a: {
    const uint16_t addr = nextw(z);
    z->a = rb(z, addr);
    z->mem_ptr = addr + 1;
  } break;
case 0x3c: z->a = inc(z, z->a); break;
case 0x3d: z->a = dec(z, z->a); break;
case 0x3e: z->a = nextb(z); break;
case 0x47: z->b = z->a; break;
case 0x4f: z->c = z->a; break;
case 0x57: z->d = z->a; break;
case 0x5e: z->e = rb(z, get_hl(z)); break;
case 0x5f: z->e = z->a; break;
case 0x77: wb(z, get_hl(z), z->a); break;
case 0x7a: z->a = z->d; break;
case 0x7e: z->a = rb(z, get_hl(z)); break;
case 0x87: z->a = addb(z, z->a, z->a, 0); break;
case 0xa7: land(z, z->a); break;
case 0xaf: lxor(z, z->a); break;
case 0xb8: cp(z, z->b); break;
case 0xc0: cond_ret(z, z->zf == 0); break;
case 0xc3: jump(z, nextw(z)); break;
case 0xc8: cond_ret(z, z->zf == 1); break;
case 0xc9: ret(z); break;
case 0xca: cond_jump(z, z->zf == 1); break;
case 0xcd: call(z, nextw(z)); break;
case 0xe6: land(z, nextb(z)); break;
case 0xf1: {
    uint16_t val = popw(z);
    z->a = val >> 8;
    set_f(z, val & 0xFF);
  } break;
case 0xf5: pushw(z, (z->a << 8) | get_f(z)); break;
  }
  wf_hot_process_tail(z);return 1;
 }
#undef WF_SCALAR_ALWAYS_INLINE

WF_SMALL __attribute__((section(".wf_hotcode")))
unsigned wf_z80_control_batch(z80 *z,unsigned budget,uint32_t limit)
{
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;unsigned total=0;
 while(!v->error){
  uint32_t io=v->io_epoch;
  unsigned n=0;
  switch(z->pc){case 0x0e6a:case 0x0f5f:case 0x037c:case 0x0f2d:case 0x08c1:case 0x0f4b:case 0x0a8f:case 0x0aa0:case 0x0991:case 0x0a0b:case 0x080d:case 0x08a7:
   n=wf_z80_control_fast_step(z,budget,limit);break;
  default:break;
  }
  if(!n)n=wf_z80_scalar_inline(z,budget,limit);
  if(!n)break;
  total+=n;budget-=n;
  if(z->pc==0x003a||v->io_epoch!=io)break;
 }
 return total;
}
