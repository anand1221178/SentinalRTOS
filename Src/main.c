#include <stdlib.h>
#include "os_task.h"

uint32_t task1_stack[256];
uint32_t task2_stack[256];

/* Forward declarations */
void task1(void);
void task2(void);


int main(void)
{
    // Initialize Kernel (Sets up SysTick, etc.)
    os_kernel_init();

    // Create Tasks
    os_task_create(&task1, task1_stack, 1);
    os_task_create(&task2, task2_stack, 1);

    // Launch! (Never returns)
    os_kernel_launch(); 
    
    return 0; // We should never get here
}

void task1(void) {
    while(1) {
        // Toggle an LED or do some math
    }
}

void task2(void) {
    while(1) {
        // Print a message or read a sensor
    }
}