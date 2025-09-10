/*
 * nx_link_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 *
 */

#include "stdint.h"
#include "stdbool.h"
#include "nx_api.h"
#include "main.h"

#include "nx_app.h"
#include "config.h"
#include "utils.h"
#include "tx_app.h"
#include "state_machine.h"


TX_THREAD nx_link_thread_handle;
uint8_t   nx_link_thread_stack[NX_LINK_THREAD_STACK_SIZE];


/* TODO: Remember the dynamically assigned IP address (NX_DHCP_CLIENT_RESTORE_STATE) */
/* TODO: rethink this thread (+statuses) */

/* This thread monitors the link state and  */
void nx_link_thread_entry(uint32_t thread_input) {

    uint32_t    actual_status;
    nx_status_t nx_status;
    tx_status_t tx_status;
    bool        linkdown = true;

    while (1) {

        /* Send request to check if the Ethernet cable is connected */
        nx_status = nx_ip_interface_status_check(&nx_ip_instance, 0, NX_IP_LINK_ENABLED, &actual_status, 10);

        if (nx_status == NX_SUCCESS) {

            /* The link just went up */
            if (linkdown) {
                linkdown = false;

                /* Send request to enable PHY link */
                nx_status = nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_ENABLE, &actual_status);
                if ((nx_status != NX_SUCCESS) && (nx_status != NX_ALREADY_ENABLED)) Error_Handler();

                /* Notify the state machine that the link is up */
                tx_status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_NX_LINK_UP_EVENT, TX_OR);
                if (tx_status != TX_SUCCESS) Error_Handler();

                /* Send request to check if an address is resolved */
                nx_status = nx_ip_interface_status_check(&nx_ip_instance, 0, NX_IP_ADDRESS_RESOLVED, &actual_status, 10);
                if (nx_status == NX_SUCCESS) {

                    /* Stop DHCP */
                    nx_status = nx_dhcp_stop(&dhcp_client);
                    if (nx_status != NX_SUCCESS) Error_Handler();

                    /* Re-initialize DHCP */
                    nx_status = nx_dhcp_reinitialize(&dhcp_client);
                    if (nx_status != NX_SUCCESS) Error_Handler();

                    /* Start DHCP */
                    nx_status = nx_dhcp_start(&dhcp_client);
                    if (nx_status != NX_SUCCESS) Error_Handler();

                    /* Wait until an IP address is ready */
                    tx_status = tx_semaphore_get(&dhcp_semaphore_handle, TX_WAIT_FOREVER);
                    if (tx_status != TX_SUCCESS) Error_Handler();
                }
            }

            /* The link just went down */
            else {

                /* Set the DHCP Client's remaining lease time to 0 seconds to trigger an immediate renewal request for a DHCP address. */
                nx_status = nx_dhcp_client_update_time_remaining(&dhcp_client, 0);
                if (nx_status != NX_SUCCESS) Error_Handler();
            }
        }

        /* The cable was just unplugged */
        else if (!linkdown) {
            linkdown = true;

            /* The network cable is not connected. */
            nx_status = nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_DISABLE, &actual_status);
            if (nx_status != NX_SUCCESS) Error_Handler();
        }

        // TODO: what if nx_status = driver error

        tx_thread_sleep_ms(NX_APP_CABLE_CONNECTION_CHECK_PERIOD);
    }
}
