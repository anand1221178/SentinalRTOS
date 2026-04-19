#ifndef OS_TASK_H
#define OS_TASK_H

#include <stdint.h>

/* Task states */
typedef enum
{
    READY,
    RUNNING,
    BLOCKED,
    SUSPENDED
} TaskState_t;

/* Task Control Block (TCB) */
typedef struct TCB
{
    /* data */
    uint32_t *stackPtr; /* Current stack ptr (has to be first since required in the assembly) */
    TaskState_t state; /* Current state of the task */
    uint8_t priority; /* Task priority (0 being highest) */
    uint32_t stackSize; /* Size of the stack in words */
    uint32_t timeout; /* USe for os-delay */
    struct TCB *next; /* ptr to the next task in the circle */
    uint32_t sleep_time; /* Time remaining in sleep (ms) */
} TCB_t;

#endif
