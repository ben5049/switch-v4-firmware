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
#include "utils.h"
#include "config.h"
#include "zenoh-pico.h"
#include "zenoh_cleanup.h"
#include "ucdr/microcdr.h"
#include "comms_thread.h"
#include "switch_thread.h"


#define MODE    "client"
#define LOCATOR "" /* If empty, it will scout */
// #define LOCATOR       "udp/192.168.50.2:6000"

#define PUB_STATS_KEYEXPR     DEVICE_NAME "/stats"
#define PUB_HEARTBEAT_KEYEXPR DEVICE_NAME "/heartbeat" /* The topic to publish */

#define SUB_HEARTBEAT_KEYEXPR "server/heartbeat"

#define BUFFER_LENGTH         (256) /* Max message size in bytes */


uint8_t   comms_thread_stack[COMMS_THREAD_STACK_SIZE];
TX_THREAD comms_thread_handle;

uint8_t      zenoh_byte_pool_buffer[ZENOH_MEM_POOL_SIZE] __ALIGNED(32);
TX_BYTE_POOL zenoh_byte_pool;

uint32_t restart_counter = 0;


void liveliness_handler(z_loaned_sample_t* sample, void* ctx) {
    UNUSED(ctx);
    z_sample_kind_t sample_kind = z_sample_kind(sample);
    switch (sample_kind) {

        /* New liveliness token */
        case Z_SAMPLE_KIND_PUT:
            break;

        /* Liveliness token deleted */
        case Z_SAMPLE_KIND_DELETE:
            /* TODO: Signal to comms_thread that it needs to restart */
            break;

        default:
            Error_Handler();
    }
}


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

            /* Session open: proceed */
            if (z_status == Z_OK) {
                break;
            }

            /* Unable to find other Zenoh devices: try again */
            else if (z_status == _Z_ERR_SCOUT_NO_RESULTS) {
                z_sleep_ms(200);
                goto restart;
                continue;
            }

            /* Error handling */
            else if (z_status == _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN) { /* Can occur when the router is not configured with the correct transport */
                z_sleep_ms(200);
                goto restart;
            }

            /* Unhandled error: call system error handler */
            else {
#ifdef DEBUG
                Error_Handler();
#endif
                z_sleep_ms(200);
                goto restart;
            }

        } while (z_status < Z_OK);

        /* Start the read, lease tasks and periodic scheduler threads */
        z_status = zp_start_read_task(z_loan_mut(s), NULL);
        if (z_status < Z_OK) Error_Handler();
        z_status = zp_start_lease_task(z_loan_mut(s), NULL);
        if (z_status < Z_OK) Error_Handler();
        //        z_status = zp_start_periodic_scheduler_task(z_loan_mut(s), NULL);
        //        if (z_status < Z_OK) Error_Handler();

        /* Declare publisher */
        z_owned_publisher_t pub;
        z_view_keyexpr_t    pke;
        z_status = z_view_keyexpr_from_str(&pke, PUB_STATS_KEYEXPR);
        if (z_status < Z_OK) Error_Handler();
        z_status = z_declare_publisher(z_loan(s), &pub, z_loan(pke), NULL);
        if (z_status < Z_OK) Error_Handler();

        /* Declare advanced subscriber */
        //        ze_advanced_subscriber_options_t sub_opts;
        //        ze_advanced_subscriber_options_default(&sub_opts);
        //        ze_advanced_subscriber_history_options_default(&sub_opts.history);
        //        sub_opts.history.detect_late_publishers = true;
        //        ze_advanced_subscriber_recovery_options_default(&sub_opts.recovery);
        //        ze_advanced_subscriber_last_sample_miss_detection_options_default(&sub_opts.recovery.last_sample_miss_detection);
        //        sub_opts.recovery.last_sample_miss_detection.periodic_queries_period_ms = 0; /* Use publisher heartbeats by default, otherwise enable periodic queries by setting to > 0 */
        //        sub_opts.subscriber_detection                                           = true;
        // z_owned_closure_sample_t callback;
        // z_closure(&callback, data_handler, NULL, NULL);
        // ze_owned_advanced_subscriber_t sub;

        // z_status = ze_declare_advanced_subscriber(z_loan(s), &sub, z_loan(ke), z_move(callback), &sub_opts);
        // if (z_status < Z_OK) Error_Handler();
        // ze_owned_closure_miss_t         miss_callback;
        // ze_owned_sample_miss_listener_t miss_listener;
        // z_closure(&miss_callback, miss_handler, NULL, NULL);
        // ze_advanced_subscriber_declare_sample_miss_listener(z_loan(sub), &miss_listener, z_move(miss_callback));
        //        z_owned_closure_sample_t liveliness_callback;
        //        z_closure(&liveliness_callback, liveliness_handler, NULL, NULL);

        // z_owned_subscriber_t liveliness_sub;
        // z_status = ze_advanced_subscriber_detect_publishers(z_loan(sub), &liveliness_sub, z_move(liveliness_callback), &liveliness_sub_opt);
        // if (z_status < Z_OK) Error_Handler();

        // z_view_keyexpr_t ske;
        // z_status = z_view_keyexpr_from_str(&ske, SUB_HEARTBEAT_KEYEXPR);
        // if (z_status < Z_OK) Error_Handler();
        // z_owned_closure_sample_t liveliness_callback;
        // z_closure(&liveliness_callback, liveliness_handler, NULL, NULL);
        // z_status = z_liveliness_declare_background_subscriber(z_loan(s), z_loan(ske), z_move(liveliness_callback), NULL);
        // if (z_status < Z_OK) Error_Handler();

        /* Initialise the Micro CDR buffers */
        uint8_t*   buffer = z_malloc(BUFFER_LENGTH);
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
