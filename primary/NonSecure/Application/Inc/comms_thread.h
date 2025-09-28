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


#include "stdint.h"
#include "tx_api.h"

#include "config.h"


/* Exported variables */
extern uint8_t      comms_thread_stack[COMMS_THREAD_STACK_SIZE];
extern TX_THREAD    comms_thread_handle;
extern uint8_t      zenoh_byte_pool_buffer[ZENOH_MEM_POOL_SIZE];
extern TX_BYTE_POOL zenoh_byte_pool;


void comms_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_COMMS_THREAD_H_ */
