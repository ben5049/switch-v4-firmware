/*
 * comms_thread.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#include "zenoh-pico.h"
#include "comms_thread.h"
#include "utils.h"


uint8_t comms_thread_stack[COMMS_THREAD_STACK_SIZE];
TX_THREAD comms_thread_ptr;


void comms_thread_entry(uint32_t initial_input){

    while (1){
        tx_thread_sleep_ms(1000);
    }
}
