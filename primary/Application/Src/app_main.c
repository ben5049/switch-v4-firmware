/*
 * app_main.c
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 */

#include "app_main.h"
#include "switch_thread.h"
#include "stp_thread.h"

/* This function should be called once in App_ThreadX_Init */
void tx_user_init(){
	tx_mutex_create(&sja1105_mutex_ptr, "sja1105_mutex", TX_INHERIT);
	tx_thread_create(&switch_thread_ptr, "switch_thread", switch_thread_entry, 0, switch_thread_stack, SWITCH_THREAD_STACK_SIZE, SWITCH_THREAD_PRIORIY, SWITCH_THREAD_PREMPTION_PRIORIY, 1, TX_AUTO_START);
	tx_thread_create(&stp_thread_ptr, "stp_thread", stp_thread_entry, 0, stp_thread_stack, STP_THREAD_STACK_SIZE, STP_THREAD_PRIORIY, STP_THREAD_PREMPTION_PRIORIY, 1, TX_AUTO_START);
}
