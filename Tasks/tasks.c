#include "tasks.h"

void os_idle_task(void)
{
    while(1)
    {
        /* Suspend CPU until the next interuppt */
        __asm volatile ("wfi");
    }
}