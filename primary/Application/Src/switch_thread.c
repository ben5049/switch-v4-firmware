/*
 * switch.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "switch_thread.h"
#include "sja1105.h"
#include "sja1105q_default_conf.h"
#include "utils.h"


#define PORT_88Q2112_PHY0 0
#define PORT_88Q2112_PHY1 1
#define PORT_88Q2112_PHY2 2
#define PORT_LAN8671_PHY  3
#define PORT_HOST         4

uint8_t switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_ptr;
TX_MUTEX sja1105_mutex_ptr;

/* Imported variables */
extern SPI_HandleTypeDef hspi2;

static void sja1105_delay_ms(uint32_t ms){
	tx_thread_sleep_ms(ms);
}

static void sja1105_delay_ns(uint32_t ns){

	/* CPU runs at 250MHz so one instruction is 4ns.
	 * The loop contains a NOP, ADDS, CMP and branch instruction per cycle.
	 * This means the loop delay is 4 * 4ns = 16ns.
	 * This is true for O3 but will take longer for O0.
	 */
	for (uint32_t t = 0; t < ns; t += 16){
		__NOP();
	}
}

static SJA1105_StatusTypeDef sja1105_take_mutex(uint32_t timeout){

	SJA1105_StatusTypeDef status = SJA1105_OK;

	switch (tx_mutex_get(&sja1105_mutex_ptr, MS_TO_TICKS(timeout))){
	case TX_SUCCESS:
		status = SJA1105_OK;
		break;
	case TX_NOT_AVAILABLE:
		status = SJA1105_BUSY;
		break;
	default:
		status = SJA1105_ERROR;
		break;
	}

	return status;
}

static SJA1105_StatusTypeDef sja1105_give_mutex(void){

	SJA1105_StatusTypeDef status = SJA1105_OK;

	if (tx_mutex_put(&sja1105_mutex_ptr) != TX_SUCCESS) status = SJA1105_ERROR;

	return status;
}

static const SJA1105_CallbacksTypeDef sja1105_callbacks = {
		.callback_delay_ms   = &sja1105_delay_ms,
		.callback_delay_ns   = &sja1105_delay_ns,
		.callback_take_mutex = &sja1105_take_mutex,
		.callback_give_mutex = &sja1105_give_mutex
};


void switch_thread_entry(uint32_t initial_input){

	static SJA1105_HandleTypeDef hsja1105;
    static SJA1105_ConfigTypeDef sja1105_conf;
	static SJA1105_PortTypeDef   sja1105_ports[SJA1105_NUM_PORTS];

	static int16_t temp_x10;

    /* Set the general switch parameters */
	sja1105_conf.variant    = VARIANT_SJA1105Q;
    sja1105_conf.spi_handle = &hspi2;
    sja1105_conf.cs_port    = SWCH_CS_GPIO_Port;
    sja1105_conf.cs_pin     = SWCH_CS_Pin;
    sja1105_conf.rst_port   = SWCH_RST_GPIO_Port;
    sja1105_conf.rst_pin    = SWCH_RST_Pin;
    sja1105_conf.timeout    = 100;

	/* Configure port speeds and interfaces */
	if (SJA1105_ConfigurePort(sja1105_ports, PORT_88Q2112_PHY0, SJA1105_INTERFACE_RGMII, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8) != SJA1105_OK) Error_Handler();
	if (SJA1105_ConfigurePort(sja1105_ports, PORT_88Q2112_PHY1, SJA1105_INTERFACE_RGMII, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8) != SJA1105_OK) Error_Handler();
	if (SJA1105_ConfigurePort(sja1105_ports, PORT_88Q2112_PHY2, SJA1105_INTERFACE_RGMII, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8) != SJA1105_OK) Error_Handler();
	if (SJA1105_ConfigurePort(sja1105_ports, PORT_LAN8671_PHY,  SJA1105_INTERFACE_RMII,  SJA1105_SPEED_10M,     SJA1105_IO_3V3) != SJA1105_OK) Error_Handler();
	if (SJA1105_ConfigurePort(sja1105_ports, PORT_HOST,         SJA1105_INTERFACE_RMII,  SJA1105_SPEED_100M,    SJA1105_IO_3V3) != SJA1105_OK) Error_Handler();

    /* Initialise the switch */
	if (SJA1105_Init(&hsja1105, &sja1105_conf, sja1105_ports, &sja1105_callbacks, sja1105_static_conf, SJA1105_STATIC_CONF_SIZE) != SJA1105_OK) Error_Handler();

    /* Set the speed of the dynamic ports. TODO: This should be after PHY auto-negotiaion */
    if (SJA1105_UpdatePortSpeed(&hsja1105, PORT_88Q2112_PHY0, SJA1105_SPEED_1G)) Error_Handler();
    if (SJA1105_UpdatePortSpeed(&hsja1105, PORT_88Q2112_PHY1, SJA1105_SPEED_1G)) Error_Handler();
    if (SJA1105_UpdatePortSpeed(&hsja1105, PORT_88Q2112_PHY2, SJA1105_SPEED_1G)) Error_Handler();

	while (1){
        tx_thread_sleep_ms(100);
	    SJA1105_ReadTemperatureX10(&hsja1105, &temp_x10);
	}
}
