#ifndef WF_LOAD_WINDOW_H
#define WF_LOAD_WINDOW_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* A synchronous, single-file startup lease. The caller owns the aligned
 * scratch and must keep it disjoint from every read destination. Initialize
 * the state to zero, open only while the scratch is idle, then close BEFORE
 * releasing it. There is no allocation or retained data after close.
 * Source failures may leave a partial caller destination, as with CDFS;
 * the failed refill is never published and the original error is returned. */
enum {WF_LOAD_WINDOW_BYTES=32768u,WF_LOAD_WINDOW_NAME_BYTES=13u,
      WF_LOAD_WINDOW_ARGUMENT=-931,WF_LOAD_WINDOW_STATE=-932};
typedef int (*wf_load_window_read_fn)(const char *,uint32_t,void *,size_t);
typedef struct {
 uint8_t *buffer;
 wf_load_window_read_fn source;
 uint32_t file_bytes,first,valid;
 unsigned active,busy;
 char name[WF_LOAD_WINDOW_NAME_BYTES];
} wf_load_window_t;

static inline uintptr_t wf_load_window_physical(uintptr_t p){
#if defined(__sh__) || defined(WF_PLATFORM_SATURN)
 return p&0x1fffffffu;
#else
 return p;
#endif
}
static inline int wf_load_window_span(const void *p,size_t bytes){
 uintptr_t a=(uintptr_t)p;
 if(bytes&&bytes-1u>UINTPTR_MAX-a)return 0;
#if defined(__sh__) || defined(WF_PLATFORM_SATURN)
 if(bytes>0x20000000u-wf_load_window_physical(a))return 0;
#endif
 return 1;
}
static inline int wf_load_window_overlap(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=wf_load_window_physical((uintptr_t)a),y=wf_load_window_physical((uintptr_t)b);
 if(!an||!bn)return 0;
 return x<=y?y-x<an:x-y<bn;
}
static inline void wf_load_window_close(wf_load_window_t *w){
 if(!w)return;
 /* A source callback may revoke the lease. Preserve the recursion fence
  * until that callback unwinds, so it cannot reopen a different lease. */
 unsigned busy=w->busy;
 memset(w,0,sizeof *w);w->busy=busy;
}
static inline int wf_load_window_open(wf_load_window_t *w,void *buffer,size_t capacity,
 const char *name,uint32_t file_bytes,wf_load_window_read_fn source){
 if(!w||!buffer||!name||!source||capacity!=WF_LOAD_WINDOW_BYTES||
    ((uintptr_t)buffer&31u)||!wf_load_window_span(w,sizeof *w)||
    !wf_load_window_span(buffer,capacity)||!wf_load_window_span(name,WF_LOAD_WINDOW_NAME_BYTES)||
    wf_load_window_overlap(w,sizeof *w,buffer,capacity))return WF_LOAD_WINDOW_ARGUMENT;
 if(w->active||w->busy)return WF_LOAD_WINDOW_STATE;
 char saved[WF_LOAD_WINDOW_NAME_BYTES];unsigned n=0;
 while(n<sizeof saved&&name[n]){saved[n]=name[n];++n;}
 if(!n||n==sizeof saved)return WF_LOAD_WINDOW_ARGUMENT;
 saved[n]=0;
 memset(w,0,sizeof *w);memcpy(w->name,saved,n+1u);
 w->buffer=buffer;w->source=source;w->file_bytes=file_bytes;w->active=1;
 return 0;
}
static inline int wf_load_window_read(wf_load_window_t *w,const char *name,
 uint32_t offset,void *destination,size_t bytes){
 if(!w||!wf_load_window_span(w,sizeof *w))return WF_LOAD_WINDOW_ARGUMENT;
 if(!w->active||w->busy)return WF_LOAD_WINDOW_STATE;
 if(!name||!wf_load_window_span(name,WF_LOAD_WINDOW_NAME_BYTES)||
    strncmp(name,w->name,WF_LOAD_WINDOW_NAME_BYTES)||offset>w->file_bytes||
    bytes>w->file_bytes-offset||(!destination&&bytes)||
    !wf_load_window_span(destination,bytes)||
    wf_load_window_overlap(destination,bytes,w,sizeof *w)||
    wf_load_window_overlap(destination,bytes,w->buffer,WF_LOAD_WINDOW_BYTES)){
  w->valid=0;return WF_LOAD_WINDOW_ARGUMENT;
 }
 w->busy=1;uint8_t *out=destination;int rc=0;
 while(bytes){
  if(!w->valid||offset<w->first||offset-w->first>=w->valid){
   uint32_t first=offset&~(uint32_t)(WF_LOAD_WINDOW_BYTES-1u);
   size_t valid=w->file_bytes-first;if(valid>WF_LOAD_WINDOW_BYTES)valid=WF_LOAD_WINDOW_BYTES;
   w->valid=0;
   rc=w->source(w->name,first,w->buffer,valid);
   if(rc)break;
   if(!w->active){rc=WF_LOAD_WINDOW_STATE;break;}
   w->first=first;w->valid=(uint32_t)valid;
  }
  size_t within=offset-w->first,take=w->valid-within;
  if(take>bytes)take=bytes;
  memcpy(out,w->buffer+within,take);
  out+=take;offset+=(uint32_t)take;bytes-=take;
 }
 if(rc)w->valid=0;
 w->busy=0;return rc;
}
#endif
