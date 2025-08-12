/*
 * tx_app.c
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 */

#include "tx_app.h"
#include "nx_app.h"
#include "nx_ptp.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "stp_thread.h"
#include "comms_thread.h"
#include "config.h"


/* This function should be called once in App_ThreadX_Init */
void tx_user_init(void *memory_ptr) {
    tx_mutex_create(&sja1105_mutex_ptr, "sja1105_mutex", TX_INHERIT);

    tx_semaphore_create(&dhcp_semaphore_ptr, "dhcp_semaphore", 0);

    tx_event_flags_create(&stp_events, "stp_events");

    tx_queue_create(&nx_ptp_tx_queue_ptr, "nx_ptp_tx_queue", sizeof(nx_ptp_tx_info_t), nx_ptp_tx_queue_stack, NX_PTP_TX_QUEUE_SIZE);

    tx_thread_create(&nx_app_thread_ptr, "nx_app_thread", nx_app_thread_entry, 0, nx_app_thread_stack, NX_APP_THREAD_STACK_SIZE, NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&nx_link_thread_ptr, "nx_link_thread", nx_link_thread_entry, 0, nx_link_thread_stack, NX_LINK_THREAD_STACK_SIZE, NX_LINK_THREAD_PRIORITY, NX_LINK_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&nx_ptp_tx_thread_ptr, "nx_ptp_tx_thread", nx_ptp_tx_thread_entry, 0, nx_ptp_tx_thread_stack, NX_PTP_TX_THREAD_STACK_SIZE, NX_PTP_TX_THREAD_PRIORITY, NX_PTP_TX_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);
    tx_thread_create(&switch_thread_ptr, "switch_thread", switch_thread_entry, 0, switch_thread_stack, SWITCH_THREAD_STACK_SIZE, SWITCH_THREAD_PRIORIY, SWITCH_THREAD_PREMPTION_PRIORIY, 1, TX_AUTO_START);
    tx_thread_create(&stp_thread_ptr, "stp_thread", stp_thread_entry, 0, stp_thread_stack, STP_THREAD_STACK_SIZE, STP_THREAD_PRIORIY, STP_THREAD_PREMPTION_PRIORIY, 1, TX_AUTO_START);
    tx_thread_create(&comms_thread_ptr, "comms_thread", comms_thread_entry, 0, comms_thread_stack, COMMS_THREAD_STACK_SIZE, COMMS_THREAD_PRIORIY, COMMS_THREAD_PREMPTION_PRIORIY, 1, TX_DONT_START);
}
