/*
 * switch.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "switch_thread.h"
#include "sja1105.h"
#include "utils.h"


uint8_t switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_ptr;

SJA1105_HandleTypeDef hsja1105;


static void sja1105_delay_ms(uint32_t ms){
	tx_thread_sleep_ms(ms);
}

static SJA1105_StatusTypeDef sja1105_take_mutex(uint32_t timeout){
	return SJA1105_OK;
}

static SJA1105_StatusTypeDef sja1105_give_mutex(void){
	return SJA1105_OK;
}

static const SJA1105_CallbacksTypeDef sja1105_callbacks = {
	.callback_delay_ms   = &sja1105_delay_ms,
	.callback_take_mutex = &sja1105_take_mutex,
	.callback_give_mutex = &sja1105_give_mutex
};


void switch_thread_entry(uint32_t initial_input){

	if (SJA1105_Init(
		&hsja1105,
		VARIANT_SJA1105Q,
		&sja1105_callbacks,
		&hspi2,
		SWCH_CS_GPIO_Port,
		SWCH_CS_Pin,
		SWCH_RST_GPIO_Port,
		SWCH_RST_Pin,
		100
	) != SJA1105_OK) Error_Handler();

	while (1){
		tx_thread_sleep_ms(100);
	}
}
