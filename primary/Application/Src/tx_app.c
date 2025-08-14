/*
 * tx_app.c
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 */

#include "tx_app.h"
#include "nx_app.h"
#include "nx_link_thread.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "phy_thread.h"
#include "stp_thread.h"
#include "comms_thread.h"
#include "ptp_thread.h"
#include "config.h"


/* This function should be called once in App_ThreadX_Init */
void tx_user_init(void *memory_ptr) {

    /* Create mutexes */
    tx_mutex_create(&sja1105_mutex_handle, "sja1105_mutex", TX_INHERIT);

    /* Create semaphores */
    tx_semaphore_create(&dhcp_semaphore_handle, "dhcp_semaphore", 0);

    /* Create event flags */
    tx_event_flags_create(&stp_events_handle, "stp_events_handle");

    /* Create queues */
    tx_queue_create(&ptp_tx_queue_handle, "ptp_tx_queue", sizeof(nx_ptp_tx_info_t), ptp_tx_queue_stack, PTP_TX_QUEUE_SIZE);

    /* Create threads */ // clang-format off
    tx_thread_create(&nx_app_thread_handle,  "nx_app_thread",  nx_app_thread_entry,  0, nx_app_thread_stack,  NX_APP_THREAD_STACK_SIZE,  NX_APP_THREAD_PRIORITY,  NX_APP_THREAD_PRIORITY,           TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&nx_link_thread_handle, "nx_link_thread", nx_link_thread_entry, 0, nx_link_thread_stack, NX_LINK_THREAD_STACK_SIZE, NX_LINK_THREAD_PRIORITY, NX_LINK_THREAD_PRIORITY,          TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&switch_thread_handle,  "switch_thread",  switch_thread_entry,  0, switch_thread_stack,  SWITCH_THREAD_STACK_SIZE,  SWITCH_THREAD_PRIORITY,  SWITCH_THREAD_PREMPTION_PRIORITY, 1,                TX_AUTO_START);
    tx_thread_create(&phy_thread_handle,     "phy_thread",     phy_thread_entry,     0, phy_thread_stack,     PHY_THREAD_STACK_SIZE,     PHY_THREAD_PRIORITY,     PHY_THREAD_PREMPTION_PRIORITY,    1,                TX_AUTO_START);
    tx_thread_create(&stp_thread_handle,     "stp_thread",     stp_thread_entry,     0, stp_thread_stack,     STP_THREAD_STACK_SIZE,     STP_THREAD_PRIORITY,     STP_THREAD_PREMPTION_PRIORITY,    1,                TX_AUTO_START);
    tx_thread_create(&comms_thread_handle,   "comms_thread",   comms_thread_entry,   0, comms_thread_stack,   COMMS_THREAD_STACK_SIZE,   COMMS_THREAD_PRIORITY,   COMMS_THREAD_PREMPTION_PRIORITY,  1,                TX_DONT_START);
    tx_thread_create(&ptp_thread_handle,     "ptp_thread",     ptp_thread_entry,     0, ptp_thread_stack,     PTP_THREAD_STACK_SIZE,     PTP_THREAD_PRIORITY,     PTP_THREAD_PRIORITY,              TX_NO_TIME_SLICE, TX_DONT_START); // clang-format on
}
