#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_present.h"

static uint8_t rom[524288], tile_cache[WF_SATURN_LWRAM_TILE_BYTES];
static FILE *pack, *hot;
static unsigned sprite_reads, corrupt, initial_reads, extension_reads;
static uint32_t payload_offset, expected_header;
static unsigned id_offset;
static unsigned be16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int in_hot_table(const uint8_t *header,unsigned offset,unsigned count,unsigned tile){
    unsigned lo=0,hi=count;
    while(lo<hi){unsigned mid=lo+(hi-lo)/2u;
        if(be16(header+offset+mid*2u)<tile)lo=mid+1u;else hi=mid;}
    return lo<count&&be16(header+offset+lo*2u)==tile;
}
static int read_file(const char *name,uint32_t offset,void *dst,size_t bytes)
{
    FILE *file;
    if (!strcmp(name,"SPRITES.PAK")) {file=pack;++sprite_reads;}
    else if (!strcmp(name,"HOTSPR.BIN")) file=hot;
    else return -1;
    if(file==hot&&!corrupt){
        if(!offset){if(bytes!=4096u)return -1;++initial_reads;}
        else if(offset<expected_header)++extension_reads;
        else payload_offset=offset;
    }
    if(fseek(file,(long)offset,SEEK_SET)||fread(dst,1,bytes,file)!=bytes)return -1;
    if(file==hot&&!offset){
        uint8_t *p=dst;
        if(corrupt==1u)p[0]=0;
        if(corrupt==2u){p[id_offset+2]=p[id_offset];p[id_offset+3]=p[id_offset+1];}
        if(corrupt==3u)p[7]=127u;
        if(corrupt==4u){p[4]=255u;p[5]=255u;}
        if(corrupt==6u)memset(p+8,0,4);
        if(corrupt==7u)memset(p+8,255,4);
        if(corrupt==8u)p[15]^=1;
        if(corrupt==9u)p[19]^=1;
        if(corrupt==10u)p[23]^=1;
    }
    if(file==hot&&offset==expected_header&&corrupt==11u){uint8_t *p=dst;p[0]=p[1]=255u;}
    return file==hot&&offset&&corrupt==5u?-1:0;
}
int main(int argc,char **argv)
{
    uint8_t header[16384],expected[128];const uint8_t *got;
    unsigned count,cold=0,dictionary;size_t payload_bytes,resident_slots;
    size_t tile_capacity=sizeof tile_cache;
    if(wf_saturn_memory_contract_validate()!=WF_OK){fputs("Invalid native memory partition\n",stderr);return 1;}
    if((argc!=3&&argc!=4)||!(pack=fopen(argv[1],"rb"))||!(hot=fopen(argv[2],"rb")))return 2;
    if(argc==4&&!strcmp(argv[3],"--front-lease"))tile_capacity=WF_SATURN_LWRAM_FRONT_TILE_BYTES;
    else if(argc==4){
        if(strcmp(argv[3],"--reject"))return 2;
        wf_r18_present_init(read_file,rom,sizeof rom,tile_cache,tile_capacity);
        if(wf_r18_hot_sprites_load()!=WF_BLOCKED||wf_r18_hot_sprites_bytes()){
            fputs("Invalid pack accepted\n",stderr);return 1;}
        puts("PASS invalid pack rejected without active payload");return 0;
    }
    if(fread(header,1,4096u,hot)!=4096u)return 2;
    dictionary=!memcmp(header,"R18D",4);id_offset=dictionary?24u:8u;
    count=be16(header+4);
    expected_header=id_offset+count*2u>8192u?16384u:id_offset+count*2u>4096u?8192u:4096u;
    if(expected_header>4096u&&fread(header+4096u,1,expected_header-4096u,hot)!=expected_header-4096u)return 2;
    payload_bytes=dictionary?((be32(header+12)+31u)&~31u)+(size_t)be32(header+16):(size_t)count*128u;
    size_t mapping=dictionary?((be32(header+12)+31u)&~31u):0u;
    size_t spill=mapping>WF_SATURN_HWRAM_HOT_INDEX_BYTES?mapping-WF_SATURN_HWRAM_HOT_INDEX_BYTES:0u;
    resident_slots=((dictionary?spill+(size_t)be32(header+16):payload_bytes)+127u)/128u;
    wf_r18_present_init(read_file,rom,sizeof rom,tile_cache,tile_capacity);
    if(wf_r18_hot_sprites_load()!=WF_OK||!count){fputs("Hot sprite pack failed to load\n",stderr);return 1;}
    uint32_t last_payload=dictionary?expected_header+((be32(header+12)+31u)&~31u):expected_header;
    if(initial_reads!=1u||extension_reads!=(expected_header>4096u)||payload_offset!=last_payload||
       wf_r18_hot_sprites_bytes()!=expected_header+payload_bytes){fputs("Header/payload offset or reported size differs\n",stderr);return 1;}
    if(fseek(hot,0,SEEK_END)||(size_t)ftell(hot)!=wf_r18_hot_sprites_bytes()){fputs("Reported size differs from complete file\n",stderr);return 1;}
    for(unsigned i=0;i<count;i++){
        unsigned tile=be16(header+id_offset+i*2u);
        if(tile==cold)++cold;
        if(fseek(pack,(long)tile*128,SEEK_SET)||fread(expected,1,128,pack)!=128||
           wf_r18_tile_read((uint16_t)tile,&got)!=WF_OK||memcmp(expected,got,128)||sprite_reads||
           (dictionary&&((uintptr_t)got&31u))){
            fprintf(stderr,"Hot tile %u differs or caused a pack read\n",tile);return 1;}
    }
    if(fseek(pack,(long)cold*128,SEEK_SET)||fread(expected,1,128,pack)!=128||
       wf_r18_tile_read((uint16_t)cold,&got)!=WF_OK||memcmp(expected,got,128)||sprite_reads!=1u||
       wf_r18_tile_read((uint16_t)cold,&got)!=WF_OK||memcmp(expected,got,128)||sprite_reads!=1u){
        fputs("Cold tile fallback differs or rereads the pack\n",stderr);return 1;}
    /* Two distinct cold tiles with the same old modulo slot must coexist
     * while the cache has hundreds of otherwise unused slots. */
    size_t cold_slots=tile_capacity/128u-resident_slots;
    if(cold_slots>1u){
        unsigned collision=0;
        for(size_t candidate=(size_t)cold+cold_slots;candidate<65536u;candidate+=cold_slots)
            if(!in_hot_table(header,id_offset,count,(unsigned)candidate)){
                collision=(unsigned)candidate;break;}
        if(!collision||wf_r18_tile_read((uint16_t)collision,&got)!=WF_OK||sprite_reads!=2u||
           wf_r18_tile_read((uint16_t)cold,&got)!=WF_OK||sprite_reads!=2u){
            fputs("Cold cache evicted a live tile despite vacant slots\n",stderr);return 1;}
    }
    for(corrupt=1u;corrupt<=(dictionary?11u:5u);++corrupt){
        if(corrupt==2u&&count==1u)continue;
        if(corrupt==11u&&be32(header+8)==65536u)continue;
        wf_r18_present_init(read_file,rom,sizeof rom,tile_cache,tile_capacity);
        if(wf_r18_hot_sprites_load()!=WF_BLOCKED||wf_r18_hot_sprites_bytes()){
            fprintf(stderr,"Malformed hot pack %u accepted\n",corrupt);return 1;}
    }
    corrupt=0;
    for(size_t slots=resident_slots-1u;slots<=resident_slots;slots++){
        wf_r18_present_init(read_file,rom,sizeof rom,tile_cache,slots*128u);
        if(wf_r18_hot_sprites_load()!=WF_BLOCKED||wf_r18_hot_sprites_bytes()){
            fputs("Insufficient tile buffer accepted\n",stderr);return 1;}
    }
    wf_r18_present_init(read_file,rom,sizeof rom,tile_cache,(resident_slots+1u)*128u);
    if(wf_r18_hot_sprites_load()!=WF_OK){fputs("Payload plus one fallback slot rejected\n",stderr);return 1;}
    corrupt=5;
    if(wf_r18_hot_sprites_load()!=WF_BLOCKED||wf_r18_hot_sprites_bytes()){
        fputs("Failed reload retained an active hot table\n",stderr);return 1;}
    printf("PASS %s %u logical tiles, %zu file payload bytes: exact bytes, zero hot CD reads, cold fallback, malformed rejection\n",dictionary?"R18D":"R18H",count,payload_bytes);
    return 0;
}
