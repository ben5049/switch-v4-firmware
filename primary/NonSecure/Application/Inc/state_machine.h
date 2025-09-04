/*
 * state_machine.h
 *
 *  Created on: Sep 3, 2025
 *      Author: bens1
 */

#ifndef INC_STATE_MACHINE_H_
#define INC_STATE_MACHINE_H_


#include "stdint.h"
#include "tx_api.h"

#include "config.h"


#define STATE_MACHINE_ALL_EVENTS               ((ULONG) 0xffffffff)
#define STATE_MACHINE_SWITCH_INITIALISED_EVENT ((ULONG) 1 << 0)
#define STATE_MACHINE_NX_INITIALISED_EVENT     ((ULONG) 1 << 0)


extern TX_THREAD            state_machine_thread_handle;
extern uint8_t              state_machine_thread_stack[STATE_MACHINE_THREAD_STACK_SIZE];
extern TX_EVENT_FLAGS_GROUP state_machine_events_handle;


void state_machine_thread_entry(uint32_t initial_input);


#endif /* INC_STATE_MACHINE_H_ */
