/*
 * integrity.c
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdbool.h"
#include "hash.h"
#include "pka.h"

#include "integrity.h"
#include "config.h"


typedef struct {
    bool                       bank_swap;
    volatile integrity_state_t bank1_secure_digest_state;
    sha256_digest_t            bank1_secure_digest;
    volatile integrity_state_t bank2_secure_digest_state;
    sha256_digest_t            bank2_secure_digest;
    volatile integrity_state_t bank1_non_secure_digest_state;
    sha256_digest_t            bank1_non_secure_digest;
    volatile integrity_state_t bank2_non_secure_digest_state;
    sha256_digest_t            bank2_non_secure_digest;
} integrity_handle_t;


static integrity_handle_t hintegrity;

extern HASH_HandleTypeDef hhash;
extern PKA_HandleTypeDef  hpka;


static integrity_status_t _INTEGRITY_Init(integrity_handle_t *self, bool bank_swap) {

    integrity_status_t status = INTEGRITY_OK;

    self->bank_swap = bank_swap;

    self->bank1_secure_digest_state     = INTEGRITY_HASH_NOT_COMPUTED;
    self->bank2_secure_digest_state     = INTEGRITY_HASH_NOT_COMPUTED;
    self->bank1_non_secure_digest_state = INTEGRITY_HASH_NOT_COMPUTED;
    self->bank2_non_secure_digest_state = INTEGRITY_HASH_NOT_COMPUTED;

    return status;
}

integrity_status_t INTEGRITY_Init(bool bank_swap) {
    return _INTEGRITY_Init(&hintegrity, bank_swap);
}


/* ---------------------------------------------------------------------------- */
/* Hash Functions */
/* ---------------------------------------------------------------------------- */


static integrity_status_t _INTEGRITY_start_secure_firmware_hash(integrity_handle_t *self, uint8_t bank) {

    integrity_status_t status    = HAL_OK;
    uint8_t           *start_ptr = NULL;
    uint32_t           size      = FLASH_S_REGION_SIZE + FLASH_NSC_REGION_SIZE;
    sha256_digest_t   *digest;

    /* Check a hash isn't in progress already */
    if (INTEGRITY_get_hash_in_progress()) status = INTEGRITY_BUSY;
    if (status != INTEGRITY_OK) return status;

    /* Check the input */
    if ((bank != FLASH_BANK_1) && (bank != FLASH_BANK_2)) status = INTEGRITY_PARAMETER_ERROR;
    if ((size % 4) != 0) status = INTEGRITY_PARAMETER_ERROR;
    if (status != INTEGRITY_OK) return status;

    /* Get the bank address */
    if (((bank == FLASH_BANK_1) && !self->bank_swap) || ((bank == FLASH_BANK_2) && self->bank_swap)) {
        start_ptr = (uint8_t *) FLASH_S_BANK1_BASE_ADDR;
    } else if (((bank == FLASH_BANK_1) && self->bank_swap) || ((bank == FLASH_BANK_2) && !self->bank_swap)) {
        start_ptr = (uint8_t *) FLASH_S_BANK2_BASE_ADDR;
    } else {
        status = INTEGRITY_PARAMETER_ERROR;
    }
    if (status != INTEGRITY_OK) return status;

    /* Get the output digest pointer */
    digest = (bank == FLASH_BANK_1) ? &self->bank1_secure_digest : &self->bank2_secure_digest;
    if (digest == NULL) status = INTEGRITY_PARAMETER_ERROR;
    if (status != INTEGRITY_OK) return status;

    /* Update self */
    if (bank == FLASH_BANK_1) {
        self->bank1_secure_digest_state = INTEGRITY_HASH_IN_PROGRESS;
    } else if (bank == FLASH_BANK_2) {
        self->bank2_secure_digest_state = INTEGRITY_HASH_IN_PROGRESS;
    }

    /* Start the hash */
    status = HAL_HASH_Start_DMA(&hhash, start_ptr, size, (uint8_t *) digest);
    if (status != INTEGRITY_OK) {

        /* Update self */
        if (bank == FLASH_BANK_1) {
            self->bank1_secure_digest_state = INTEGRITY_HASH_ERROR;
        } else if (bank == FLASH_BANK_2) {
            self->bank2_secure_digest_state = INTEGRITY_HASH_ERROR;
        }

        return status;
    }

    return status;
}

integrity_status_t INTEGRITY_start_secure_firmware_hash(uint8_t bank) {
    return _INTEGRITY_start_secure_firmware_hash(&hintegrity, bank);
}


static sha256_digest_t *_INTEGRITY_get_secure_firmware_hash(integrity_handle_t *self, uint8_t bank) {

    sha256_digest_t *hash = NULL;

    if ((bank == FLASH_BANK_1) && (self->bank1_secure_digest_state == INTEGRITY_HASH_COMPLETE)) {
        hash = &self->bank1_secure_digest;
    } else if ((bank == FLASH_BANK_2) && (self->bank2_secure_digest_state == INTEGRITY_HASH_COMPLETE)) {
        hash = &self->bank2_secure_digest;
    }

    return hash;
}

sha256_digest_t *INTEGRITY_get_secure_firmware_hash(uint8_t bank) {
    return _INTEGRITY_get_secure_firmware_hash(&hintegrity, bank);
}


static integrity_status_t _INTEGRITY_start_non_secure_firmware_hash(integrity_handle_t *self, uint8_t bank) {

    integrity_status_t status = INTEGRITY_NOT_IMPLEMENTED_ERROR;

    return status;
}

integrity_status_t INTEGRITY_start_non_secure_firmware_hash(uint8_t bank) {
    return _INTEGRITY_start_non_secure_firmware_hash(&hintegrity, bank);
}


static bool _INTEGRITY_get_in_progress(integrity_handle_t *self) {
    if (self->bank1_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        return true;
    } else if (self->bank2_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        return true;
    } else if (self->bank1_non_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        return true;
    } else if (self->bank2_non_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        return true;
    } else {
        return false;
    }
}


bool INTEGRITY_get_hash_in_progress() {
    return _INTEGRITY_get_in_progress(&hintegrity);
}


/* ---------------------------------------------------------------------------- */
/* Signature Functions */
/* ---------------------------------------------------------------------------- */


/* TODO: Idk man */
bool INTEGRITY_check_non_secure_firmware_signature(void) {
    PKA_ECDSAVerifInTypeDef signature;
    HAL_PKA_ECDSAVerif(&hpka, &signature, 100);
    HAL_PKA_ECDSAVerif_IsValidSignature(&hpka);
}


/* ---------------------------------------------------------------------------- */
/* Callbacks */
/* ---------------------------------------------------------------------------- */


void HAL_HASH_DgstCpltCallback(HASH_HandleTypeDef *hhash) {
    if (hintegrity.bank1_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        hintegrity.bank1_secure_digest_state = INTEGRITY_HASH_COMPLETE;
    } else if (hintegrity.bank2_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        hintegrity.bank2_secure_digest_state = INTEGRITY_HASH_COMPLETE;
    } else if (hintegrity.bank1_non_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        hintegrity.bank1_non_secure_digest_state = INTEGRITY_HASH_COMPLETE;
    } else if (hintegrity.bank2_non_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        hintegrity.bank2_non_secure_digest_state = INTEGRITY_HASH_COMPLETE;
    } else {
        Error_Handler();
    }
}
