#include "os_queue.h"

/* 
    Send the q struct
    send the buffer array so it has space in memory to use
    send how many items we want to store
*/
void os_queue_init(os_message_queue_t *q, uint32_t *buffer_array, uint32_t capacity)
{
    /* Setp the ring buffer */
    q->buffer = buffer_array;
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;

    /* initialise the sync mech */
    q->lock = (os_mutex_t){0, NULL, {NULL}, 0};
    /* empty_slots: Starts with 'count' at capacity. Max is capacity. */
    q->empty_slots = (os_semaphore_t){capacity, capacity, {NULL}, 0};
    
    /* filled_slots: Starts with 'count' at 0. Max is capacity. */
    q->filled_slots = (os_semaphore_t){0, capacity, {NULL}, 0};
}

uint8_t os_queue_send(os_message_queue_t *q, uint32_t message, uint32_t timeout) 
{
    /* Try to grab an empty slot without blocking (Timeout = 0) */
    if (os_semaphore_acquire(&q->empty_slots, 0) == OS_SUCCESS)
    {
        /* NORMAL SEND: We have space! */
        os_mutex_acquire(&q->lock, WAIT_FOREVER);
        
        /* Write data and advance head */
        q->buffer[q->head] = message;
        q->head = (q->head + 1) % q->capacity;
        
        os_mutex_release(&q->lock);
        
        /* Signal consumer that a new slot is filled */
        os_semaphore_release(&q->filled_slots);
    }
    else
    {
        /* OVERWRITE: Queue is full! (Timeout hit instantly) */
        os_mutex_acquire(&q->lock, WAIT_FOREVER);
        
        /* 1. Overwrite the absolute oldest data */
        q->buffer[q->head] = message;
        
        /* 2. Advance BOTH head and tail to maintain the ring */
        q->head = (q->head + 1) % q->capacity;
        q->tail = (q->tail + 1) % q->capacity; 
        
        os_mutex_release(&q->lock);
        
        /* We DO NOT release filled_slots here because the total 
           number of filled slots hasn't changed (it's still at max capacity). */
    }
    
    return OS_SUCCESS;
}   

uint8_t os_queue_receive(os_message_queue_t *q, uint32_t *buffer, uint32_t timeout)
{

    /* Wait till there is at least one filled slot */
    if (os_semaphore_acquire(&q->filled_slots, timeout) == OS_TIMEOUT)
    {
        return OS_TIMEOUT; /* Do not touch the mutex*/
    }
    

    /* lock memory */
    os_mutex_acquire(&q->lock, timeout);

    /* Read the data */
    *buffer = q->buffer[q->tail];
    q->tail = (q->tail + 1) % q->capacity;

    /* unlock mem */
    os_mutex_release(&q->lock);

    /* Signal to producer that 1 new empty slotis avalibale */
    os_semaphore_release(&q->empty_slots);

    return OS_SUCCESS;
}