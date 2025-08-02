/*
 * stp_thread.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "hal.h"
#include "nx_api.h"

#include "stp.h"

#include "stp_thread.h"
#include "stp_callbacks.h"
#include "config.h"
#include "utils.h"


uint8_t stp_thread_stack[STP_THREAD_STACK_SIZE];
TX_THREAD stp_thread_ptr;

volatile uint32_t stp_error_counter = 0;

STP_BRIDGE* bridge;


void stp_thread_entry(uint32_t initial_input){

	static constexpr uint8_t mac_address[6] = {
		MAC_ADDR_OCTET1,
		MAC_ADDR_OCTET2,
		MAC_ADDR_OCTET3,
		MAC_ADDR_OCTET4,
		MAC_ADDR_OCTET5,
		MAC_ADDR_OCTET6
	};

    /* Set the ethernet packet parameters to default */
    bpdu_packet_init();

	bridge = STP_CreateBridge(5, 0, 16, &stp_callbacks, mac_address, 100);
	STP_StartBridge (bridge, tx_time_get_ms());

	/* Setup timing control variables */
    uint32_t current_time = tx_time_get();
    uint32_t next_wake_time = current_time + TX_TIMER_TICKS_PER_SECOND;

    while (1){

        /* Sleep until the next wake time */
    	current_time = tx_time_get_ms();
        if (current_time < next_wake_time){
            tx_thread_sleep(next_wake_time - current_time);
        }

        /* Schedule next wake time in one second */
        next_wake_time += TX_TIMER_TICKS_PER_SECOND;

        /* Do any necessary STP work */
		STP_OnOneSecondTick(bridge, tx_time_get_ms());
	}
}
