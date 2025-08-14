/*
 * phy_thread.h
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#ifndef INC_PHY_THREAD_H_
#define INC_PHY_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "tx_api.h"

#include "config.h"


extern uint8_t   phy_thread_stack[PHY_THREAD_STACK_SIZE];
extern TX_THREAD phy_thread_handle;

void phy_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_PHY_THREAD_H_ */
