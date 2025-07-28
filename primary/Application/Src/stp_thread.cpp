/*
 * stp_thread.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "stp_thread.h"
#include "stp.h"
#include "config.h"
#include "utils.h"


uint8_t stp_thread_stack[STP_THREAD_STACK_SIZE];
TX_THREAD stp_thread_ptr;

static constexpr uint8_t fallback_address[6] = { 0x10, 0x20, 0x30, 0x40, 0x55, 0x60 };

static const uint8_t bpdu_dest_address[] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x00 };
static const uint8_t bpdu_llc[3] = { 0x42, 0x42, 0x03 };

STP_BRIDGE* bridge;


static const STP_CALLBACKS stp_callbacks =
{
	.enableBpduTrapping    = nullptr,
	.enableLearning        = nullptr,
	.enableForwarding      = nullptr,
	.transmitGetBuffer     = nullptr,
	.transmitReleaseBuffer = nullptr,
	.flushFdb              = nullptr,
	.debugStrOut           = nullptr,
	.onTopologyChange      = nullptr,
	.onPortRoleChanged     = nullptr,
	.allocAndZeroMemory    = nullptr,
	.freeMemory            = nullptr,
};



void stp_thread_entry(uint32_t initial_input){

	static constexpr uint8_t mac_address[6] = {
		MAC_ADDR_OCTET1,
		MAC_ADDR_OCTET2,
		MAC_ADDR_OCTET3,
		MAC_ADDR_OCTET4,
		MAC_ADDR_OCTET5,
		MAC_ADDR_OCTET6
	};

	bridge = STP_CreateBridge(5, 0, 16, &stp_callbacks, mac_address, 100);
	STP_StartBridge (bridge, tx_time_get_ms());

	/* Setup timing control variables */
    uint32_t current_time = tx_time_get();
    uint32_t next_wake_time = current_time + TX_TIMER_TICKS_PER_SECOND;

    while (1){

        /* Sleep until the next wake time */
    	current_time = tx_time_get();
        if (current_time < next_wake_time){
            tx_thread_sleep(next_wake_time - current_time);
        }

        /* Schedule next wake time in one second */
        next_wake_time += TX_TIMER_TICKS_PER_SECOND;

        /* Do any necessary STP work */
		STP_OnOneSecondTick(bridge, tx_time_get_ms());
	}
}
