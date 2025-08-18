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
#include "usart.h"
#include "gpio.h"

#include "app_main.h"


// int main(void) {

//     /* -------------------- MCU Configuration -------------------- */

//     /* Reset of all peripherals, Initialises the Flash interface and the Systick */
//     HAL_Init();

//     /* Configure the system clock */
//     SystemClock_Config();

//     /* Initialise all configured peripherals */
//     MX_GPIO_Init();
//     MX_SPI1_Init();
//     MX_SPI2_Init();
//     MX_UART4_Init();
//     MX_ETH_Init();
//     MX_ICACHE_Init();
//     MX_CRC_Init();
//     MX_DTS_Init();

//     /* Initialise the RTOS */
//     MX_ThreadX_Init();

//     /* We should never get here as control is now taken by the scheduler */
//     while (1);
// }
