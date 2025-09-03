/*
 * memory_tools.c
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#include "memory.h"
#include "hal.h"
#include "main.h"

#include "memory_tools.h"
#include "config.h"
#include "logging.h"
#include "integrity.h"
#include "metadata.h"


bool get_bank_swap() {
    FLASH_OBProgramInitTypeDef ob;
    HAL_FLASHEx_OBGetConfig(&ob);
    return ob.USERConfig & FLASH_OPTSR_SWAP_BANK;
}


void swap_banks() {
    // should also hardware erase the backup sram
    /* In debug mode don't actually swap banks */

	// TODO: WRITE THIS
#ifdef DEBUG
    while (1);
#endif

    HAL_NVIC_SystemReset();
}


/* Note this function is called before logging is started */
void enable_backup_domain(void) {

    /* Enable write access to backup domain */
    PWR_S->DBPCR |= PWR_DBPCR_DBP; /* Disable write protection */

    /* Enable clock to backup domain (updated __HAL_RCC_BKPRAM_CLK_ENABLE() for secure world) */
    __IO uint32_t tmpreg;
    SET_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);
    /* Delay after an RCC peripheral clock enabling */
    tmpreg = READ_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);
    UNUSED(tmpreg);

    /* Enable retention through sleep and power-off */
    PWR_S->BDCR |= PWR_BDCR_BREN;

    /* Enable voltage and temperature monitoring. TODO: Use this? */
    // PWR_S->BDCR  |= PWR_BDCR_MONEN;
}

/* Check secure firmware */
bool check_s_firmware(uint8_t bank) {

    uint8_t status = 0;
    bool    valid  = true;

    /* Make sure the valid flag is set */
    if (bank == FLASH_BANK_1) {
        valid = hmeta.metadata.s_firmware_1_valid;
    } else if (bank == FLASH_BANK_2) {
        valid = hmeta.metadata.s_firmware_2_valid;
    } else {
        valid = false;
    }
    if (!valid) return valid;

    /* Make sure secure firmwares are supposed to be identical */
    valid = memcmp(hmeta.metadata.s_firmware_1_hash, hmeta.metadata.s_firmware_2_hash, SHA256_SIZE) == 0;
    if (!valid) return valid;

    /* Compute the hash */
    status = INTEGRITY_compute_s_firmware_hash(bank);
    CHECK_STATUS_INTEGRITY(status);

    /* Compare the computed hash with the stored hash */
    status = META_check_s_firmware_hash(&hmeta, bank, INTEGRITY_get_s_firmware_hash(bank), &valid);
    CHECK_STATUS_META(status);

    /* Update hmeta */
    if (bank == FLASH_BANK_1) {
        hmeta.metadata.s_firmware_1_valid = valid;
    } else if (bank == FLASH_BANK_2) {
        hmeta.metadata.s_firmware_2_valid = valid;
    }

    return valid;
}

/* Non-secure firmware check */
bool check_ns_firmware(uint8_t bank) {

    uint8_t status = 0;
    bool    valid  = true;

    /* Make sure the valid flag is set */
    if (bank == FLASH_BANK_1) {
        valid = hmeta.metadata.ns_firmware_1_valid;
    } else if (bank == FLASH_BANK_2) {
        valid = hmeta.metadata.ns_firmware_2_valid;
    } else {
        valid = false;
    }
    if (!valid) return valid;

    /* Compute the hash */
    status = INTEGRITY_compute_ns_firmware_hash(bank);
    CHECK_STATUS_INTEGRITY(status);

    /* Compare the computed hash with the stored hash */
    status = META_check_ns_firmware_hash(&hmeta, bank, INTEGRITY_get_ns_firmware_hash(bank), &valid);
    CHECK_STATUS_META(status);

    /* Update hmeta */
    if (bank == FLASH_BANK_1) {
        hmeta.metadata.ns_firmware_1_valid = valid;
    } else if (bank == FLASH_BANK_2) {
        hmeta.metadata.ns_firmware_2_valid = valid;
    }

    return valid;
}

bool copy_s_firmware_to_other_bank(bool bank_swap) {

    /* Don't write when debugging */
#ifdef DEBUG
    return true;
#endif

    if (!bank_swap) {

        /* Set invalid before copying */
        hmeta.metadata.s_firmware_2_valid = false;

        /* Do the copy */
        memcpy((void *) FLASH_S_BANK2_BASE_ADDR, (void *) FLASH_S_BANK1_BASE_ADDR, FLASH_S_REGION_SIZE);

        /* Set what the hash should be */
        META_set_s_firmware_hash(&hmeta, FLASH_BANK_2, INTEGRITY_get_ns_firmware_hash(FLASH_BANK_1));

        /* Check the hash and return */
        hmeta.metadata.s_firmware_2_valid = check_s_firmware(FLASH_BANK_2);
        return hmeta.metadata.s_firmware_2_valid;
    } else {

        /* Set invalid before copying */
        hmeta.metadata.s_firmware_1_valid = false;

        /* Do the copy */
        memcpy((void *) FLASH_S_BANK1_BASE_ADDR, (void *) FLASH_S_BANK2_BASE_ADDR, FLASH_S_REGION_SIZE);

        /* Set what the hash should be */
        META_set_s_firmware_hash(&hmeta, FLASH_BANK_1, INTEGRITY_get_ns_firmware_hash(FLASH_BANK_2));

        /* Check the hash and return */
        hmeta.metadata.s_firmware_1_valid = check_s_firmware(FLASH_BANK_1);
        return hmeta.metadata.s_firmware_1_valid;
    }
}

bool copy_ns_firmware_to_other_bank(bool bank_swap) {
    if (!bank_swap) {
        return copy_ns_firmware(FLASH_BANK_1, FLASH_BANK_2);
    } else {
        return copy_ns_firmware(FLASH_BANK_2, FLASH_BANK_1);
    }
}

bool copy_ns_firmware(uint8_t from_bank, uint8_t to_bank) {

    /* Don't write when debugging */
#ifdef DEBUG
    return true;
#endif

    /* Non-swapped banks */
    if ((from_bank == FLASH_BANK_1) && (to_bank == FLASH_BANK_2)) {

        /* Set invalid before copying */
        hmeta.metadata.ns_firmware_2_valid = false;

        /* Do the copy */
        memcpy((void *) FLASH_NS_BANK2_BASE_ADDR, (void *) FLASH_NS_BANK1_BASE_ADDR, FLASH_NS_REGION_SIZE);

        /* Set what the hash should be */
        META_set_ns_firmware_hash(&hmeta, FLASH_BANK_2, INTEGRITY_get_ns_firmware_hash(FLASH_BANK_1));

        /* Check the hash and return */
        hmeta.metadata.ns_firmware_2_valid = check_ns_firmware(FLASH_BANK_2);
        return hmeta.metadata.ns_firmware_2_valid;
    }

    /* Swapped banks */
    else if ((from_bank == FLASH_BANK_2) && (to_bank == FLASH_BANK_1)) {

        /* Set invalid before copying */
        hmeta.metadata.ns_firmware_1_valid = false;

        /* Do the copy */
        memcpy((void *) FLASH_NS_BANK1_BASE_ADDR, (void *) FLASH_NS_BANK2_BASE_ADDR, FLASH_NS_REGION_SIZE);

        /* Set what the hash should be */
        META_set_ns_firmware_hash(&hmeta, FLASH_BANK_1, INTEGRITY_get_ns_firmware_hash(FLASH_BANK_2));

        /* Check the hash and return */
        hmeta.metadata.ns_firmware_1_valid = check_ns_firmware(FLASH_BANK_1);
        return hmeta.metadata.ns_firmware_1_valid;
    }

    /* Invalid bank numbers */
    else {
        return false;
    }

    return true;
}
