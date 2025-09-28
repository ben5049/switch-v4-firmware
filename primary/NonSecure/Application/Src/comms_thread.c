/*
 * comms_thread.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#include "stdio.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nx_stm32_eth_config.h"
#include "main.h"

#include "tx_app.h"
#include "nx_app.h"
#include "zenoh-pico.h"
#include "zenoh_cleanup.h"
#include "ucdr/microcdr.h"
#include "comms_thread.h"
#include "utils.h"
#include "config.h"
#include "switch_thread.h"


// #if Z_FEATURE_PUBLICATION == 1

#define MODE          "client"
#define LOCATOR       "" /* If empty, it will scout */

#define KEYEXPR       "demo/example/zenoh-pico-pub"
#define VALUE         "[Switch V4] Pub from Zenoh-Pico!"

#define BUFFER_LENGTH (256)


uint8_t   comms_thread_stack[COMMS_THREAD_STACK_SIZE];
TX_THREAD comms_thread_handle;

uint8_t      zenoh_byte_pool_buffer[ZENOH_MEM_POOL_SIZE] __ALIGNED(32);
TX_BYTE_POOL zenoh_byte_pool;

uint32_t restart_counter = 0;

void comms_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status    = TX_SUCCESS;
    _z_res_t    z_status     = Z_OK;
    bool        session_open = false;

    while (1) {

        /* Initialise the byte pool */
        tx_status = tx_byte_pool_create(&zenoh_byte_pool, "Zenoh Pico memory pool", zenoh_byte_pool_buffer, ZENOH_MEM_POOL_SIZE);
        if (tx_status != TX_SUCCESS) Error_Handler();

        /* Read and apply the config */
        z_owned_config_t config;
        z_status = z_config_default(&config);
        if (z_status < Z_OK) Error_Handler();
        z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
        if (z_status < Z_OK) Error_Handler();
        if (strcmp(LOCATOR, "") != 0) {
            if (strcmp(MODE, Z_CONFIG_MODE_CLIENT) == 0) {
                z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, LOCATOR);
                if (z_status < Z_OK) Error_Handler();
            } else {
                z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, LOCATOR);
                if (z_status < Z_OK) Error_Handler();
            }
        }

        /* Start a session */
        z_owned_session_t s;
        do {

            /* Attempt to open session */
            z_status = z_open(&s, z_move(config), NULL);

            /* Session open: continue */
            if (z_status == Z_OK) {
            }

            /* Unable to find other Zenoh devices: try again */
            else if ((z_status == _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN) || /* Can occur when server is not configured with the correct transport */
                     (z_status == _Z_ERR_SCOUT_NO_RESULTS)                 /* UDP multicasts received no response */
            ) {
                z_sleep_ms(200);
            }

            /* Unhandled error: call system error handler */
            else {
                Error_Handler();
            }

        } while (z_status < 0);

        // /* Create the read task */
        // static StackType_t  read_task_stack[1000];
        // static StaticTask_t read_task_buffer;
        // z_task_attr_t       read_task_attr = {
        //           .name              = "ZenohReadTask",
        //           .priority          = 10,
        //           .stack_depth       = 1000,
        //           .static_allocation = true,
        //           .stack_buffer      = read_task_stack,
        //           .task_buffer       = &read_task_buffer,
        // };

        // /* Create the read task */
        // zp_task_read_options_t read_task_opt = {.task_attributes = &read_task_attr};

        // static StackType_t lease_task_stack[1000];
        // static StaticTask_t lease_task_buffer;

        // z_task_attr_t lease_task_attr = {
        //     .name = "ZenohLeaseTask",
        //     .priority = 10,
        //     .stack_depth = 1000,
        //     .static_allocation = true,
        //     .stack_buffer = lease_task_stack,
        //     .task_buffer = &lease_task_buffer,
        // };

        // zp_task_lease_options_t lease_task_opt = {.task_attributes = &lease_task_attr};

        zp_task_read_options_t  read_task_opt  = {0};
        zp_task_lease_options_t lease_task_opt = {0};

        /* Start the read and lease tasks */
        if (zp_start_read_task(z_loan_mut(s), &read_task_opt) < Z_OK ||
            zp_start_lease_task(z_loan_mut(s), &lease_task_opt) < Z_OK) {
            z_session_drop(z_session_move(&s));
            Error_Handler();
        }

        // z_publisher_options_t publisher_opt = {
        //     .congestion_control = Z_CONGESTION_CONTROL_DROP,
        //     .encoding           = 0,
        //     .is_express         = false,
        //     .priority           = 0,
        // };

        /* Declare publisher */
        z_owned_publisher_t pub;
        z_view_keyexpr_t    ke;
        z_status = z_view_keyexpr_from_str(&ke, KEYEXPR);
        if (z_status < Z_OK) Error_Handler();
        z_status = z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL);
        if (z_status < Z_OK) Error_Handler();

        /* Initialise the buffers */
        uint8_t   *buffer = z_malloc(BUFFER_LENGTH);
        ucdrBuffer writer;
        ucdrBuffer reader;
        ucdr_init_buffer(&writer, buffer, BUFFER_LENGTH);
        ucdr_init_buffer(&reader, buffer, BUFFER_LENGTH);

        session_open = z_internal_session_check(&s);
        while (session_open) {

            // {
            //     bool port_states[SJA1105_NUM_PORTS];
            //     for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) {
            //         SJA1105_PortGetForwarding(&hsja1105, i, port_states + i);
            //     }
            //     ucdr_init_buffer(&writer, buffer, BUFFER_LENGTH);
            //     ucdr_serialize_array_bool(&writer, port_states, SJA1105_NUM_PORTS);
            // }

            {
                sja1105_statistics_t stats;
                if (SJA1105_ReadStatistics(&hsja1105, &stats) != SJA1105_OK) Error_Handler();
                uint64_t total_received_bytes = 0;
                for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) total_received_bytes += stats.rx_bytes[i];
                ucdr_init_buffer(&writer, buffer, BUFFER_LENGTH);
                ucdr_serialize_uint64_t(&writer, total_received_bytes);
            }

            /* Create payload */
            z_owned_bytes_t payload;
            z_status = z_bytes_copy_from_buf(&payload, buffer, writer.offset);
            if (z_status < Z_OK) goto restart;

            /* Publish the message */
            z_publisher_put_options_t options;
            z_publisher_put_options_default(&options);
            z_status = z_publisher_put(z_loan(pub), z_move(payload), &options);
            if (z_status < Z_OK) goto restart;

            // /* Check the free space in the byte pool */
            // tx_status = tx_byte_pool_info_get(&zenoh_byte_pool, NULL, &zenoh_bytes_available, NULL, NULL, NULL, NULL);
            // if (tx_status != TX_SUCCESS) goto restart;

            z_status = z_sleep_ms(1000);
            if (z_status < Z_OK) goto restart;

            session_open = z_internal_session_check(&s);
        }

    restart:

        restart_counter++;

        /* Clean-up. TODO: Decide if this is useful */
        // z_drop(z_move(pub));
        // z_drop(z_move(s));

        /* Delete all the threads, mutexes and semaphores created by Zenoh Pico */
        zenoh_cleanup_tx();

        /* Delete all the sockets created by Zenoh Pico */
        zenoh_cleanup_nx();

        /* Delete the byte pool */
        tx_status = tx_byte_pool_delete(&zenoh_byte_pool);
        if (tx_status != TX_SUCCESS) Error_Handler();

        // Error_Handler(); // TODO: Delete
    }
}
