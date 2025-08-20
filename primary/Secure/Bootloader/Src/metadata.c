/*
 * metadata.c
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#include "memory.h"
#include "main.h"
#include "hal.h"

#include "metadata.h"
#include "fram.h"
#include "stm32_uidhash.h"
#include "utils.h"


#define META_FRAM_DATA_START_ADDR ((FRAM_END_ADDR + 1) - sizeof(metadata_data_t))


metadata_handle_t __attribute__((section(".BACKUP_Section"))) hmeta; /* Placed in backup SRAM */

extern SPI_HandleTypeDef hspi1;


static void META_GetDeviceID(metadata_handle_t *self) {

    /* Get the unique identifier (UID) of this microcontroller and put it into an array of bytes */
    uint8_t  UID[12];
    uint32_t UID_word1 = HAL_GetUIDw0();
    uint32_t UID_word2 = HAL_GetUIDw1();
    uint32_t UID_word3 = HAL_GetUIDw2();
    UID[0]             = (UID_word1 & 0x000000FF) >> 0;
    UID[1]             = (UID_word1 & 0x0000FF00) >> 8;
    UID[2]             = (UID_word1 & 0x00FF0000) >> 16;
    UID[3]             = (UID_word1 & 0xFF000000) >> 24;
    UID[4]             = (UID_word2 & 0x000000FF) >> 0;
    UID[5]             = (UID_word2 & 0x0000FF00) >> 8;
    UID[6]             = (UID_word2 & 0x00FF0000) >> 16;
    UID[7]             = (UID_word2 & 0xFF000000) >> 24;
    UID[8]             = (UID_word3 & 0x000000FF) >> 0;
    UID[9]             = (UID_word3 & 0x0000FF00) >> 8;
    UID[10]            = (UID_word3 & 0x00FF0000) >> 16;
    UID[11]            = (UID_word3 & 0xFF000000) >> 24;

    /* Hash the UID array into a 32 bit number */
    self->device_id = Hash32Len5to12(UID, 12);
}


static metadata_status_t META_lock_metadata(metadata_handle_t *self) {

    metadata_status_t    status = META_OK;
    fram_block_protect_t protection;

    _Static_assert(sizeof(metadata_data_t) <= FRAM_HALF_SIZE, "Metadata struct too large for FRAM");
    _Static_assert(sizeof(metadata_counters_t) <= FRAM_HALF_SIZE, "Counters struct too large for FRAM");

    /* Only lock the upper quarter if the metadata fits there */
    if (sizeof(metadata_data_t) <= FRAM_QUARTER_SIZE) {
        protection = FRAM_PROTECT_UPPER_QUARTER;
    }

    /* Lock upper half if the metadata fits there */
    else {
        protection = FRAM_PROTECT_UPPER_HALF;
    }

    /* Write the protection */
    if (FRAM_SetBlockProtection(&self->hfram, protection) != FRAM_OK) status = META_FRAM_ERROR;
    if (status != META_OK) return status;

    return status;
}


static metadata_status_t META_unlock_metadata(metadata_handle_t *self) {

    metadata_status_t status = META_OK;

    /* Disable protection */
    if (FRAM_SetBlockProtection(&self->hfram, FRAM_PROTECT_NONE) != FRAM_OK) status = META_FRAM_ERROR;
    if (status != META_OK) return status;

    return status;
}


metadata_status_t META_Init(metadata_handle_t *self, bool bank_swap) {

    metadata_status_t status = META_OK;

    /* Reset the module struct */
    self->initialised = false;
    self->first_boot  = false;
    self->bank_swap   = bank_swap;

    /* Wait for the FRAM to be available */
    while (HAL_GetTick() < FRAM_TPU) __NOP();

    /* Initialise the FRAM */
    if (FRAM_Init(&self->hfram, FRAM_VARIANT_FM25CL64B, &hspi1, FRAM_CS_GPIO_Port, FRAM_CS_Pin, FRAM_HOLD_GPIO_Port, FRAM_HOLD_Pin, FRAM_WP_GPIO_Port, FRAM_WP_Pin) != FRAM_OK) status = META_FRAM_ERROR;
    if (status != META_OK) return status;

    /* Enable metadata area protection in the FRAM (counters are always unlocked) */
    status = META_lock_metadata(self);
    if (status != META_OK) return status;

    /* Load the metadata */
    status = META_load_metadata(self);
    if (status != META_OK) return status;

    /* Get the device ID */
    META_GetDeviceID(self);
    if (self->device_id != self->metadata.device_id) {
        self->first_boot = true;
    }

    /* Check if the metadata version in the firmware is new */
    if (METADATA_VERSION_MAJOR > self->metadata.metadata_version_major) {
        self->first_boot = true; /* New major version */
    } else if (METADATA_VERSION_MAJOR > self->metadata.metadata_version_minor) {
        self->first_boot = true; /* New minor version */
    } else if (METADATA_VERSION_PATCH > self->metadata.metadata_version_patch) {
        self->first_boot = true; /* New patch version */
    }

#if METADATA_ENABLE_ROLLBACK_PROTECTION == true
    /* Check for rollbacks */
    if (METADATA_VERSION_MAJOR < self->metadata.metadata_version_major) {
        status = META_VERSION_ROLLBACK_ERROR;
        return status;
    } else if ((METADATA_VERSION_MAJOR == self->metadata.metadata_version_major) &&
               (METADATA_VERSION_MINOR < self->metadata.metadata_version_minor)) {
        status = META_VERSION_ROLLBACK_ERROR;
        return status;
    } else if ((METADATA_VERSION_MAJOR == self->metadata.metadata_version_major) &&
               (METADATA_VERSION_MINOR == self->metadata.metadata_version_minor) &&
               (METADATA_VERSION_PATCH < self->metadata.metadata_version_patch)) {
        status = META_VERSION_ROLLBACK_ERROR;
        return status;
    }
#endif /* METADATA_ENABLE_ROLLBACK_PROTECTION == true */

    self->initialised = true;

    return status;
}


/* Should be called if META_Init() found this was the first boot */
metadata_status_t META_Configure(metadata_handle_t *self, sha256_digest_t *secure_firmware_hash) {

    metadata_status_t status = META_NOT_IMPLEMENTED_ERROR;

    /* Clear the counters */
    memset(&self->counters, 0, sizeof(metadata_counters_t));

    /* Set the device ID */
    self->metadata.device_id = self->device_id;

    /* Set the metadata version */
    self->metadata.metadata_version_major = METADATA_VERSION_MAJOR;
    self->metadata.metadata_version_minor = METADATA_VERSION_MINOR;
    self->metadata.metadata_version_patch = METADATA_VERSION_PATCH;

    /* Set the secure firmware hash */
    META_set_secure_firmware_hash(&hmeta, CURRENT_FLASH_BANK(self->bank_swap), secure_firmware_hash);
    /* TODO: Set valid separately ^ */

    /* Dump the configured metadata */
    status = META_dump_metadata(self);
    if (status != META_OK) return status;

    /* Dump the empty counters */
    status = META_dump_counters(self);
    if (status != META_OK) return status;

    return status;
}


/* Load the metadata (data) from the FRAM */
metadata_status_t META_load_metadata(metadata_handle_t *self) {

    metadata_status_t status = META_OK;

    _Static_assert((META_FRAM_DATA_START_ADDR + sizeof(metadata_data_t) - 1) == FRAM_END_ADDR, "Metadata struct not aligned to end of FRAM");

    if (FRAM_Read(&self->hfram, META_FRAM_DATA_START_ADDR, (uint8_t *) &self->metadata, sizeof(metadata_data_t)) != FRAM_OK) status = META_FRAM_ERROR;

    return status;
}


/* Dump the metadata (data) to the FRAM */
metadata_status_t META_dump_metadata(metadata_handle_t *self) {

    metadata_status_t status = META_OK;

    /* Don't dump non-initialised metadata */
    if (self->initialised == false) status = META_NOT_INITIALISED_ERROR;
    if (status != META_OK) return status;

    _Static_assert((META_FRAM_DATA_START_ADDR + sizeof(metadata_data_t) - 1) == FRAM_END_ADDR, "Metadata struct not aligned to end of FRAM");

    /* Unlock the metadata region in the FRAM */
    status = META_unlock_metadata(self);
    if (status != META_OK) return status;

    /* Write to the FRAM */
    if (FRAM_Write(&self->hfram, META_FRAM_DATA_START_ADDR, (uint8_t *) &self->metadata, sizeof(metadata_data_t)) != FRAM_OK) status = META_FRAM_ERROR;
    if (status != META_OK) return status;

    /* Lock the metadata region in the FRAM . TODO: If there is an error earlier then this needs to be called before returning */
    status = META_lock_metadata(self);
    if (status != META_OK) return status;

    return status;
}


/* Load the event counters from the FRAM */
metadata_status_t META_load_counters(metadata_handle_t *self) {

    metadata_status_t status = META_NOT_IMPLEMENTED_ERROR;

    return status;
}


/* Dump the event counters to the FRAM */
metadata_status_t META_dump_counters(metadata_handle_t *self) {

    metadata_status_t status = META_NOT_IMPLEMENTED_ERROR;

    /* Don't dump non-initialised metadata */
    if (self->initialised == false) status = META_NOT_INITIALISED_ERROR;
    if (status != META_OK) return status;

    return status;
}


metadata_status_t META_set_secure_firmware_hash(metadata_handle_t *self, uint8_t bank, sha256_digest_t *hash) {

    metadata_status_t status = META_OK;
    sha256_digest_t  *destination_hash;

    /* Get the hash destination pointer */
    if (bank == FLASH_BANK_1) {
        destination_hash = &self->metadata.secure_firmware_1_hash;
    } else if (bank == FLASH_BANK_2) {
        destination_hash = &self->metadata.secure_firmware_2_hash;
    } else {
        status = META_PARAMETER_ERROR;
    }
    if (status != META_OK) return status;

    /* Copy the hash */
    _Static_assert(sizeof(sha256_digest_t) == SHA256_SIZE, "Hash struct incorrect size");
    memcpy(destination_hash, hash, sizeof(sha256_digest_t));

    /* Update the device struct */
    if (bank == FLASH_BANK_1) {
        self->metadata.secure_firmware_1_valid = true;
    } else if (bank == FLASH_BANK_2) {
        self->metadata.secure_firmware_2_valid = true;
    }

    self->metadata.secure_firmware_1_valid = true;
    return status;
}


metadata_status_t META_compare_secure_firmware_hash(metadata_handle_t *self, uint8_t bank, sha256_digest_t *hash, bool *identical) {

    metadata_status_t status      = META_OK;
    sha256_digest_t  *stored_hash = NULL;

    /* Retrieve the stored hash for the bank's secure firmware */
    if ((bank == FLASH_BANK_1) && self->metadata.secure_firmware_1_valid) {
        stored_hash = &self->metadata.secure_firmware_1_hash;
    } else if ((bank == FLASH_BANK_2) && self->metadata.secure_firmware_2_valid) {
        stored_hash = &self->metadata.secure_firmware_2_hash;
    } else {
        status = META_PARAMETER_ERROR;
    }
    if (status != META_OK) return status;

    /* Check the retrieved hash is valid */
    if (stored_hash == NULL) status = META_PARAMETER_ERROR;
    if (status != META_OK) return status;

    /* Compare the hashes */
    *identical = memcmp(hash, stored_hash, sizeof(sha256_digest_t)) == 0;

    return status;
}


metadata_status_t META_set_non_secure_firmware_hash(metadata_handle_t *self, uint8_t bank, sha256_digest_t *hash) {

    metadata_status_t status = META_OK;
    sha256_digest_t  *destination_hash;

    /* Get the hash destination pointer */
    if (bank == FLASH_BANK_1) {
        destination_hash = &self->metadata.non_secure_firmware_1_hash;
    } else if (bank == FLASH_BANK_2) {
        destination_hash = &self->metadata.non_secure_firmware_2_hash;
    } else {
        status = META_PARAMETER_ERROR;
    }
    if (status != META_OK) return status;

    /* Copy the hash */
    _Static_assert(sizeof(sha256_digest_t) == SHA256_SIZE, "Hash struct incorrect size");
    memcpy(destination_hash, hash, sizeof(sha256_digest_t));

    /* Update the device struct */
    if (bank == FLASH_BANK_1) {
        self->metadata.non_secure_firmware_1_valid = true;
    } else if (bank == FLASH_BANK_2) {
        self->metadata.non_secure_firmware_2_valid = true;
    }

    self->metadata.secure_firmware_1_valid = true;
    return status;
}


metadata_status_t META_compare_non_secure_firmware_hash(metadata_handle_t *self, uint8_t bank, sha256_digest_t *hash, bool *identical) {

    metadata_status_t status      = META_OK;
    sha256_digest_t  *stored_hash = NULL;

    /* Retrieve the stored hash for the bank's secure firmware */
    if ((bank == FLASH_BANK_1) && self->metadata.non_secure_firmware_1_valid) {
        stored_hash = &self->metadata.non_secure_firmware_1_hash;
    } else if ((bank == FLASH_BANK_2) && self->metadata.non_secure_firmware_2_valid) {
        stored_hash = &self->metadata.non_secure_firmware_2_hash;
    } else {
        status = META_PARAMETER_ERROR;
    }
    if (status != META_OK) return status;

    /* Check the retrieved hash is valid */
    if (stored_hash == NULL) status = META_PARAMETER_ERROR;
    if (status != META_OK) return status;

    /* Compare the hashes */
    *identical = memcmp(hash, stored_hash, sizeof(sha256_digest_t)) == 0;

    return status;
}
