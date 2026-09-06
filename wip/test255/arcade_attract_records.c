#include "arcade_attract_records.h"
#define R(v,a,b,c,x,y) {(v),{(a),(b),(c)},(x),(y)}
const arcade_record_entry_t arcade_default_tag_records[ARCADE_RECORD_ROWS]={
 R(8,'T','A','K',0,1),R(6,'H','A','G',2,3),R(5,'H','A','N',6,7),R(5,'S','I','N',8,9),
 R(4,'T','A','T',2,3),R(4,'K','I','N',0,1),R(3,'N','A','R',2,3),R(3,'M','A','E',6,7),
 R(2,'S','A','K',8,9),R(2,'I','S','I',2,3),R(1,'T','O','M',0,1),
};
const arcade_record_entry_t arcade_default_royal_records[ARCADE_RECORD_ROWS]={
 R(9,'T','A','K',0,1),R(7,'H','A','G',2,3),R(6,'H','A','N',6,7),R(5,'S','I','N',8,9),
 R(4,'T','A','T',2,3),R(4,'K','I','N',0,1),R(3,'N','A','R',2,3),R(3,'M','A','E',6,7),
 R(2,'S','A','K',8,9),R(2,'I','S','I',2,3),R(1,'T','O','M',0,1),
};
#undef R
const arcade_record_table_t arcade_record_tables[2]={
 {arcade_default_tag_records,0x00c2,0x0100,0},
 {arcade_default_royal_records,0x0104,0x0100,8},
};
const uint16_t arcade_record_name_offsets[ARCADE_RECORD_ROWS]={0x043c,0x083c,0x0c3c,0x0f3c,0x113c,0x133c,0x153c,0x173c,0x193c,0x1b3c,0x1d3c};
const uint16_t arcade_record_metric_offsets[ARCADE_RECORD_ROWS]={0x0570,0x0970,0x0d70,0x0f5c,0x115c,0x135c,0x155c,0x175c,0x195c,0x1b5c,0x1d5c};
