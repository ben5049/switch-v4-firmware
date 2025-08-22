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


#define LOG_INFO(...)                _LOG(__VA_ARGS__)
#define LOG_INFO_SHA256(hash_ptr)    _LOG_SHA256(hash_ptr)

#define LOG_WARNING(...)             _LOG(__VA_ARGS__)
#define LOG_WARNING_SHA256(hash_ptr) _LOG_SHA256(hash_ptr)

#define LOG_ERROR(...)               _LOG(__VA_ARGS__)
#define LOG_ERROR_SHA256(hash_ptr)   _LOG_SHA256(hash_ptr)


#define _LOG(...)                         \
    do {                                  \
        printf("%10ld: ", HAL_GetTick()); \
        printf(__VA_ARGS__);              \
    } while (0)

#define _LOG_SHA256(hash_ptr)                            \
    do {                                                 \
        printf("0x");                                    \
        for (uint_fast8_t i = 0; i < SHA256_SIZE; i++) { \
            printf("%02x", ((uint8_t *) hash_ptr)[i]);   \
        }                                                \
        printf("\n");                                    \
    } while (0)


#endif /* INC_LOGGING_H_ */
