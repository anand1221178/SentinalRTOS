/* STM32 has 32 bit wide regs -> each increment or decreament will move exactly one reg width in mem*/
#include "os_kernel.h"

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
    /* We start by looking at the next task in the CLL */
    TCB_t *next_task = current_tcb->next;

    /* We have to skip over any tasks which are not in the READY state -> use a do-while*/
    do
    {
        if(next_task->state == READY)
        {
            /* We wanna round robin to this one since it is ready to work*/
            current_tcb = next_task;
            return;
        }
        next_task = next_task->next;
    } while (next_task != current_tcb->next);
    
    /* If we get here it means no tasks were ready so do idling task */
}

void SysTick_Handler(void)
{
    os_scheduler(); /* Decide who to take on next */

    /* Push the "Request PendSV" button in the CPU */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}