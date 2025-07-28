/*
 * switch.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_SWITCH_H_
#define INC_SWITCH_H_

#include "tx_api.h"
#include "stdint.h"
#include "stm32h5xx_hal.h"

#include "app_main.h"

/* Exported variables */
extern uint8_t switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
extern TX_THREAD switch_thread_ptr;

/* Exported functions*/
VOID switch_thread_entry(ULONG initial_input);

extern SPI_HandleTypeDef hspi2;

#endif /* INC_SWITCH_H_ */
