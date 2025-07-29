/*
 * switch.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_SWITCH_THREAD_H_
#define INC_SWITCH_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"
#include "stdint.h"
#include "stm32h5xx_hal.h"

#include "app_main.h"

/* Exported variables */
extern uint8_t switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
extern TX_THREAD switch_thread_ptr;
extern TX_MUTEX sja1105_mutex_ptr;

/* Exported functions*/
void switch_thread_entry(uint32_t initial_input);

/* Imported variables */
extern SPI_HandleTypeDef hspi2;


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_THREAD_H_ */
