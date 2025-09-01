/*
 * logging.h
 *
 *  Created on: Aug 21, 2025
 *      Author: bens1
 */

#ifndef INC_LOGGING_H_
#define INC_LOGGING_H_


#include "stdio.h"
#include "stdarg.h"
#include "hal.h"

#include "utils.h"
#include "error.h"


#define LOG_INFO(format, ...)                     _LOG(format, ##__VA_ARGS__)
#define LOG_INFO_NO_CHECK(format, ...)            _LOG_NO_CHECK(format, ##__VA_ARGS__)
#define LOG_INFO_SHA256(format, hash_ptr, ...)    _LOG_SHA256(format, hash_ptr, ##__VA_ARGS__)

#define LOG_WARNING(format, ...)                  _LOG(format, ##__VA_ARGS__)
#define LOG_WARNING_NO_CHECK(format, ...)         _LOG_NO_CHECK(format, ##__VA_ARGS__)
#define LOG_WARNING_SHA256(format, hash_ptr, ...) _LOG_SHA256(format, hash_ptr, ##__VA_ARGS__)

#define LOG_ERROR(format, ...)                    _LOG(format, ##__VA_ARGS__)
#define LOG_ERROR_NO_CHECK(format, ...)           _LOG_NO_CHECK(format, ##__VA_ARGS__)
#define LOG_ERROR_SHA256(format, hash_ptr, ...)   _LOG_SHA256(format, hash_ptr, ##__VA_ARGS__)


#define _LOG(format, ...)                                                   \
    do {                                                                    \
        printf("%10lu: ", HAL_GetTick()); /* TODO: Add UART toggle macro */ \
        printf(format, ##__VA_ARGS__);    /* TODO: Add UART toggle macro */ \
        log_status_t _s = log_write(&hlog, format, ##__VA_ARGS__);          \
        CHECK_STATUS(_s, LOG_OK, ERROR_LOG);                                \
    } while (0)

#define _LOG_NO_CHECK(format, ...)                                          \
    do {                                                                    \
        printf("%10lu: ", HAL_GetTick()); /* TODO: Add UART toggle macro */ \
        printf(format, ##__VA_ARGS__);    /* TODO: Add UART toggle macro */ \
        log_write(&hlog, format, ##__VA_ARGS__);                            \
    } while (0)

#define _LOG_SHA256(format, hash_ptr, ...)                                                       \
    do {                                                                                         \
        char  _hash_buf[2 + (2 * SHA256_SIZE) + 1]; /* "0x" + "XX" per byte + null terminator */ \
        char *_p  = _hash_buf;                                                                   \
        _p       += sprintf(_p, "0x");                                                           \
        for (uint_fast8_t i = 0; i < SHA256_SIZE; i++) {                                         \
            _p += sprintf(_p, "%02x", ((uint8_t *) hash_ptr)[i]);                                \
        }                                                                                        \
        printf(format, _hash_buf, ##__VA_ARGS__); /* TODO: Add UART toggle macro */              \
        log_status_t _s = log_write(&hlog, format, _hash_buf, ##__VA_ARGS__);                    \
        CHECK_STATUS(_s, LOG_OK, ERROR_LOG);                                                     \
    } while (0)

#define CHECK_STATUS_LOG(status) CHECK_STATUS((status), LOG_OK, ERROR_LOG)


#define LOG_BUFFER_SIZE          (128 * 1024)
#define LOG_MAX_MESSAGE_LENGTH   (249)

#define LOG_TYPE_SIZE            (1)
#define LOG_LENGTH_SIZE          (1) /* Max length is (2^LOG_LENGTH_SIZE - 1) */
#define LOG_TIMESTAMP_SIZE       (4) /* 32-bit timestamp */
#define LOG_HEADER_SIZE          (LOG_TYPE_SIZE + LOG_LENGTH_SIZE + LOG_TIMESTAMP_SIZE)

#define LOG_MAX_ENTRY_SIZE       (LOG_HEADER_SIZE + LOG_MAX_MESSAGE_LENGTH)

#define LOG_INVALID              (0)
#define LOG_EMPTY                (1)
#define LOG_COMMITTED            (2)


typedef enum {
    LOG_OK      = HAL_OK,
    LOG_ERROR   = HAL_ERROR,
    LOG_BUSY    = HAL_BUSY,
    LOG_TIMEOUT = HAL_TIMEOUT,
    LOG_PARAMETER_ERROR,
} log_status_t;

typedef struct {
    uint32_t head_offset;
    uint32_t tail_offset;
    uint8_t *log_buffer;
    uint32_t buffer_size;
    uint32_t offset_mask; /* E.g 0x0001ffff for 128KiB buffer */
} log_handle_t;


extern log_handle_t hlog;


log_status_t log_init(log_handle_t *self, uint8_t *log_buffer, uint32_t buffer_size);
log_status_t log_write(log_handle_t *self, const char *format, ...);


#endif /* INC_LOGGING_H_ */
