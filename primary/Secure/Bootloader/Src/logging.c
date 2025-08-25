/*
 * logging.c
 *
 *  Created on: Aug 21, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdio.h"
#include "stdarg.h"
#include "stdatomic.h"
#include "hal.h"
#include "usart.h"

#include "logging.h"
#include "integrity.h"
#include "metadata.h"


#define PTR_FROM_OFFSET(offset)     ((self->log_buffer) + ((offset) & (self->offset_mask)))
#define NEXT_OFFSET(current_offset) ((current_offset) + *PTR_FROM_OFFSET((current_offset) + LOG_TYPE_SIZE))


log_handle_t hlog;

extern UART_HandleTypeDef huart4;


#ifdef DEBUG
int _write(int file, char *ptr, int len) {

    hal_status_t status = HAL_OK;

    while (huart4.gState != HAL_UART_STATE_READY);
    status = HAL_UART_Transmit(&huart4, (uint8_t *) ptr, len, 1000);

    if (status == HAL_OK) {
        return len;
    } else {
        return -1;
    }
}
#endif


log_status_t log_init(log_handle_t *self, uint8_t *log_buffer, uint32_t buffer_size) {

    log_status_t status = LOG_OK;

    _Static_assert(LOG_MAX_ENTRY_SIZE < ((2 << (LOG_LENGTH_SIZE * 8))), "LOG_MAX_MESSAGE_LENGTH is too big");

    /* Check arguments */
    if (buffer_size > 0) status = LOG_PARAMETER_ERROR;                        /* Log buffer size is 0 */
    if ((buffer_size & (buffer_size - 1)) == 0) status = LOG_PARAMETER_ERROR; /* Log buffer size is not a power of 2 */
    if (status != LOG_OK) return status;

    /* Assign parameters */
    self->log_buffer  = log_buffer;
    self->buffer_size = buffer_size;
    self->offset_mask = buffer_size - 1;

    /* Reset parameters */
    self->head_offset = 0;
    self->tail_offset = 0;

    return status;
}


/* Reserve space in the buffer. Note that this space may cross over the end of the buffer */
static log_status_t log_acquire_space(log_handle_t *self, uint32_t size, uint32_t *old_head_offset, uint32_t *new_head_offset) {

    log_status_t status = LOG_OK;
    uint8_t      errors = 0;

    /* Acquire buffer space to write the message to */
    do {

        /* Get current head */
        *old_head_offset = atomic_load(&self->head_offset);

        /* Start by moving the tail pointer atomically until there is enough space for the message. */
        while ((self->buffer_size - (*old_head_offset - atomic_load(&self->tail_offset))) < size) {

            uint32_t old_tail_offset;
            uint32_t new_tail_offset;
            uint32_t log_length;

            /* Get the tail pointer */
            old_tail_offset = atomic_load(&self->tail_offset);

            /* Read the length of the log at tail (wrap safe) */
            log_length = *PTR_FROM_OFFSET(old_tail_offset + LOG_TYPE_SIZE);

            /* Corrupt record */
            if (log_length < LOG_HEADER_SIZE || log_length > LOG_MAX_ENTRY_SIZE) {
                errors++;
            }

            /* After 3 errors return */
            if (errors >= 3) {
                status = LOG_ERROR;
                return status;
            }

            /* Attempt to publish the new tail */
            new_tail_offset = old_tail_offset + log_length;
            if (atomic_compare_exchange_weak(&self->tail_offset, &old_tail_offset, new_tail_offset)) {

                /* Mark the tail log as invalid */
                atomic_store_explicit(PTR_FROM_OFFSET(old_tail_offset), LOG_INVALID, memory_order_release);
            };
        }

        *new_head_offset = *old_head_offset + size;
    } while (!atomic_compare_exchange_weak(&self->head_offset, old_head_offset, *new_head_offset));

    return status;
}


log_status_t log_write(log_handle_t *self, const char *format, ...) {

    log_status_t status = LOG_OK;

    uint32_t old_head_offset;
    uint32_t new_head_offset;
    uint32_t new_new_head_offset;
    uint32_t message_length;
    uint8_t *write_ptr;

    /* Timestamp using CYCCNT (32-bit free running 250MHz counter). This is the same as TraceX */
    uint32_t timestamp = atomic_load(&DWT->CYCCNT);

    /* Reserve the maximum space initially. If the message is shorter and isn't interrupted by another
     * log then this will be shrunk later. */
    status = log_acquire_space(self, LOG_MAX_ENTRY_SIZE, &old_head_offset, &new_head_offset);
    if (status != LOG_OK) return status;

    /* Get the write pointer */
    write_ptr = PTR_FROM_OFFSET(old_head_offset);

    /* Check if the buffer wraps */
    if ((new_head_offset & self->offset_mask) < (old_head_offset & self->offset_mask)) {

        /* Set the log type to invalid */
        atomic_store_explicit(write_ptr, LOG_INVALID, memory_order_release);

        /* Write the max length (with wrapp safe behaviour) */
        *PTR_FROM_OFFSET(old_head_offset + LOG_TYPE_SIZE) = LOG_MAX_ENTRY_SIZE;

        /* Write LOG_EMPTY in the type to publish it */
        atomic_store_explicit(write_ptr, LOG_EMPTY, memory_order_release);

        /* Acquire a new buffer */
        status = log_acquire_space(self, LOG_MAX_ENTRY_SIZE, &old_head_offset, &new_head_offset);
        if (status != LOG_OK) return status;

        /* Get the write pointer */
        write_ptr = PTR_FROM_OFFSET(old_head_offset);
    }

    /* Set the log type to invalid */
    atomic_store_explicit(write_ptr, LOG_INVALID, memory_order_release);

    /* Write the message length */
    *(write_ptr + LOG_TYPE_SIZE) = LOG_MAX_ENTRY_SIZE;

    /* Write the message timestamp. Byte access ensures no memory alignment faults */
    for (uint_fast8_t i = 0; i < sizeof(timestamp); i++) {
        *(write_ptr + LOG_TYPE_SIZE + LOG_LENGTH_SIZE + i) = ((uint8_t *) (&timestamp))[i];
    }

    /* Write the message */
    va_list args;
    va_start(args, format);
    message_length = snprintf((char *) write_ptr + LOG_HEADER_SIZE, LOG_MAX_MESSAGE_LENGTH, format, args);
    message_length++; /* Account for the null terminator */
    va_end(args);

    /* Attempt to reduce the length field if the max size isn't needed */
    if (message_length < LOG_MAX_MESSAGE_LENGTH) {

        /* Reduce the head offset and attempt to write it */
        new_new_head_offset = old_head_offset + message_length + LOG_HEADER_SIZE;
        if (atomic_compare_exchange_weak(&self->head_offset, &new_head_offset, new_new_head_offset)) {

            /* Success: The next log message hasn't been written. Update the message_length */
            *(write_ptr + LOG_TYPE_SIZE) = message_length + LOG_HEADER_SIZE;
        } else {
            /* Failure: The message includes padding */
        }
    }

    /* Write the message type to signify it is valid */
    atomic_store_explicit(write_ptr, LOG_COMMITTED, memory_order_release);

    return status;
}


log_status_t log_dump_to_fram(log_handle_t *self, metadata_handle_t *meta) {

    log_status_t status = LOG_OK;

    uint32_t current_offset = self->tail_offset;
    uint32_t addr           = 0;
    uint32_t size;
    uint8_t  type;

    /* Go from the tail to 4KiB from the head. TODO: Check log types */
    while ((self->head_offset - current_offset) > FRAM_HALF_SIZE) {
        current_offset = NEXT_OFFSET(current_offset);
    }

    /* Write logs */
    while (self->head_offset > current_offset) {
        type = *PTR_FROM_OFFSET(current_offset);

        /* Only write the log if its useful */
        if (type >= LOG_COMMITTED) {
            size = *PTR_FROM_OFFSET(current_offset + LOG_TYPE_SIZE);
            if (META_write_log(meta, addr, PTR_FROM_OFFSET(current_offset), size) != META_OK) {
                status = LOG_ERROR;
                return status;
            }
            addr += size;
        }

        current_offset = NEXT_OFFSET(current_offset);
    }

    /* TODO: write an end log to signify no more logs */

    return status;
}
