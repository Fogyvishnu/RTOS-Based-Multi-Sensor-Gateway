/**
 * @file ring_buffer.c
 * @brief Lockless circular buffer implementation
 */

#include "ring_buffer.h"

void ring_buffer_init(RingBuffer_t *rb, uint8_t *storage, size_t capacity) {
    if (!rb || !storage || capacity == 0) return;
    rb->buffer = storage;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
}

bool ring_buffer_push(RingBuffer_t *rb, uint8_t byte) {
    if (!rb || !rb->buffer) return false;
    size_t next_head = (rb->head + 1) % rb->capacity;
    if (next_head == rb->tail) {
        /* Buffer is full */
        return false;
    }
    rb->buffer[rb->head] = byte;
    rb->head = next_head;
    return true;
}

bool ring_buffer_pop(RingBuffer_t *rb, uint8_t *byte) {
    if (!rb || !rb->buffer || !byte) return false;
    if (rb->head == rb->tail) {
        /* Buffer is empty */
        return false;
    }
    *byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    return true;
}

bool ring_buffer_peek(const RingBuffer_t *rb, uint8_t *byte) {
    if (!rb || !rb->buffer || !byte) return false;
    if (rb->head == rb->tail) return false;
    *byte = rb->buffer[rb->tail];
    return true;
}

size_t ring_buffer_available(const RingBuffer_t *rb) {
    if (!rb) return 0;
    if (rb->head >= rb->tail) {
        return rb->head - rb->tail;
    } else {
        return rb->capacity - (rb->tail - rb->head);
    }
}

bool ring_buffer_is_empty(const RingBuffer_t *rb) {
    if (!rb) return true;
    return (rb->head == rb->tail);
}

bool ring_buffer_is_full(const RingBuffer_t *rb) {
    if (!rb) return false;
    return ((rb->head + 1) % rb->capacity) == rb->tail;
}

void ring_buffer_flush(RingBuffer_t *rb) {
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
}
