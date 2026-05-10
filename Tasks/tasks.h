#ifndef TASKS_H
#define TASKS_H

#include "os_task.h"
#include "os_kernel.h"
#include "os_queue.h"
#include "stm32f4xx.h"

#define TRIG_PIN 0  /* Example: PA0 */
/* In tasks.h */
extern os_semaphore_t echo_ready;
extern volatile uint32_t time_start;
extern volatile uint32_t time_end;

/* Extern declarations for shared resources defined in main.c */
extern uint32_t servo_stack[256];
extern os_message_queue_t sweep_queue;
extern uint32_t sweep_queue_buffer[8];

void os_idle_task(void);
void sweep_task(void);
void radar_task(void);

#endif
