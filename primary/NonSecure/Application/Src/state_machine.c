/*
 * state_machine.c
 *
 *  Created on: Sep 3, 2025
 *      Author: bens1
 */

#include "main.h"
#include "eth.h"

#include "state_machine.h"
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
#include "utils.h"


TX_THREAD            state_machine_thread_handle;
uint8_t              state_machine_thread_stack[STATE_MACHINE_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP state_machine_events_handle;


void state_machine_thread_entry(uint32_t initial_input) {

    uint32_t    event_flags;
    tx_status_t status = TX_SUCCESS;

    /* Startup sequence */

    /* Start the PHYs, STP thread, NetX networking threads */
    status = tx_thread_resume(&phy_thread_handle);
    if (status != TX_SUCCESS) Error_Handler();
    status = tx_thread_resume(&nx_link_thread_handle);
    if (status != TX_SUCCESS) Error_Handler();
    status = tx_thread_resume(&switch_thread_handle);
    if (status != TX_SUCCESS) Error_Handler();

    /* -------------------- Link Up -------------------- */

    /* Wait for the link to be up (from nx_link_thread_entry) */
    status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_NX_LINK_UP_EVENT, TX_OR, &event_flags, TX_WAIT_FOREVER);
    if (status != TX_SUCCESS) Error_Handler();

#if ENABLE_STP_THREAD == true
    status = tx_thread_resume(&stp_thread_handle);
    if (status != TX_SUCCESS) Error_Handler();
#endif

    /* -------------------- Network Up -------------------- */

    /* Wait for the network to be initialised and an IP address assigned */
    status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED_EVENT, TX_OR, &event_flags, TX_WAIT_FOREVER);
    if (status != TX_SUCCESS) Error_Handler();

    /* Start the threads that require networking */
    status = tx_thread_resume(&comms_thread_handle);
    if (status != TX_SUCCESS) Error_Handler();
    // status = tx_thread_resume(&ptp_thread_handle);
    // if (status != TX_SUCCESS) Error_Handler();

    while (1) {
        tx_thread_sleep_ms(1000);
    }
}
