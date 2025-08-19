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
#include "nxd_ptp_client.h"
#include "nx_stm32_eth_driver.h"
#include "main.h"

#include "nx_app.h"
#include "ptp_thread.h"
#include "ptp_callbacks.h"
#include "config.h"
#include "comms_thread.h"


#define NULL_ADDRESS 0


NX_IP    nx_ip_instance;
uint32_t ip_address;
uint32_t net_mask;

NX_PACKET_POOL nx_packet_pool;
static uint8_t nx_packet_pool_memory[NX_APP_PACKET_POOL_SIZE] __attribute__((section(".ETH_Section"))) ;

NX_DHCP      dhcp_client;
TX_SEMAPHORE dhcp_semaphore_handle;

NX_PTP_CLIENT  ptp_client;
static uint8_t nx_internal_ptp_stack[NX_INTERNAL_PTP_THREAD_STACK_SIZE];

TX_THREAD nx_app_thread_handle;
uint8_t   nx_app_thread_stack[NX_APP_THREAD_STACK_SIZE];


/* This function should be called once in MX_NetXDuo_Init */
nx_status_t nx_user_init(TX_BYTE_POOL *byte_pool) {

    nx_status_t status = NX_STATUS_SUCCESS;

    /* Initialize the NetXDuo system. */
    uint8_t *pointer;
    nx_system_initialize();

    /* Create the Packet pool to be used for packet allocation,
     * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
     */
    status = nx_packet_pool_create(&nx_packet_pool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, nx_packet_pool_memory, NX_APP_PACKET_POOL_SIZE);
    if (status != NX_SUCCESS) return status;

    /* Allocate the memory for nx_ip_instance */
    status = (tx_byte_allocate(byte_pool, (void **) &pointer, NX_INTERNAL_IP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS);
    if (status != NX_SUCCESS) return status;

    /* Create the main NX_IP instance */
    status = nx_ip_create(&nx_ip_instance, "NetX Ip instance", NX_DEFAULT_IP_ADDRESS, NX_DEFAULT_NET_MASK, &nx_packet_pool, nx_stm32_eth_driver, pointer, NX_INTERNAL_IP_THREAD_STACK_SIZE, NX_INTERNAL_IP_THREAD_PRIORITY);
    if (status != NX_SUCCESS) return status;

    /* Allocate the memory for ARP */
    status = (tx_byte_allocate(byte_pool, (void **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS);
    if (status != NX_SUCCESS) return status;

    /* Enable ARP and provide the ARP cache size for the IP instance */
    status = nx_arp_enable(&nx_ip_instance, (void *) pointer, DEFAULT_ARP_CACHE_SIZE);
    if (status != NX_SUCCESS) return status;

    /* Enable the ICMP */
    status = nx_icmp_enable(&nx_ip_instance);
    if (status != NX_SUCCESS) return status;

    /* Enable TCP */
    status = nx_tcp_enable(&nx_ip_instance);
    if (status != NX_SUCCESS) return status;

    /* Enable UDP required for DHCP communication */
    status = nx_udp_enable(&nx_ip_instance);
    if (status != NX_SUCCESS) return status;

    /* Create the DHCP client */
    status = nx_dhcp_create(&dhcp_client, &nx_ip_instance, "DHCP Client");
    if (status != NX_SUCCESS) return status;

    /* Create the PTP client */
    status = nx_ptp_client_create(&ptp_client, &nx_ip_instance, 0, &nx_packet_pool, NX_INTERNAL_PTP_THREAD_PRIORITY, (UCHAR *) nx_internal_ptp_stack, sizeof(nx_internal_ptp_stack), ptp_clock_callback, NX_NULL);
    if (status != NX_SUCCESS) return status;

    return status;
}


static void ip_address_change_notify_callback(NX_IP *ip_instance, void *ptr) {
    if (nx_ip_address_get(&nx_ip_instance, &ip_address, &net_mask) != NX_SUCCESS) {
        Error_Handler();
    }
    if (ip_address != NULL_ADDRESS) {
        tx_semaphore_put(&dhcp_semaphore_handle);
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
    if (tx_semaphore_get(&dhcp_semaphore_handle, TX_WAIT_FOREVER) != TX_SUCCESS) {
        Error_Handler();
    }

    /* TODO: Start any threads that require networking after this is done such as the comms thread (not stp though) */

    /* the network is correctly initialized, start certain threads */
    tx_thread_resume(&comms_thread_handle);
    tx_thread_resume(&ptp_thread_handle);

    /* This thread is no longer needed */
    tx_thread_relinquish();
    return;
}
