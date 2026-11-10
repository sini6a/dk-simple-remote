#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H
#include <stdint.h>
#include <stddef.h>
#define SYS_EVENT_MSG 0x8000
#define INVALID_TASK_ID 255
#define MS1_TO_SYSTEM_TIME(ms) ((ms)*8/5)
typedef unsigned gapRole_States_t;
typedef struct { unsigned ignored; } gapRoleEvent_t;
uint8_t TMOS_ProcessEventRegister(uint16_t (*process)(uint8_t,uint16_t));
uint32_t TMOS_GetSystemClock(void);
void tmos_set_event(uint8_t task,uint16_t event);
void tmos_start_task(uint8_t task,uint16_t event,uint32_t ticks);
void tmos_stop_task(uint8_t task,uint16_t event);
uint8_t *tmos_msg_receive(uint8_t task);
void tmos_msg_deallocate(uint8_t *msg);
#endif
