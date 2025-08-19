/*
 * boot_main.c
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#include "crc.h"
#include "flash.h"
#include "ramcfg.h"
#include "sau.h"
#include "spi.h"
#include "gpio.h"

#include "boot_main.h"
#include "config.h"
#include "integrity.h"
#include "metadata.h"


metadata_handle_t hmeta;


void boot_main() {

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
    MX_CRC_Init();
    MX_GPIO_Init();
    MX_FLASH_Init();
    MX_SAU_Init();
    MX_RAMCFG_Init();
    MX_SPI1_Init();

    /* Calculate the CRCs while waiting for the FRAM to be available */
//    uint32_t current_bank_secure_crc = calculate_secure_crc(true);
//    uint32_t other_bank_secure_crc   = calculate_secure_crc(false);

    /* Initialised the metadata struct (controls FRAM) */
    if (META_Init(&hmeta) != META_OK) {
        Error_Handler();
    }

    /* If this is the first boot then configure the device and metadata */
    if (hmeta.first_boot) {
        // TODO:
    }

    /* Else this isn't the first boot, proceed */
    else {
        // TODO:
    }
}
