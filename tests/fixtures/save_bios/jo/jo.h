#ifndef TEST_JO_JO_H
#define TEST_JO_JO_H
#include <stdint.h>
typedef struct {unsigned year,month,day,hour,minute,week;} jo_datetime;
void jo_core_disable_reset(void);
void jo_core_enable_reset(void);
void jo_getdate(jo_datetime *);
#endif
