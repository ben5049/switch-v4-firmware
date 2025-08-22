/*
 * metadata.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_METADATA_H_
#define INC_METADATA_H_


#include "fram.h"
#include "integrity.h"


#define METADATA_VERSION_MAJOR              0
#define METADATA_VERSION_MINOR              0
#define METADATA_VERSION_PATCH              0

#define METADATA_ENABLE_ROLLBACK_PROTECTION true


typedef enum {
    META_OK      = HAL_OK,
    META_ERROR   = HAL_ERROR,
    META_BUSY    = HAL_BUSY,
    META_TIMEOUT = HAL_TIMEOUT,
    META_NOT_IMPLEMENTED_ERROR,
    META_NOT_INITIALISED_ERROR,
    META_PARAMETER_ERROR,
    META_FRAM_ERROR,
    META_VERSION_ROLLBACK_ERROR,
    META_RNG_ERROR,
    META_MEMORY_ERROR,
    META_ENCRYPTION_ERROR,
} metadata_status_t;

/* This struct stores the actual metadata data and is a mirror of the data stored in the FRAM. When this struct is updated the METADATA_VERSION numbers must be incremented. */
typedef struct __attribute__((__packed__)) {

    /* Firmware image data */
    bool    secure_firmware_1_valid;
    uint8_t secure_firmware_1_hash[SHA256_SIZE];
    bool    secure_firmware_2_valid;
    uint8_t secure_firmware_2_hash[SHA256_SIZE];
    bool    non_secure_firmware_1_valid;
    uint8_t non_secure_firmware_1_hash[SHA256_SIZE];
    bool    non_secure_firmware_2_valid;
    uint8_t non_secure_firmware_2_hash[SHA256_SIZE];

    /* Device ID computed from hash of 96-bit unique identifier */
    uint32_t device_id;

    /* Must be at the end of the struct */
    uint8_t metadata_version_major;
    uint8_t metadata_version_minor;
    uint8_t metadata_version_patch;
} metadata_data_t;

/* This struct stores the actual metadata counters and is a mirror of the data stored in the FRAM. When this struct is updated the METADATA_VERSION numbers must be incremented. */
typedef struct __attribute__((__packed__)) {

} metadata_counters_t;

typedef struct {
    fram_handle_t hfram;
    bool          first_boot;
    bool          initialised;
    uint32_t      device_id;
    bool          bank_swap;

    __ALIGN_BEGIN metadata_data_t metadata     __ALIGN_END; /* Must be aligned to prevent non-maskable interrupts on non-byte accesses */
    __ALIGN_BEGIN metadata_counters_t counters __ALIGN_END; /* Must be aligned to prevent non-maskable interrupts on non-byte accesses */
} metadata_handle_t;


extern metadata_handle_t hmeta;


metadata_status_t META_Init(metadata_handle_t *meta, bool bank_swap);
metadata_status_t META_Configure(metadata_handle_t *meta, uint8_t *secure_firmware_hash);

metadata_status_t META_set_secure_firmware_hash(metadata_handle_t *self, uint8_t bank, uint8_t *hash);
metadata_status_t META_compare_secure_firmware_hash(metadata_handle_t *self, uint8_t bank, uint8_t *hash, bool *identical);
metadata_status_t META_set_non_secure_firmware_hash(metadata_handle_t *self, uint8_t bank, uint8_t *hash);
metadata_status_t META_compare_non_secure_firmware_hash(metadata_handle_t *self, uint8_t bank, uint8_t *hash, bool *identical);

metadata_status_t META_load_metadata(metadata_handle_t *self);
metadata_status_t META_dump_metadata(metadata_handle_t *self);
metadata_status_t META_load_counters(metadata_handle_t *self);
metadata_status_t META_dump_counters(metadata_handle_t *self);


#endif /* INC_METADATA_H_ */
