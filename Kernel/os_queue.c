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

void os_queue_send(os_message_queue_t *q, uint32_t message) 
{
    /* Wait till there is atelast 1 empty slot */
    os_semaphore_acquire(&q->empty_slots);

    /* lock mem so we can edit it and make sure no dual edits */
    os_mutex_acquire(&q->lock);

    /* Write the data -> RING BUFFER LOGIC */
    q->buffer[q->head] = message;

    /* Advance the head pointer and wrap around if it hits capacity */
    q->head = (q->head + 1) % q->capacity;

    /* unlock mem */
    os_mutex_release(&q->lock);

    //Signal to the Consumer that there is 1 new filled slot */
    os_semaphore_release(&q->filled_slots);
}

uint32_t os_queue_receive(os_message_queue_t *q)
{
    uint32_t message;

    /* Wait till there is at least one filled slot */
    os_semaphore_acquire(&q->filled_slots);

    /* lock memory */
    os_mutex_acquire(&q->lock);

    /* Read the data */
    message = q->buffer[q->tail];
    q->tail = (q->tail + 1) % q->capacity;

    /* unlock mem */
    os_mutex_release(&q->lock);

    /* Signal to producer that 1 new empty slotis avalibale */
    os_semaphore_release(&q->empty_slots);

    return message;
}