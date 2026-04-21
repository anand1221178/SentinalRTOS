#include <stdlib.h>
#include "os_task.h"
#include "lock.h"
#include "os_kernel.h"
#include "tasks.h"
#include "uart.h"
#include "Bench/microbench.h"

int main(void)
{
    /* Initialize the microbenchmark suite (enables DWT counter and UART) */
    microbench_init();
    
    uart_print("--- SENTINEL MICROBENCH START ---\r\n");

    /* Run the Mutex Overhead Benchmark */
    bench_mutex_overhead();

    uart_print("--- BENCHMARK COMPLETE ---\r\n");

    while(1){}; /* End of benchmark */
}
