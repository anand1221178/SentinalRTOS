#include <stdlib.h>
#include "os_task.h"
#include "lock.h"
#include "os_kernel.h"
#include "tasks.h"
#include "uart.h"
#include "Bench/microbench.h"
#include "Drivers/Inc/servo.h"
#include "os_queue.h"

/* Sweep task initialises */
uint32_t servo_stack[256];
os_message_queue_t sweep_queue;
uint32_t sweep_queue_buffer[8];


int main(void)
{
    /* Initialize Kernel */
    os_kernel_init();

    /* Initialize UART */
    uart_init();
    uart_print("--- SENTINEL RTOS BOOTING ---\r\n");

    /* Run Power-On Self-Tests */
    os_run_post();

    /* Initialize Peripherals */
    servo_init();
    /* Initialise queue with mem buf and capacity */
    os_queue_init(&sweep_queue, sweep_queue_buffer, 8);

    /* Create Tasks */
    os_task_create(sweep_task, &servo_stack[0], 1);

    /* Launch the kernel */
    os_kernel_launch();

    while(1){}; /* DONT REACH HERE! */
}
