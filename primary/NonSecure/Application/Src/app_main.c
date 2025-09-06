/*
 * app_main.c
 *
 *  Created on: 17 Aug 2025
 *      Author: bens1
 */

#include "main.h"
#include "app_threadx.h"
#include "main.h"
#include "crc.h"
#include "dts.h"
#include "eth.h"
#include "icache.h"
#include "spi.h"
#include "gpio.h"
#include "gpdma.h"
#include "aes.h"

#include "app_main.h"
#include "switch_thread.h"
#include "utils.h"


void app_main(void) {

    /* Make sure shared structs start uninitialised */
    hsja1105.initialised = false;
    // TODO: same with PHYs

    /* Initialise all configured peripherals */
    MX_GPIO_Init();
    MX_GPDMA1_Init();
    MX_CRC_Init();
    MX_DTS_Init();
    MX_ICACHE_Init();
    MX_SPI2_Init();
    MX_AES_Init();

    /* Change to FPWM mode for more accurate 3.3V rail. TODO: make into function */
    __disable_irq();
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, RESET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, SET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, RESET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, SET);
    __enable_irq();

    /* Initialise the switch */
    if (switch_init(&hsja1105) != SJA1105_OK) Error_Handler();

    /* Ethernet MAC can now be initialised (requires switch REFCLK) */
    MX_ETH_Init();
}
