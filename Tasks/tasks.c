#include "tasks.h"
#include "uart.h"

void os_idle_task(void)
{
    while(1)
    {
        /* Suspend CPU until the next interuppt */
        // uart_print("IDLE\r\n");
        __asm volatile ("wfi");
    }
}