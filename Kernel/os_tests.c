#include "os_kernel.h"
#include "uart.h"

static void test_mutex_basic(void) {
    os_mutex_t test_mutex;
    test_mutex.lock = 0;
    test_mutex.owner = NULL;
    test_mutex.wait_count = 0;
    
    uart_print("[POST] Testing Mutex... ");
    
    os_mutex_acquire(&test_mutex);
    if (test_mutex.lock != 1) {
        uart_print("FAIL: Lock not set\r\n");
        while(1);
    }
    
    os_mutex_release(&test_mutex);
    if (test_mutex.lock != 0) {
        uart_print("FAIL: Lock not cleared\r\n");
        while(1);
    }
    
    uart_print("PASS\r\n");
}

static void test_semaphore_basic(void) {
    os_semaphore_t test_sem;
    test_sem.count = 2;
    test_sem.max_count = 2;
    test_sem.wait_count = 0;
    
    uart_print("[POST] Testing Semaphore... ");
    
    os_semaphore_acquire(&test_sem);
    if (test_sem.count != 1) {
        uart_print("FAIL: Count not 1\r\n");
        while(1);
    }
    
    os_semaphore_acquire(&test_sem);
    if (test_sem.count != 0) {
        uart_print("FAIL: Count not 0\r\n");
        while(1);
    }
    
    os_semaphore_release(&test_sem);
    if (test_sem.count != 1) {
        uart_print("FAIL: Release failed\r\n");
        while(1);
    }
    
    uart_print("PASS\r\n");
}

#include "os_queue.h"

static void test_queue_basic(void) {
    os_message_queue_t test_q;
    uint32_t test_buffer[4];
    uint32_t msg_sent = 0xDEADBEEF;
    uint32_t msg_received = 0;

    uart_print("[POST] Testing Queue... ");

    os_queue_init(&test_q, test_buffer, 4);
    os_queue_send(&test_q, msg_sent);
    msg_received = os_queue_receive(&test_q);

    if (msg_received != msg_sent) {
        uart_print("FAIL: Data mismatch\r\n");
        while(1);
    }

    uart_print("PASS\r\n");
}

void os_run_post(void) {
    uart_print("\r\n--- STARTING KERNEL POST ---\r\n");
    
    test_mutex_basic();
    test_semaphore_basic();
    test_queue_basic();
    
    uart_print("--- KERNEL READY ---\r\n\r\n");
}
