/*
 * app_main.c
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 */

#include "app_main.h"
#include "switch.h"


/* This function should be called once in App_ThreadX_Init */
void create_threads(){
	tx_thread_create(&switch_thread_ptr, "switch_thread", switch_thread_entry, 0, switch_thread_stack, SWITCH_THREAD_STACK_SIZE, SWITCH_THREAD_PRIORIY, SWITCH_THREAD_PREMPTION_PRIORIY, 1, TX_AUTO_START);
}
