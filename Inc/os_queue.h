#ifndef __OS_QUEUE_H__
#define __OS_QUEUE_H__

#include <stdint.h>
#include "os_kernel.h" // Assuming this is where os_mutex_t and os_semaphore_t live

typedef struct {
    uint32_t *buffer;             /* Pointer to the actual array in memory */
    uint32_t capacity;            /* Maximum number of items the queue can hold */
    uint32_t head;                /* Index where the Producer writes */
    uint32_t tail;                /* Index where the Consumer reads */
    
    os_mutex_t lock;              /* Protects the array, head, and tail */
    os_semaphore_t empty_slots;   /* Puts Producer to sleep if queue is full */
    os_semaphore_t filled_slots;  /* Puts Consumer to sleep if queue is empty */
} os_message_queue_t;

void os_queue_init(os_message_queue_t *q, uint32_t *buffer_array, uint32_t capacity);
void os_queue_send(os_message_queue_t *q, uint32_t message);
uint32_t os_queue_receive(os_message_queue_t *q);

#endif