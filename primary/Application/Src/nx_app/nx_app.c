/*
 * nx_app.c
 *
 *  Created on: Aug 10, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdbool.h"
#include "nx_api.h"
#include "nxd_dhcp_client.h"
#include "nx_stm32_eth_driver.h"
#include "main.h"

#include "nx_app.h"
#include "config.h"
#include "comms_thread.h"


#define NULL_ADDRESS 0


NX_IP    nx_ip_instance;
uint32_t ip_address;
uint32_t net_mask;

NX_PACKET_POOL nx_packet_pool;

NX_DHCP      dhcp_client;
TX_SEMAPHORE dhcp_semaphore_ptr;

TX_THREAD nx_app_thread_ptr;
uint8_t   nx_app_thread_stack[NX_APP_THREAD_STACK_SIZE];

TX_THREAD nx_link_thread_ptr;
uint8_t   nx_link_thread_stack[NX_LINK_THREAD_STACK_SIZE];

/* This function should be called once in MX_NetXDuo_Init */
nx_status_t nx_user_init(TX_BYTE_POOL *byte_pool) {

    nx_status_t status = NX_STATUS_SUCCESS;

    /* Initialize the NetXDuo system. */
    uint8_t *pointer;
    nx_system_initialize();

    /* Allocate the memory for packet_pool.  */
    if (tx_byte_allocate(byte_pool, (void **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS) {
        return TX_POOL_ERROR;
    }

    /* Create the Packet pool to be used for packet allocation,
     * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
     */
    status = nx_packet_pool_create(&nx_packet_pool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE);

    if (status != NX_SUCCESS) {
        return NX_POOL_ERROR;
    }

    /* Allocate the memory for nx_ip_instance */
    if (tx_byte_allocate(byte_pool, (void **) &pointer, NX_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT) != TX_SUCCESS) {
        return TX_POOL_ERROR;
    }

    /* Create the main NX_IP instance */
    status = nx_ip_create(&nx_ip_instance, "NetX Ip instance", NX_APP_DEFAULT_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK, &nx_packet_pool, nx_stm32_eth_driver, pointer, NX_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY);

    if (status != NX_SUCCESS) {
        return NX_NOT_SUCCESSFUL;
    }

    /* Allocate the memory for ARP */
    if (tx_byte_allocate(byte_pool, (void **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS) {
        return TX_POOL_ERROR;
    }

    /* Enable ARP and provide the ARP cache size for the IP instance */
    status = nx_arp_enable(&nx_ip_instance, (void *) pointer, DEFAULT_ARP_CACHE_SIZE);

    if (status != NX_SUCCESS) {
        return NX_NOT_SUCCESSFUL;
    }

    /* Enable the ICMP */
    status = nx_icmp_enable(&nx_ip_instance);

    if (status != NX_SUCCESS) {
        return NX_NOT_SUCCESSFUL;
    }

    /* Enable TCP */
    status = nx_tcp_enable(&nx_ip_instance);

    if (status != NX_SUCCESS) {
        return NX_NOT_SUCCESSFUL;
    }

    /* Enable UDP required for DHCP communication */
    status = nx_udp_enable(&nx_ip_instance);

    if (status != NX_SUCCESS) {
        return NX_NOT_SUCCESSFUL;
    }

    /* Create the DHCP client */
    status = nx_dhcp_create(&dhcp_client, &nx_ip_instance, "DHCP Client");

    if (status != NX_SUCCESS) {
        return NX_DHCP_ERROR;
    }

    /* TODO: Enable STP */

    return status;
}


static void ip_address_change_notify_callback(NX_IP *ip_instance, void *ptr) {
    if (nx_ip_address_get(&nx_ip_instance, &ip_address, &net_mask) != NX_SUCCESS) {
        Error_Handler();
    }
    if (ip_address != NULL_ADDRESS) {
        tx_semaphore_put(&dhcp_semaphore_ptr);
    }
}


void nx_app_thread_entry(uint32_t initial_input) {

    nx_status_t status = NX_SUCCESS;

    /* Register the IP address change callback */
    status = nx_ip_address_change_notify(&nx_ip_instance, ip_address_change_notify_callback, NULL);
    if (status != NX_SUCCESS) {
        Error_Handler();
    }

    /* Start the DHCP client */
    status = nx_dhcp_start(&dhcp_client);
    if (status != NX_SUCCESS) {
        Error_Handler();
    }

    /* Wait until an IP address is ready */
    if (tx_semaphore_get(&dhcp_semaphore_ptr, TX_WAIT_FOREVER) != TX_SUCCESS) {
        Error_Handler();
    }

    /* TODO: Start any threads that require networking after this is done such as the comms thread (not stp though) */

    /* the network is correctly initialized, start the TCP thread */
    tx_thread_resume(&comms_thread_ptr);

    /* this thread is not needed any more, relinquish it */
    tx_thread_relinquish();

    return;
}


/* TODO: Remember the dynamically assigned IP address (NX_DHCP_CLIENT_RESTORE_STATE) */

void nx_link_thread_entry(uint32_t thread_input) {

    nx_ip_status_t actual_status;
    nx_status_t    status;
    bool           linkdown = false;

    while (1) {

        /* Send request to check if the Ethernet cable is connected. */
        status = nx_ip_interface_status_check(&nx_ip_instance, 0, NX_IP_LINK_ENABLED, (uint32_t *) &actual_status, 10);

        if (status == NX_SUCCESS) {
            if (linkdown) {
                linkdown = false;

                /* Send request to enable PHY Link. */
                nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_ENABLE, (uint32_t *) &actual_status);

                /* Send request to check if an address is resolved. */
                status = nx_ip_interface_status_check(&nx_ip_instance, 0, NX_IP_ADDRESS_RESOLVED, (uint32_t *) &actual_status, 10);
                if (status == NX_SUCCESS) {

                    /* Stop DHCP */
                    nx_dhcp_stop(&dhcp_client);

                    /* Re-initialize DHCP */
                    nx_dhcp_reinitialize(&dhcp_client);

                    /* Start DHCP */
                    nx_dhcp_start(&dhcp_client);

                    /* Wait until an IP address is ready */
                    if (tx_semaphore_get(&dhcp_semaphore_ptr, TX_WAIT_FOREVER) != TX_SUCCESS) {
                        Error_Handler();
                    }

                } else {

                    /* Set the DHCP Client's remaining lease time to 0 seconds to trigger an immediate renewal request for a DHCP address. */
                    nx_dhcp_client_update_time_remaining(&dhcp_client, 0);
                }
            }
        } else if (!linkdown) {
            linkdown = true;

            /* The network cable is not connected. */
            nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_DISABLE, (uint32_t *) &actual_status);
        }

        tx_thread_sleep(NX_APP_CABLE_CONNECTION_CHECK_PERIOD);
    }
}
