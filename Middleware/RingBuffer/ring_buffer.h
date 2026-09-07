/**
 * @file ring_buffer.h
 * @brief Lockless Single-Producer Single-Consumer circular ring buffer for UART RX/TX
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    volatile size_t head;
    volatile size_t tail;
} RingBuffer_t;

/**
 * @brief Initialize ring buffer
 */
void ring_buffer_init(RingBuffer_t *rb, uint8_t *storage, size_t capacity);

/**
 * @brief Push a single byte into the ring buffer
 * @return true if pushed, false if buffer is full
 */
bool ring_buffer_push(RingBuffer_t *rb, uint8_t byte);

/**
 * @brief Pop a single byte from the ring buffer
 * @return true if popped, false if buffer is empty
 */
bool ring_buffer_pop(RingBuffer_t *rb, uint8_t *byte);

/**
 * @brief Peek at the next byte without removing it
 */
bool ring_buffer_peek(const RingBuffer_t *rb, uint8_t *byte);

/**
 * @brief Get number of bytes available to read
 */
size_t ring_buffer_available(const RingBuffer_t *rb);

/**
 * @brief Check if buffer is empty
 */
bool ring_buffer_is_empty(const RingBuffer_t *rb);

/**
 * @brief Check if buffer is full
 */
bool ring_buffer_is_full(const RingBuffer_t *rb);

/**
 * @brief Flush/clear ring buffer
 */
void ring_buffer_flush(RingBuffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H */
