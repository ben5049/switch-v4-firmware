/*
 * comms.h
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#ifndef INC_COMMS_THREAD_H_
#define INC_COMMS_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"
#include "stdint.h"
#include "hal.h"

#include "app_main.h"

/* Exported variables */
extern uint8_t   comms_thread_stack[COMMS_THREAD_STACK_SIZE];
extern TX_THREAD comms_thread_ptr;

void comms_thread_entry(uint32_t initial_input);

#ifdef __cplusplus
}
#endif

#endif /* INC_COMMS_THREAD_H_ */
