#ifndef __MIRCOBENCH_H__
#define __MIRCOBENCH_H__

#include "stm32f4xx.h" 
#include "os_kernel.h"
#include "uart.h"

/* Init function for microbench */
void microbench_init(void);

/* Mutex bench function */
void bench_mutex_overhead(void);

#endif