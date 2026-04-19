/* STM32 has 32 bit wide regs -> each increment or decreament will move exactly one reg width in mem*/
#include "os_kernel.h"
#include "tasks.h"
#include "lock.h"
/* Max number of tasks */


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
    /* Point tmp ptr to top of stack */
    /* We are using a full descending stack, so we start at the end */
    uint32_t *stk = stackTop;

    /*Hardware stack frame -> decrement and fill*/
    /* *(--stk) moves the ptr 4 bytes down so perfcet */
    *(--stk) = 0x01000000; /* xPSR with bit 24 set */
    *(--stk) = (uint32_t)taskptr; /* Task ptr set */
    *(--stk) = 0xDEADBEEF; /* LR with dummy value so we can spot it */
    *(--stk) = 0x12121212; /* R12 -> scratch reg with dummy value */
    *(--stk) = 0x03030303; /* R3 -> scrath reg with dummy value */
    *(--stk) = 0x02020202; /* R2 -> scratch reg with dummy value */
    *(--stk) = 0x01010101; /* R1 -> scratch reg with dummy value */
    *(--stk) = 0x00000000; /* R0 */

    /* Now need to assig the software frame -> will use a for loop to push these */
    for (int i = 0 ; i < 8; i++)
    {
        *(--stk) = 0x00000000; /* Push dummy values  for R4 through R11*/
    }

    return stk; /* Just need to retunr the base address and nothing else, since we want the address of the R4 which is the last reg
    we pushed */
}

bool os_task_create(void (*taskptr)(void), uint32_t *stackLimit, uint8_t priority)
{
    if(task_count >= MAX_TASKS) return false;

    TCB_t *new_tcb = &os_tasks[task_count];

    /* Setting basic TCB info */
    new_tcb->priority = priority;
    new_tcb->state = READY;

    /* We now need to initialise our stack -> we know the stack size is 256 words*/
    uint32_t *stackTop = stackLimit + 256;
    new_tcb->stackPtr = os_task_init_stack(taskptr, stackTop);

    /* Setting up the circular LL for round robbin*/
    if (task_count == 0)
    {
        /* Point new task to self */
        new_tcb->next = new_tcb;
        /* Point global ptr to new tcb which points to A since A is pointing to A*/
        current_tcb = new_tcb;
    }
    else{
        /* Not the first task */
        new_tcb->next = current_tcb->next;
        current_tcb->next = new_tcb;
    }

    task_count++;

    return true;
}

void os_scheduler(void)
{
    /* Initialize with NULL or a very low priority baseline */
    TCB_t* best_task = NULL; 
    uint8_t highest_prior = 0;

    /* Search for the highest priority READY task */
    for (int i = 0; i < task_count; i++)
    {
        /* Use . because os_tasks[i] is the struct itself */
        if(os_tasks[i].state == READY)
        {
            /* If it's the first READY task found, or higher priority than the last */
            if (best_task == NULL || os_tasks[i].priority >= highest_prior)
            {
                highest_prior = os_tasks[i].priority;
                best_task = &os_tasks[i]; /* Use & to get the pointer to this task */
            }
        }
    }

    /* If we found a READY task, switch to it. 
       If no tasks were READY, the CPU stays on the previous task 
 */
    if(best_task != NULL) {
        current_tcb = best_task;
    }
}
void SysTick_Handler(void) {
    uint8_t switch_needed = 0;

    /* 1. Loop through all tasks to update sleep timers */
    for (int i = 0; i < task_count; i++) {
        if (os_tasks[i].sleep_time > 0) {
            os_tasks[i].sleep_time--;
            
            /* 2. If timer just hit zero, wake it up! */
            if (os_tasks[i].sleep_time == 0) {
                os_tasks[i].state = READY;
                
                /* 3. Preemption Check: Is this newly awake task higher priority 
                   than the one we are currently running? */
                if (os_tasks[i].priority > current_tcb->priority) {
                    switch_needed = 1;
                }
            }
        }
    }

    /* 4. If a higher priority task is now READY, trigger PendSV */
    if (switch_needed) {
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; 
    }
}

void os_delay(uint32_t ms)
{
    __disable_irq(); /* Start of the critical section */
    current_tcb->sleep_time = ms;
    current_tcb->state = BLOCKED;

    __enable_irq(); /* end of the critical section */

    /* Trigger a Context Switch now */
    /* We do this since, if we dont the task will continue running in its "remaining time slice - 1ms" even though it has nothing to do
    and the CPU will essentially idle inside the OD code instead of doing other useful tasks 
    So by calling this we tell the hardware that this task is finished for now please swap it out for the next READY task*/
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void os_kernel_init(void)
{
    __disable_irq(); /* We dont want anything interfering with this! */
    task_count = 0; /* Clear the task list and everything we have */
    current_tcb = NULL; /* There should be no tasks running yet! */
}

void os_kernel_launch(void)
{
    /* Configure our systick for 1ms interrupts with 16mhz clock */
    SysTick->LOAD = 15999;
    SysTick->VAL = 0;

    /* Enable internal clock */
    SysTick->CTRL = SYSTICK_CTRL_CONFIG;

    /* Set Pendsv to lowest priority so it doesnt interrupt hardware timing */
    SCB->SHP[10] = 0xFF;

    os_start_first_task(); 
}

void os_mutex_acquire(os_mutex_t* mutex)
{
    __disable_irq(); /* We wanna protect the check */

    if(mutex->lock == 0) /* not taken -> free */
    {
        mutex->lock = 1; /* Set the lock to locked */
        mutex->owner = current_tcb;
        __enable_irq(); /* Renable interrupts */
    }
    else
    {
        /* If the lock is in use change the task requesting the lock to blocked  */
        current_tcb->state = BLOCKED;

        /* Add current_tcb to mutex->wait_queue */
        mutex->wait_queue[mutex->wait_count] = current_tcb;
        mutex->wait_count += 1;

        __enable_irq();

        // Trigger a switch so another task can run
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}

void os_mutex_release(os_mutex_t* mutex)
{
    __disable_irq();

    if(mutex->wait_count > 0)/* multiple tasks waiting for the lock */
    {
        /* Direct Handoff: */
        /* Hand voer the ownership and wake the task */
        mutex->owner = mutex->wait_queue[0];
        mutex->owner->state = READY;

        /* Need to shift the queue forward */
        for (int i = 0; i < mutex->wait_count - 1; i++)
        {
            mutex->wait_queue[i] = mutex->wait_queue[i+1];
        }
        mutex->wait_count--;

        __enable_irq();
        /* GEt the CPU to CS */
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    else
    {
        /* Else no task ware waiting */
        mutex->lock = 0;
        mutex->owner = NULL;
    }


}