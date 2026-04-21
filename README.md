# SENTINEL RTOS

SENTINEL is a custom, bare-metal preemptive Real-Time Operating System (RTOS) designed for ARM Cortex-M4 (STM32F411xE). This project focuses on high-performance kernel design, featuring a custom context switcher, synchronization primitives, and a dedicated benchmarking suite.

## Key Features

*   **Preemptive Multitasking:** Custom assembly context switcher using PendSV and SysTick.
*   **Dual-Stack Architecture:** Uses Main Stack Pointer (MSP) for kernel/ISRs and Process Stack Pointer (PSP) for user tasks.
*   **Synchronization Primitives:** 
    *   **Mutexes:** Ownership tracking and waiting queues.
    *   **Semaphores:** Support for both Binary and Counting semaphores.
*   **POST (Power-On Self-Test):** Built-in kernel diagnostic suite that validates primitives before the OS launches.
*   **MicroBench:** High-precision benchmarking using the hardware DWT (Data Watchpoint and Trace) cycle counter.

## Benchmark Results (Iteration 1)

MicroBench measures the cycle-accurate overhead of kernel operations. 

```text
--- SENTINEL MICROBENCH START ---
[Bench] Mutex overhead:94 cycles(5 us)
--- BENCHMARK COMPLETE ---
```

*Note: Results obtained on STM32F411 running at 16MHz.*

## Project Structure

*   `Inc/` & `Kernel/`: Core OS kernel logic, scheduler, and assembly context switcher.
*   `Drivers/`: Bare-metal peripheral drivers (UART, GPIO).
*   `Bench/`: MicroBench suite for performance profiling.
*   `Tasks/`: Default system tasks (Idle task, telemetry).
*   `Src/main.c`: Application entry point and system configuration.

## Build and Flash

Requires the `arm-none-eabi` toolchain and `make`.

```bash
# Compile the project
make all

# Flash to STM32 (requires GDB/OpenOCD)
make flash
```

## Future Roadmap
- Priority Inheritance for Mutexes
- Heap Management (kmalloc)
- Tickless Idle for Power Efficiency
- Inter-Process Communication (Mailboxes/Queues)
