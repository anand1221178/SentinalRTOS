#include "os_kernel.h"
#include "tasks.h"
#include "lock.h"

/* Arr holding the tasks -> Physical storage in RAM for all our TASK CONTROL BLOCKS*/
TCB_t os_tasks[MAX_TASKS];
/* Current task count in th TCB */
static uint8_t task_count = 0;

/* Global pointer for current TCB  */
TCB_t *current_tcb = NULL;

/* 
taskPtr  Address of the function the task should execute.
stackTop Pointer to the highest address of the allocated stack array.
*/
uint32_t* os_task_init_stack(void (*taskptr)(void), uint32_t *stackTop)
{
    uint32_t *stk = stackTop;

    *(--stk) = 0x01000000; /* xPSR with bit 24 set */
    *(--stk) = (uint32_t)taskptr; /* Task ptr set */
    *(--stk) = 0xDEADBEEF; /* LR with dummy value */
    *(--stk) = 0x12121212; /* R12 */
    *(--stk) = 0x03030303; /* R3 */
    *(--stk) = 0x02020202; /* R2 */
    *(--stk) = 0x01010101; /* R1 */
    *(--stk) = 0x00000000; /* R0 */

    for (int i = 0 ; i < 8; i++)
    {
        *(--stk) = 0x00000000; /* R4 through R11 */
    }

    return stk; 
}

bool os_task_create(void (*taskptr)(void), uint32_t *stackLimit, uint8_t priority)
{
    if(task_count >= MAX_TASKS) return false;

    TCB_t *new_tcb = &os_tasks[task_count];

    new_tcb->base_priority = priority;
    new_tcb->current_priority = priority;
    new_tcb->state = READY;

    uint32_t *stackTop = stackLimit + 256;
    new_tcb->stackPtr = os_task_init_stack(taskptr, stackTop);

    if (task_count == 0)
    {
        new_tcb->next = (struct TCB*)new_tcb;
        current_tcb = new_tcb;
    }
    else{
        new_tcb->next = current_tcb->next;
        current_tcb->next = (struct TCB*)new_tcb;
    }

    task_count++;
    return true;
}

void os_scheduler(void)
{
    TCB_t* best_task = NULL; 
    uint8_t highest_prior = 0;

    for (int i = 0; i < task_count; i++)
    {
        if(os_tasks[i].state == READY)
        {
            if (best_task == NULL || os_tasks[i].current_priority >= highest_prior)
            {
                highest_prior = os_tasks[i].current_priority;
                best_task = &os_tasks[i];
            }
        }
    }

    if(best_task != NULL) {
        current_tcb = best_task;
    }
}

void SysTick_Handler(void) {
    uint8_t switch_needed = 0;

    for (int i = 0; i < task_count; i++) {
        if (os_tasks[i].sleep_time > 0) {
            os_tasks[i].sleep_time--;
            
            if (os_tasks[i].sleep_time == 0) {
                os_tasks[i].state = READY;
                if (os_tasks[i].current_priority > current_tcb->current_priority) {
                    switch_needed = 1;
                }
            }
        }
    }

    if (switch_needed) {
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; 
    }
}

void os_delay(uint32_t ms)
{
    __disable_irq(); 
    current_tcb->sleep_time = ms;
    current_tcb->state = BLOCKED;
    __enable_irq(); 

    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void os_kernel_init(void)
{
    __disable_irq(); 
    
    /* Enable Debug during Sleep/Stop/Standby modes */
    /* DBGMCU_CR at 0xE0042008: Set bits 0, 1, 2 */
    *((volatile uint32_t *)0xE0042008) |= 0x07;

    task_count = 0; 
    current_tcb = NULL; 
}

void os_kernel_launch(void)
{
    SysTick->LOAD = 15999;
    SysTick->VAL = 0;
    SysTick->CTRL = SYSTICK_CTRL_CONFIG;
    SCB->SHP[10] = 0xFF;
    os_start_first_task(); 
}

uint8_t os_mutex_acquire(os_mutex_t* mutex, uint32_t timeout)
{
    __disable_irq();

    if(mutex->lock == 0)
    {
        mutex->lock = 1;
        mutex->owner = current_tcb;
        __enable_irq();
        return OS_SUCCESS;
    }
    else
    {
        if (timeout == 0) {
            __enable_irq();
            return OS_TIMEOUT;
        }

        if(current_tcb->current_priority > mutex->owner->current_priority)
        {
            mutex->owner->current_priority = current_tcb->current_priority;
        }

        current_tcb->state = BLOCKED;
        mutex->wait_queue[mutex->wait_count] = current_tcb;
        mutex->wait_count += 1;
        __enable_irq();

        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        return OS_SUCCESS;
    }
}

void os_mutex_release(os_mutex_t* mutex)
{
    __disable_irq();
    current_tcb->current_priority = current_tcb->base_priority;

    if(mutex->wait_count > 0)
    {
        mutex->owner = mutex->wait_queue[0];
        mutex->owner->state = READY;

        for (int i = 0; i < mutex->wait_count - 1; i++)
        {
            mutex->wait_queue[i] = mutex->wait_queue[i+1];
        }
        mutex->wait_count--;

        __enable_irq();
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    else
    {
        mutex->lock = 0;
        mutex->owner = NULL;
        __enable_irq();
    }
}

uint8_t os_semaphore_acquire(os_semaphore_t* sem, uint32_t timeout)
{
    __disable_irq();

    if(sem->count > 0)
    {
        sem->count--;
        __enable_irq();
        return OS_SUCCESS;
    }
    else
    {
        if (timeout == 0) {
            __enable_irq();
            return OS_TIMEOUT;
        }

        current_tcb->state = BLOCKED;
        sem->wait_queue[sem->wait_count] = current_tcb;
        sem->wait_count++;
        
        __enable_irq();
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        
        return OS_SUCCESS;
    }
}

void os_semaphore_release(os_semaphore_t* sem)
{
    __disable_irq();

    if (sem->wait_count > 0)
    {
        TCB_t* next_task = sem->wait_queue[0];
        next_task->state = READY;

        for (int i = 0; i < sem->wait_count - 1; i++) 
        {
            sem->wait_queue[i] = sem->wait_queue[i + 1];
        }
        sem->wait_count--;

        __enable_irq();
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    else
    {
        if (sem->count < sem->max_count)
        {
            sem->count++;
        }
        __enable_irq();
    }
}
