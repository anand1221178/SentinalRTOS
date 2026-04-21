#include "microbench.h"

void microbench_init(void) {
    /* 1. Enable the Trace and Debug block */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* 2. Reset the cycle counter */
    DWT->CYCCNT = 0;
    /* 3. Enable the cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* Enable uart */
    uart_init();
}

void bench_mutex_overhead(void)
{
    /* Setup */
    os_mutex_t bench_mutex = {0,NULL,{NULL},0};
    uint32_t start_time, end_time, overhead_cycles, overhead_us;

    /* Start recording times */
    start_time = DWT->CYCCNT; /* Get the starting cycle count */

    /* Execute (Measuring an uncontested acquire and release) */
    os_mutex_acquire(&bench_mutex);
    os_mutex_release(&bench_mutex);

    /* Stop and calculate */
    end_time = DWT->CYCCNT; /* Get the end cycle count */
    overhead_cycles = end_time - start_time; /* Difference */

    /* Find overhead -> we have a 16mhz proc */
    overhead_us = overhead_cycles / 16;

    /* Results */
    uart_print("[Bench] Mutex overhead:");
    uart_print_number(overhead_cycles);
    uart_print(" cycles(");
    uart_print_number(overhead_us);
    uart_print(" us)\r\n");
}