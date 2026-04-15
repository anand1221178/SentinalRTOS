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

/* --- Public Kernel API --- */

/**
 * @brief Initializes the Kernel hardware (SysTick) and TCB structures.
 */
void os_kernel_init(void);

/**
 * @brief Launches the first task. This function never returns.
 * Written in Assembly (os_kernel_asm.s).
 */
void os_kernel_launch(void);

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

#endif /* OS_KERNEL_H */