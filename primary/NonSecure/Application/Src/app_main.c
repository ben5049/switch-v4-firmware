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
}
