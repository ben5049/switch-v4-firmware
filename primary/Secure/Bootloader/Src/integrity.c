/*
 * integrity.c
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdbool.h"
#include "hal.h"
#include "crc.h"

#include "integrity.h"
#include "config.h"


extern CRC_HandleTypeDef hcrc;


uint32_t calculate_secure_crc(bool current_bank) {
    if (current_bank) {
        return HAL_CRC_Calculate(&hcrc, (uint32_t *) FLASH_S_BANK1_BASE_ADDR, (FLASH_S_REGION_SIZE + FLASH_NSC_REGION_SIZE) / sizeof(uint32_t));
    } else {
        return HAL_CRC_Calculate(&hcrc, (uint32_t *) FLASH_S_BANK2_BASE_ADDR, (FLASH_S_REGION_SIZE + FLASH_NSC_REGION_SIZE) / sizeof(uint32_t));
    }
}