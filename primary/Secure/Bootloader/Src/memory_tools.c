/*
 * memory_tools.c
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#include "hal.h"
#include "main.h"

#include "memory_tools.h"
#include "logging.h"
#include "integrity.h"
#include "metadata.h"


void swap_banks() {
    // should also hardware erase the backup sram
    /* In debug mode don't actually swap banks */
#ifdef DEBUG
    Error_Handler();
#endif
}


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

    LOG_INFO("Backup domain initialised\n");
}


/* Other secure firmware check */
bool check_ns_firmware(uint8_t bank) {

    uint8_t status = 0;
    bool    valid  = true;

    /* Compute the hash */
    status = INTEGRITY_compute_ns_firmware_hash(bank);
    CHECK_STATUS_INTEGRITY(status);

    // TODO: check the other firmware properties in hmeta.

    /* Compare the computed hash with the stored hash */
    status = META_compare_ns_firmware_hash(&hmeta, bank, INTEGRITY_get_ns_firmware_hash(bank), &valid);
    CHECK_STATUS_META(status);

    /* Update hmeta */
    if (bank == FLASH_BANK_1) {
        hmeta.metadata.ns_firmware_1_valid = valid;
    } else if (bank == FLASH_BANK_2) {
        hmeta.metadata.ns_firmware_2_valid = valid;
    }

    /* Return if not valid */
    if (!valid) return valid;

    /* Check the signature of the non-secure firmware TODO: properly w/ manifest */
    INTEGRITY_check_ns_firmware_signature(bank);

    return valid;
}
