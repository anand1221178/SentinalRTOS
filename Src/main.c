#include <stdlib.h>
#include "os_task.h"
#include "lock.h"
#include "os_kernel.h"
#include "tasks.h"

uint32_t task1_stack[256];
uint32_t task2_stack[256];
uint32_t idle_task_stack[256];

/* Forward declarations */
void task1(void);
void task2(void);

int shared_var = 0;
os_mutex_t shared_mutex = {0, NULL, {NULL}, 0}; /* lock = 0, owner = NULL, queue = empty, wait_count = 0 */

int main(void)
{
    /* clear board */
    os_kernel_init();

    /* Setup tasks */
    os_task_create(task1, &task1_stack[0], 1);
    os_task_create(task2, &task2_stack[0], 1); /* Same priority */
    os_task_create(os_idle_task, &idle_task_stack[0], 0);
    /* Launch the kernel */
    os_kernel_launch();

    while(1){}; /* DONT REACH HERE! */
}

void task1(void) {
    while(1) {
        /* Aquire the lock */
        os_mutex_acquire(&shared_mutex);

        shared_var += 10;

        os_mutex_release(&shared_mutex);

        os_delay(100);
    }
}

void task2(void) {
    while(1) {
        /* Aquire the lock */
        os_mutex_acquire(&shared_mutex);

        shared_var += 5;

        os_mutex_release(&shared_mutex);

        os_delay(100);
    }
}
