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
#include "prime256v1.h"
#include "logging.h"


typedef struct {
    bool                       bank_swap;
    volatile integrity_state_t bank1_secure_digest_state;
    uint8_t                   *bank1_secure_digest;
    volatile integrity_state_t bank2_secure_digest_state;
    uint8_t                   *bank2_secure_digest;
    volatile integrity_state_t bank1_non_secure_digest_state;
    uint8_t                   *bank1_non_secure_digest;
    volatile integrity_state_t bank2_non_secure_digest_state;
    uint8_t                   *bank2_non_secure_digest;
} integrity_handle_t;


static integrity_handle_t hintegrity;

extern HASH_HandleTypeDef hhash;
extern PKA_HandleTypeDef  hpka;


static integrity_status_t _INTEGRITY_Init(
    integrity_handle_t *self,
    bool                bank_swap,
    uint8_t            *current_bank_secure_digest,
    uint8_t            *other_bank_secure_digest,
    uint8_t            *current_bank_non_secure_digest,
    uint8_t            *other_bank_non_secure_digest) {

    integrity_status_t status = INTEGRITY_OK;

    self->bank_swap = bank_swap;

    self->bank1_secure_digest_state     = INTEGRITY_HASH_NOT_COMPUTED;
    self->bank2_secure_digest_state     = INTEGRITY_HASH_NOT_COMPUTED;
    self->bank1_non_secure_digest_state = INTEGRITY_HASH_NOT_COMPUTED;
    self->bank2_non_secure_digest_state = INTEGRITY_HASH_NOT_COMPUTED;

    if (bank_swap) {
        self->bank1_secure_digest     = other_bank_secure_digest;
        self->bank2_secure_digest     = current_bank_secure_digest;
        self->bank1_non_secure_digest = other_bank_non_secure_digest;
        self->bank2_non_secure_digest = current_bank_non_secure_digest;
    } else {
        self->bank1_secure_digest     = current_bank_secure_digest;
        self->bank2_secure_digest     = other_bank_secure_digest;
        self->bank1_non_secure_digest = current_bank_non_secure_digest;
        self->bank2_non_secure_digest = other_bank_non_secure_digest;
    }

    LOG_INFO("Integrity module initialised\n");

    return status;
}

integrity_status_t INTEGRITY_Init(
    bool     bank_swap,
    uint8_t *current_bank_secure_digest,
    uint8_t *other_bank_secure_digest,
    uint8_t *current_bank_non_secure_digest,
    uint8_t *other_bank_non_secure_digest) {

    return _INTEGRITY_Init(&hintegrity, bank_swap, current_bank_secure_digest, other_bank_secure_digest, current_bank_non_secure_digest, other_bank_non_secure_digest);
}


/* ---------------------------------------------------------------------------- */
/* Hash Functions */
/* ---------------------------------------------------------------------------- */


static integrity_status_t _INTEGRITY_compute_secure_firmware_hash(integrity_handle_t *self, uint8_t bank) {

    integrity_status_t status    = HAL_OK;
    uint8_t           *start_ptr = NULL;
    uint32_t           size      = FLASH_S_REGION_SIZE + FLASH_NSC_REGION_SIZE;
    uint8_t           *digest;

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
    digest = (bank == FLASH_BANK_1) ? self->bank1_secure_digest : self->bank2_secure_digest;
    if (digest == NULL) status = INTEGRITY_PARAMETER_ERROR;
    if (status != INTEGRITY_OK) return status;

    /* Update self */
    if (bank == FLASH_BANK_1) {
        self->bank1_secure_digest_state = INTEGRITY_HASH_IN_PROGRESS;
    } else if (bank == FLASH_BANK_2) {
        self->bank2_secure_digest_state = INTEGRITY_HASH_IN_PROGRESS;
    }

    /* Start the hash */
    LOG_INFO("Starting hash\n");
    status = HAL_HASH_Start(&hhash, start_ptr, size, digest, 200);
    LOG_INFO("Finished hash\n");
    if (status != INTEGRITY_OK) {

        /* Update self */
        if (bank == FLASH_BANK_1) {
            self->bank1_secure_digest_state = INTEGRITY_HASH_ERROR;
        } else if (bank == FLASH_BANK_2) {
            self->bank2_secure_digest_state = INTEGRITY_HASH_ERROR;
        }

        return status;
    } else {

        /* Update self */
        if (bank == FLASH_BANK_1) {
            self->bank1_secure_digest_state = INTEGRITY_HASH_COMPLETE;
        } else if (bank == FLASH_BANK_2) {
            self->bank2_secure_digest_state = INTEGRITY_HASH_COMPLETE;
        }
    }

    return status;
}

integrity_status_t INTEGRITY_compute_secure_firmware_hash(uint8_t bank) {
    return _INTEGRITY_compute_secure_firmware_hash(&hintegrity, bank);
}


static uint8_t *_INTEGRITY_get_secure_firmware_hash(integrity_handle_t *self, uint8_t bank) {

    uint8_t *hash = NULL;

    if ((bank == FLASH_BANK_1) && (self->bank1_secure_digest_state == INTEGRITY_HASH_COMPLETE)) {
        hash = self->bank1_secure_digest;
    } else if ((bank == FLASH_BANK_2) && (self->bank2_secure_digest_state == INTEGRITY_HASH_COMPLETE)) {
        hash = self->bank2_secure_digest;
    }

    return hash;
}

uint8_t *INTEGRITY_get_secure_firmware_hash(uint8_t bank) {
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

    bool in_progress = true;

    if (HAL_HASH_GetState(&hhash) == HAL_HASH_STATE_BUSY) {
        in_progress = true;
    } else if (self->bank1_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        in_progress = true;
    } else if (self->bank2_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        in_progress = true;
    } else if (self->bank1_non_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        in_progress = true;
    } else if (self->bank2_non_secure_digest_state == INTEGRITY_HASH_IN_PROGRESS) {
        in_progress = true;
    } else {
        in_progress = false;
    }

    return in_progress;
}


bool INTEGRITY_get_hash_in_progress() {
    return _INTEGRITY_get_in_progress(&hintegrity);
}


/* ---------------------------------------------------------------------------- */
/* Signature Functions */
/* ---------------------------------------------------------------------------- */


/* TODO: Idk man */
bool INTEGRITY_check_non_secure_firmware_signature(uint8_t *hash) {

    PKA_ECDSAVerifInTypeDef in = {0};

    /* Curve parameters (same as in signing example) */
    in.primeOrderSize = prime256v1_Order_len;
    in.modulusSize    = prime256v1_Prime_len;
    in.coefSign       = prime256v1_A_sign;
    in.coef           = prime256v1_absA;
    in.modulus        = prime256v1_Prime;
    in.basePointX     = prime256v1_GeneratorX;
    in.basePointY     = prime256v1_GeneratorY;
    in.primeOrder     = prime256v1_Order;

    Error_Handler();
    /* Public key of signer (stored in secure flash) */
    //    in.pPubKeyCurvePtX = root_pubkey_x; // 32-byte array
    //    in.pPubKeyCurvePtY = root_pubkey_y; // 32-byte array
    //
    //    /* Signature values r and s (each 32-byte) */
    //    in.RSign = signature_r;
    //    in.SSign = signature_s;

    /* SHA-256 digest of your signed message (manifest = {version || fw_hash}) */
    in.hash = (uint8_t *) hash;

    /* This should take 2,938,000 clock cycles which is 11.8ms. TODO: use interrupts */
    HAL_PKA_ECDSAVerif(&hpka, &in, 200); /* TODO: Check return */

    return HAL_PKA_ECDSAVerif_IsValidSignature(&hpka);
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


void HAL_HASH_ErrorCallback(HASH_HandleTypeDef *hhash) {

    switch (HAL_HASH_GetError(hhash)) {
        case (HAL_HASH_ERROR_NONE): {
            LOG_ERROR("HAL_HASH_ERROR_NONE\n");
            break;
        }
        case (HAL_HASH_ERROR_BUSY): {
            LOG_ERROR("HAL_HASH_ERROR_BUSY\n");
            break;
        }
        case (HAL_HASH_ERROR_DMA): {
            switch (HAL_DMA_GetError(hhash->hdmain)) {
                case (HAL_DMA_ERROR_NONE): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_NONE\n");
                    break;
                }
                case (HAL_DMA_ERROR_DTE): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_DTE\n");
                    break;
                }
                case (HAL_DMA_ERROR_ULE): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_ULE\n");
                    break;
                }
                case (HAL_DMA_ERROR_USE): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_USE\n");
                    break;
                }
                case (HAL_DMA_ERROR_TO): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_TO\n");
                    break;
                }
                case (HAL_DMA_ERROR_TIMEOUT): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_TIMEOUT\n");
                    break;
                }
                case (HAL_DMA_ERROR_NO_XFER): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_NO_XFER\n");
                    break;
                }
                case (HAL_DMA_ERROR_BUSY): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_BUSY\n");
                    break;
                }
                case (HAL_DMA_ERROR_INVALID_CALLBACK): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_INVALID_CALLBACK\n");
                    break;
                }
                case (HAL_DMA_ERROR_NOT_SUPPORTED): {
                    LOG_ERROR("HAL_HASH_ERROR_DMA -> HAL_DMA_ERROR_NOT_SUPPORTED\n");
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
        case (HAL_HASH_ERROR_TIMEOUT): {
            LOG_ERROR("HAL_HASH_ERROR_TIMEOUT\n");
            break;
        }
#if (USE_HAL_HASH_REGISTER_CALLBACKS == 1U)
        case (HAL_HASH_ERROR_INVALID_CALLBACK): {
            LOG_ERROR("HAL_HASH_ERROR_INVALID_CALLBACK\n");
            break;
        }
#endif
        default:
            break;
    }
    Error_Handler();
}
