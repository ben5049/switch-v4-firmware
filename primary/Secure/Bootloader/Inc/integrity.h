/*
 * integrity.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_INTEGRITY_H_
#define INC_INTEGRITY_H_


#include "stdint.h"
#include "stdbool.h"
#include "hal.h"
#include "hash.h"


#define SHA256_SIZE 32 /* Size in bytes */


typedef enum {
    INTEGRITY_OK      = HAL_OK,
    INTEGRITY_ERROR   = HAL_ERROR,
    INTEGRITY_BUSY    = HAL_BUSY,
    INTEGRITY_TIMEOUT = HAL_TIMEOUT,
    INTEGRITY_NOT_IMPLEMENTED_ERROR,
    INTEGRITY_PARAMETER_ERROR,
} integrity_status_t;

typedef enum {
    INTEGRITY_HASH_NOT_COMPUTED,
    INTEGRITY_HASH_IN_PROGRESS,
    INTEGRITY_HASH_COMPLETE,
    INTEGRITY_HASH_ERROR,
} integrity_state_t;

typedef struct {
    uint8_t bytes[SHA256_SIZE];
} sha256_digest_t;


integrity_status_t INTEGRITY_Init(bool bank_swap);

/* Hash functions */
integrity_status_t INTEGRITY_start_secure_firmware_hash(uint8_t bank);
sha256_digest_t   *INTEGRITY_get_secure_firmware_hash(uint8_t bank);
integrity_status_t INTEGRITY_start_non_secure_firmware_hash(uint8_t bank);
sha256_digest_t   *INTEGRITY_get_non_secure_firmware_hash(uint8_t bank);
bool               INTEGRITY_get_hash_in_progress(void);

/* TODO: */
bool INTEGRITY_check_non_secure_firmware_signature(void);


#endif /* INC_INTEGRITY_H_ */
