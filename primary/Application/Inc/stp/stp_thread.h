/*
 * stp_thread.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_STP_THREAD_H_
#define INC_STP_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "tx_api.h"

#include "config.h"


/* Exported variables */
extern uint8_t stp_thread_stack[STP_THREAD_STACK_SIZE];
extern TX_THREAD stp_thread_ptr;

/* Exported functions*/
void stp_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_STP_THREAD_H_ */
