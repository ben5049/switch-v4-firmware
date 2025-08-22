/*
 * boot_main.c
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdbool.h"
#include "gpio.h"
#include "gpdma.h"
#include "flash.h"
#include "sau.h"
#include "ramcfg.h"
#include "spi.h"
#include "rng.h"
#include "hash.h"
#include "pka.h"
#include "usart.h"
#include "aes.h"

#include "boot_main.h"
#include "config.h"
#include "integrity.h"
#include "metadata.h"
#include "utils.h"
#include "memory_tools.h"
#include "logging.h"


void boot_main() {

    __ALIGN_BEGIN static uint8_t current_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;

    /* Step 1: Initialise peripherals
     * Step 2: Get previous secure firmware version and CRC from FRAM for current bank
     *   Step 2a: If current version is newer then compute CRC of the secure region (FLASH_S and FLASH_NSC) and store
     *   Step 2b: If current version is the same or older or then compute CRC of FLASH_S and FLASH_NSC and compare. If corrupted trigger a bank swap (back to step 1)
     * Step 3: Get previous secure firmware version and CRC from FRAM for other bank
     *   Step 3a: If current version is newer then copy current bank secure region into other bank
     *   Step 3b: If current version is the same compute the CRC of the other bank. If corrupted then copy current bank secure region into other bank
     *   Step 3c: If current version is older then something is wrong. Trigger bank swap and it will force the other bank to repeat these steps and update current bank
     * Step 4: Compute the CRC of both non-secure regions and compare with stored value.
     *   Step 4a: If other isn't corrupted and current isn't then continue
     *   Step 4b: If other is corrupted and current isn't then copy current into other
     *   Step 4c: If other isn't corrupted and current is then copy other into current
     *   Step 4d: If both are corrupted then panic
     */

    /* Initialise peripherals */
    MX_GPIO_Init();
    MX_GPDMA1_Init();
    MX_FLASH_Init();
    MX_SAU_Init();
    MX_RAMCFG_Init();
    MX_SPI1_Init();
    MX_RNG_Init();
    MX_HASH_Init();
    MX_PKA_Init();
    MX_UART4_Init();
    MX_SAES_AES_Init();

    LOG_INFO("Starting secure firmware\n");

    enable_backup_domain();

    /* Enable ECC interrupts */
    if (HAL_RAMCFG_EnableNotification(&hramcfg_SRAM2, RAMCFG_IT_ALL) != HAL_OK) Error_Handler();
    if (HAL_RAMCFG_EnableNotification(&hramcfg_SRAM3, RAMCFG_IT_ALL) != HAL_OK) Error_Handler();
    if (HAL_RAMCFG_EnableNotification(&hramcfg_BKPRAM, RAMCFG_IT_ALL) != HAL_OK) Error_Handler();

    /* TODO: Get bank swap (from option bytes?) */
    bool bank_swap = false;

    /* Initialise the integrity module */
    if (INTEGRITY_Init(bank_swap, current_secure_firmware_hash, NULL, NULL, NULL) != INTEGRITY_OK) Error_Handler();

    /* Start calculating the SHA256 of the current secure firmware (non-blocking, uses DMA) */
    if (INTEGRITY_compute_secure_firmware_hash(CURRENT_FLASH_BANK(bank_swap)) != INTEGRITY_OK) Error_Handler();
    LOG_INFO("Secure firmware hash = ");
    LOG_INFO_SHA256(current_secure_firmware_hash);

    /* Initialise the metadata module (uses FRAM over SPI1) */
    if (META_Init(&hmeta, bank_swap) != META_OK) Error_Handler();

    /* If this is the first boot then configure the device and metadata */
    if (hmeta.first_boot) {

        if (META_Configure(&hmeta, current_secure_firmware_hash) != META_OK) Error_Handler();

        // TODO: copy current secure firmware into other bank and check it was written correctly

        while (1);
    }

    /* Else this isn't the first boot, proceed */
    else {

        /* Check the current secure firmware hasn't been corrupted or tampered with */
        bool identical = false;
        if (META_compare_secure_firmware_hash(&hmeta, CURRENT_FLASH_BANK(bank_swap), current_secure_firmware_hash, &identical) != META_OK) Error_Handler();

        /* If the secure firmware has been corrupted or tampered with then swap banks if the other bank is valid.
         * Normally this shouldn't return as it needs a system reset for swapping banks take effect. While debugging
         * ignore hash errors and continue as if valid */
#ifndef DEBUG
        if (!identical) {

            // TODO: Check other bank is valid

            swap_banks();
        }
#endif


        while (1);

        // TODO:
    }
}
