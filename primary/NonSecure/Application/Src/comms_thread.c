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
#include "state_machine.h"


#define BUFFER_LENGTH (256) /* Max message size in bytes */


typedef enum {
    HEARTBEAT_NOT_STARTED,
    HEARTBEAT_CONSUMING,
    HEARTBEAT_PRODUCING,
    HEARTBEAT_MISSED,
} heartbeat_state_t;


uint8_t   comms_thread_stack[COMMS_THREAD_STACK_SIZE];
TX_THREAD comms_thread_handle;

uint8_t      zenoh_byte_pool_buffer[ZENOH_MEM_POOL_SIZE] __ALIGNED(32);
TX_BYTE_POOL zenoh_byte_pool;

uint32_t          heartbeat_consumer_timestamp = 0;                     /* The last time a heartbeat was received */
heartbeat_state_t heartbeat_consumer_state     = HEARTBEAT_NOT_STARTED; /* The state of the heartbeat consumer */

/* Publishers */
z_owned_publisher_t stats_pub;
z_owned_publisher_t heartbeat_pub;

/* Publisher options */
static const z_publisher_options_t heartbeat_pub_options = {
    .encoding           = NULL,
    .congestion_control = Z_CONGESTION_CONTROL_BLOCK, /* This is an important message so don't drop it */
    .priority           = Z_PRIORITY_DATA_HIGH,       /* Slightly higher priority than regular data */
    .is_express         = true,                       /* Disable batching so the message is sent immediately */
    .reliability        = Z_RELIABILITY_RELIABLE,
};


/* Called when a message is received from the topic ZENOH_SUB_HEARTBEAT_KEYEXPR */
void heartbeat_sub_callback(z_loaned_sample_t* sample, void* ctx) {
    UNUSED(ctx);
    heartbeat_consumer_timestamp = tx_time_get_ms();
    if (heartbeat_consumer_state == HEARTBEAT_NOT_STARTED) {
        heartbeat_consumer_state = HEARTBEAT_CONSUMING;
    }
}


/* This thread manages the Zenoh Pico session initialisation, reconnection and heartbeats */
void comms_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status       = TX_SUCCESS;
    _z_res_t    z_status        = Z_OK;
    bool        session_open    = false;
    uint32_t    restart_counter = 0;
    uint32_t    retry_counter   = 0;

    while (1) {

        /* Reset variables */
        tx_status                = TX_SUCCESS;
        z_status                 = Z_OK;
        session_open             = false;
        heartbeat_consumer_state = HEARTBEAT_NOT_STARTED;

        /* Initialise the byte pool */
        tx_status = tx_byte_pool_create(&zenoh_byte_pool, "Zenoh Pico memory pool", zenoh_byte_pool_buffer, ZENOH_MEM_POOL_SIZE);
        if (tx_status != TX_SUCCESS) Error_Handler();

        /* Read and apply the config */
        z_owned_config_t config;
        z_status = z_config_default(&config);
        if (z_status < Z_OK) Error_Handler();
        z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_MODE);
        if (z_status < Z_OK) Error_Handler();
        if (strcmp(ZENOH_LOCATOR, "") != 0) {
            if (strcmp(ZENOH_MODE, Z_CONFIG_MODE_CLIENT) == 0) {
                z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_LOCATOR);
                if (z_status < Z_OK) Error_Handler();
            } else {
                z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, ZENOH_LOCATOR);
                if (z_status < Z_OK) Error_Handler();
            }
        }

        /* Start a session */
        static z_owned_session_t session;
        do {

            /* Attempt to open session */
            z_status = z_open(&session, z_move(config), NULL);

            /* Session open: proceed */
            if (z_status == Z_OK) {
                break;
            }

            /* Unable to find other Zenoh devices: try again */
            else if (z_status == _Z_ERR_SCOUT_NO_RESULTS) {
                z_sleep_ms(ZENOH_OPEN_SESSION_INTERVAL);
                goto retry;
            }

            /* Error handling */
            else if (z_status == _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN) { /* Can occur when the router is not configured with the correct transport */
                z_sleep_ms(ZENOH_OPEN_SESSION_INTERVAL);
                goto retry;
            }

            /* Unhandled error: call system error handler */
            else {
#ifdef DEBUG
                Error_Handler();
#endif
                z_sleep_ms(ZENOH_OPEN_SESSION_INTERVAL);
                goto retry;
            }

        } while (z_status < Z_OK);

        /* Start the read, lease tasks and periodic scheduler threads */
        z_status = zp_start_read_task(z_loan_mut(session), NULL);
        if (z_status < Z_OK) Error_Handler();
        z_status = zp_start_lease_task(z_loan_mut(session), NULL);
        if (z_status < Z_OK) Error_Handler();
        //        z_status = zp_start_periodic_scheduler_task(z_loan_mut(session), NULL);
        //        if (z_status < Z_OK) Error_Handler();

        /* Declare publisher */
        z_view_keyexpr_t stats_pub_key;
        z_status = z_view_keyexpr_from_str(&stats_pub_key, ZENOH_PUB_STATS_KEYEXPR);
        if (z_status < Z_OK) Error_Handler();
        z_status = z_declare_publisher(z_loan(session), &stats_pub, z_loan(stats_pub_key), NULL);
        if (z_status < Z_OK) Error_Handler();

        /* Declare heartbeat publisher */
        z_view_keyexpr_t heartbeat_pub_key;
        z_status = z_view_keyexpr_from_str(&heartbeat_pub_key, ZENOH_PUB_HEARTBEAT_KEYEXPR);
        if (z_status < Z_OK) Error_Handler();
        z_status = z_declare_publisher(z_loan(session), &heartbeat_pub, z_loan(heartbeat_pub_key), &heartbeat_pub_options);
        if (z_status < Z_OK) Error_Handler();

        /* Declare heartbeat background subscriber */
        z_view_keyexpr_t heartbeat_sub_key;
        z_status = z_view_keyexpr_from_str(&heartbeat_sub_key, ZENOH_SUB_HEARTBEAT_KEYEXPR);
        if (z_status < Z_OK) Error_Handler();
        z_owned_closure_sample_t heartbeat_closure;
        z_closure(&heartbeat_closure, heartbeat_sub_callback, NULL, NULL);
        z_status = z_declare_background_subscriber(z_loan(session), z_loan(heartbeat_sub_key), z_move(heartbeat_closure), NULL);
        if (z_status < Z_OK) Error_Handler();

        /* Notify the state machine */
        tx_status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_ZENOH_CONNECTED | STATE_MACHINE_UPDATE, TX_OR);
        if (tx_status != TX_SUCCESS) Error_Handler();

        /* Initialise the Micro CDR buffers */
        uint8_t*   buffer = z_malloc(BUFFER_LENGTH);
        ucdrBuffer writer;
        ucdrBuffer reader;
        ucdr_init_buffer(&writer, buffer, BUFFER_LENGTH);
        ucdr_init_buffer(&reader, buffer, BUFFER_LENGTH);

        session_open = !z_session_is_closed(z_loan(session));
        while (session_open) {

            /* Check if the server has died */
            if ((heartbeat_consumer_state == HEARTBEAT_CONSUMING) &&
                (tx_time_get_ms() > (heartbeat_consumer_timestamp + HEARTBEAT_MISS_TIMEOUT))) {
                heartbeat_consumer_state = HEARTBEAT_MISSED;
                goto restart;
                // TODO: Log in a specific zenoh event counter?
            }

            /* Create the heartbeat payload. TODO: Change heartbeat message to include uptime and other info */
            ucdr_init_buffer(&writer, buffer, BUFFER_LENGTH);
            ucdr_serialize_bool(&writer, true);
            z_owned_bytes_t payload;
            z_status = z_bytes_copy_from_buf(&payload, buffer, writer.offset);
            if (z_status < Z_OK) goto restart;

            /* Publish heartbeat message */
            z_publisher_put_options_t options;
            z_publisher_put_options_default(&options);
            z_status = z_publisher_put(z_loan(heartbeat_pub), z_move(payload), &options);
            if (z_status < Z_OK) goto restart;

            /* Sleep for HEARTBEAT_INTERVAL ms */
            tx_status = tx_thread_sleep_ms(HEARTBEAT_INTERVAL);
            if (tx_status != TX_SUCCESS) Error_Handler();
            session_open = !z_session_is_closed(z_loan(session));
        }

    /* An error occured while running */
    restart:

        /* Notify the state machine */
        tx_status = tx_event_flags_set(&state_machine_events_handle, ~STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED, TX_AND);
        if (tx_status != TX_SUCCESS) Error_Handler();
        tx_status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_UPDATE, TX_OR);
        if (tx_status != TX_SUCCESS) Error_Handler();

        restart_counter++;

        /* Clean-up. TODO: Decide if this is useful */
        // z_drop(z_move(pub));
        // z_drop(z_move(session));

    /* An error occured while connecting */
    retry:

        retry_counter++;

        /* Clear the session to make sure other threads can't send messages */
        z_internal_session_null(&session);

        /* Delete all the threads, mutexes and semaphores created by Zenoh Pico */
        zenoh_cleanup_tx();

        /* Delete all the sockets created by Zenoh Pico */
        zenoh_cleanup_nx();

        /* Delete the byte pool */
        tx_status = tx_byte_pool_delete(&zenoh_byte_pool);
        if (tx_status != TX_SUCCESS) Error_Handler();
    }
}
