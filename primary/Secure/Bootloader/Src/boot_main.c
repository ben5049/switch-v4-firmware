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
#include "error.h"


uint8_t __attribute__((section(".LOG_Section"))) log_buffer[LOG_BUFFER_SIZE];


void boot_main() {

    static uint8_t status;

    __ALIGN_BEGIN static uint8_t current_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;
    __ALIGN_BEGIN static uint8_t other_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;
    __ALIGN_BEGIN static uint8_t current_non_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;
    __ALIGN_BEGIN static uint8_t other_non_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;

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

    /* Enable logging */
    status = log_init(&hlog, log_buffer, LOG_BUFFER_SIZE);
    CHECK_STATUS(status, LOG_OK, ERROR_LOG);
    LOG_INFO("Starting secure firmware\n");

    /* Enable writing to the backup SRAM */
    enable_backup_domain();

    /* Enable ECC interrupts */
    status = HAL_RAMCFG_EnableNotification(&hramcfg_SRAM2, RAMCFG_IT_ALL);
    CHECK_STATUS(status, HAL_OK, ERROR_HAL);
    status = HAL_RAMCFG_EnableNotification(&hramcfg_SRAM3, RAMCFG_IT_ALL);
    CHECK_STATUS(status, HAL_OK, ERROR_HAL);
    status = HAL_RAMCFG_EnableNotification(&hramcfg_BKPRAM, RAMCFG_IT_ALL);
    CHECK_STATUS(status, HAL_OK, ERROR_HAL);

    /* TODO: Get bank swap (from option bytes?) */
    bool bank_swap = false;

    /* Check if the metadata module is already initialised (CPU reset, but memory persisted) */
    if (hmeta.initialised == true) {
        status = META_Reinit(&hmeta, bank_swap);
        CHECK_STATUS(status, META_OK, ERROR_META);
    }

    /* Initialise the metadata module (uses FRAM over SPI1) */
    else {
        status = META_Init(&hmeta, bank_swap);
        CHECK_STATUS(status, META_OK, ERROR_META);
    }

    /* Check if the last boot was a crash. Startup in reduced mode */
    if (hmeta.metadata.crashed) {
        /* TODO: If last shutdown was a crash, do something with messages */
    }

    /* Initialise the integrity module and pass in empty buffers for hash digests */
    status = INTEGRITY_Init(bank_swap, current_secure_firmware_hash, other_secure_firmware_hash, current_non_secure_firmware_hash, other_non_secure_firmware_hash);
    CHECK_STATUS(status, INTEGRITY_OK, ERROR_INTEGRITY);

    /* Calculate the SHA256 of the current secure firmware */
    status = INTEGRITY_compute_secure_firmware_hash(CURRENT_FLASH_BANK(bank_swap));
    CHECK_STATUS(status, INTEGRITY_OK, ERROR_INTEGRITY);
    LOG_INFO_SHA256("Secure firmware hash = %s\n", current_secure_firmware_hash);

    /* If this is the first boot then configure the device and metadata */
    if (hmeta.first_boot) {
        status = META_Configure(&hmeta, current_secure_firmware_hash);
        CHECK_STATUS(status, META_OK, ERROR_META);

        // TODO: copy current secure and non-secure firmwares into other bank and check they were written correctly
        while (1);
    }

    /* Else this isn't the first boot */
    else {

        /* Check the current secure firmware hasn't been corrupted or tampered with */
        bool identical = true;
        status         = META_compare_secure_firmware_hash(&hmeta, CURRENT_FLASH_BANK(bank_swap), current_secure_firmware_hash, &identical);
        CHECK_STATUS(status, META_OK, ERROR_META);

        /* If the secure firmware has been corrupted or tampered with then swap banks if the other bank is valid.
         * Normally this shouldn't return as it needs a system reset for swapping banks take effect. While debugging
         * ignore hash errors and continue as if valid */
#ifndef DEBUG
        if (!identical) {

            // TODO: Check other bank is valid

            swap_banks();
        }
#endif

        /* Check the other firmwares haven't been corrupted or tampered with */

        // TODO: check the other secure firmware properties in hmeta.
        // If it has the same hash as this secure firmware then check that is true (set as valid or copy self into other)
        // If different or marked as invalid then copy self into other
        // Compute hash of secure other to make sure copy was successful
        status = INTEGRITY_compute_secure_firmware_hash(OTHER_FLASH_BANK(bank_swap));
        CHECK_STATUS(status, META_OK, ERROR_INTEGRITY);
        status = META_compare_secure_firmware_hash(&hmeta, OTHER_FLASH_BANK(bank_swap), other_secure_firmware_hash, &identical);
        CHECK_STATUS(status, META_OK, ERROR_META);

        // Follow above steps for non-secure firmwares
        status = INTEGRITY_compute_non_secure_firmware_hash(CURRENT_FLASH_BANK(bank_swap));
        CHECK_STATUS(status, META_OK, ERROR_INTEGRITY);
        status = INTEGRITY_compute_non_secure_firmware_hash(OTHER_FLASH_BANK(bank_swap));
        CHECK_STATUS(status, META_OK, ERROR_INTEGRITY);


        // TODO:
    }
}
