/*
 * app_setup.c
 *
 *  Created on: 17 Aug 2025
 *      Author: bens1
 */

#include <app_setup.h>
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

#include "switch_thread.h"
#include "phy_thread.h"
#include "utils.h"


void app_setup(void) {

    /* Reset shared structs */
    memset(&hsja1105, 0, sizeof(sja1105_handle_t));
    memset(&hphy0, 0, sizeof(phy_handle_88q211x_t));
    memset(&hphy1, 0, sizeof(phy_handle_88q211x_t));
    memset(&hphy2, 0, sizeof(phy_handle_88q211x_t));
    memset(&hphy3, 0, sizeof(phy_handle_lan867x_t)); // TODO:

    /* Initialise important peripherals */
    MX_GPIO_Init();
    MX_ICACHE_Init();
    MX_CRC_Init();
    MX_SPI2_Init();

    /* Change to FPWM mode for more accurate 3.3V rail (needed by PHYs) */
    set_3v3_regulator_to_FPWM();

    /* Initialise the switch */
    sja1105_status_t switch_status = switch_init(&hsja1105);
    if (switch_status != SJA1105_OK) Error_Handler();

    /* Ethernet MAC can now be initialised (requires switch REFCLK) */
    MX_ETH_Init();

    /* Initialise less important peripherals */
    MX_GPDMA1_Init();
    MX_DTS_Init();
    MX_AES_Init();
}
