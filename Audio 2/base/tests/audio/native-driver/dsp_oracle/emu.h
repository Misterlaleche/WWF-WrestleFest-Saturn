#pragma once
#include <cstdint>
#include <cstring>
#include <iterator>
#include <algorithm>
using u16=uint16_t;using u32=uint32_t;using s16=int16_t;using s32=int32_t;using s64=int64_t;
#define BIT(x,n) (((x)>>(n))&1)
namespace util {inline s32 sext(s32 x,unsigned bits){u32 mask=1u<<(bits-1);return (s32)((((u32)x&((1u<<bits)-1u))^mask)-mask);}}
struct address_space {u16 words[0x40000];u16 (*read)(u32)=nullptr;void (*write)(u32,u16)=nullptr;u16 read_word(u32 a){return read?read(a):words[(a>>1)&0x3ffff];}void write_word(u32 a,u16 v){if(write)write(a,v);else words[(a>>1)&0x3ffff]=v;}};
