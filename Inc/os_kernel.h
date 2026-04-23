#ifndef OS_KERNEL_H
#define OS_KERNEL_H

#include <stdlib.h>
#include <stdbool.h>
#include "os_task.h"
#include "stm32f4xx.h"

/* --- Configuration --- */
#define MAX_TASKS    6      /* Maximum tasks the kernel can manage */
#define STACK_SIZE   256    /* Stack size in 32-bit words (1024 bytes) */
#define SYSTICK_FREQ 1000   /* 1kHz Tick (1ms) */

#define ST_ENABLE (1U<<0) /* Turn on counter */
#define ST_TICKINT (1U<<1) /* Enable systick interuppt */
#define ST_CLKSOURCE (1U<<2) /* Use the proc clock */

#define SYSTICK_CTRL_CONFIG  (ST_ENABLE | ST_TICKINT | ST_CLKSOURCE)


/* --- Synchronization Primitives Definitions --- */

#define MAX_WAITING_TASKS 5

typedef struct 
{
    uint32_t lock; /* 0 = unlocked 1 = locked */
    TCB_t* owner; /* The task currently holding the lock */
    TCB_t* wait_queue[MAX_WAITING_TASKS]; /* Array for the waiting tasks for the lock */
    uint8_t wait_count; /* How many tasks are currently waiting? */
} os_mutex_t;

typedef struct
{
    uint32_t count; /* Current number of avaliable tokens */
    uint32_t max_count; /* 1 for Binary, >1 for Counting */
    TCB_t* wait_queue[MAX_TASKS]; 
    uint8_t wait_count;
} os_semaphore_t;


/* --- Public Kernel API --- */

/**
 * @brief Initializes the Kernel hardware (SysTick) and TCB structures.
 */
void os_kernel_init(void);

/**
 * @brief Launches the first task. This function never returns.
 * Implementation in os_kernel.c, calls os_start_first_task.
 */
void os_kernel_launch(void);

/**
 * @brief Actual assembly implementation to switch to PSP and start tasks.
 * Defined in os_kernel_asm.s.
 */
void os_start_first_task(void);

/**
 * @brief Context switch handler. Performs saving and restoring of task context.
 * Defined in os_kernel_asm.s.
 */
void PendSV_Handler(void);

/**
 * @brief Registers a new task into the circular linked list.
 * @param taskptr    Function pointer to the task code.
 * @param stackLimit Pointer to the START of the static stack array.
 * @param priority   Task priority level.
 * @return true if successful, false if MAX_TASKS reached.
 */
bool os_task_create(void (*taskptr)(void), uint32_t *stackLimit, uint8_t priority);

/**
 * @brief The Round-Robin decision maker. Called by SysTick_Handler.
 */
void os_scheduler(void);

/* Mutex API */
void os_mutex_acquire(os_mutex_t* mutex);
void os_mutex_release(os_mutex_t* mutex);

/* Semaphore API */
void os_semaphore_acquire(os_semaphore_t* sem);
void os_semaphore_release(os_semaphore_t* sem);

void os_delay(uint32_t ms);

/**
 * @brief Runs Power-On Self-Tests for Kernel Primitives.
 * Should be called after UART init but before os_kernel_launch.
 */
void os_run_post(void);

#endif /* OS_KERNEL_H */
