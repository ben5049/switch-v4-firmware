/*
 * comms_thread.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#include "tx_api.h"
#include "nx_api.h"
#include "nx_stm32_eth_config.h"
#include "main.h"

#include "tx_app.h"
#include "nx_app.h"
#include "zenoh-pico.h"
#include "comms_thread.h"
#include "utils.h"
#include "config.h"
#include "switch_thread.h"


// #if Z_FEATURE_PUBLICATION == 1

#define MODE    "client"
#define LOCATOR "" /* If empty, it will scout */
//  #define MODE    "peer"
// #define LOCATOR "udp/192.168.50.1:6000"

#define KEYEXPR "demo/example/zenoh-pico-pub"
#define VALUE   "[Switch V4] Pub from Zenoh-Pico!"

// #define DEFAULT_PORT       6000
// #define TCP_SERVER_PORT    DEFAULT_PORT
// #define TCP_SERVER_ADDRESS IP_ADDRESS(192, 168, 1, 1)

// #define MAX_PACKET_COUNT   100
// #define DEFAULT_MESSAGE    "TCP Client on STM32H573-DK"


uint8_t   comms_thread_stack[COMMS_THREAD_STACK_SIZE];
TX_THREAD comms_thread_handle;

static uint8_t zenoh_byte_pool_buffer[ZENOH_MEM_POOL_SIZE] __ALIGNED(32);
TX_BYTE_POOL   zenoh_byte_pool;


void comms_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status = TX_SUCCESS;
    _z_res_t    z_status  = Z_OK;

    /* Initialise the byte pool */
    tx_status = tx_byte_pool_create(&zenoh_byte_pool, "Zenoh Pico memory pool", zenoh_byte_pool_buffer, ZENOH_MEM_POOL_SIZE);
    if (tx_status != TX_SUCCESS) Error_Handler();

    /* Apply the config */
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(LOCATOR, "") != 0) {
        if (strcmp(MODE, "client") == 0) {
            zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, LOCATOR);
        } else {
            zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, LOCATOR);
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
        else if ((z_status == _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN) ||
                 (z_status == _Z_ERR_SCOUT_NO_RESULTS)) {
            z_sleep_ms(500);
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

    /* Start read and lease tasks for */
    if (zp_start_read_task(z_loan_mut(s), &read_task_opt) < 0 ||
        zp_start_lease_task(z_loan_mut(s), &lease_task_opt) < 0) {
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
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0) {
        Error_Handler();
    }

    char *buf = (char *) z_malloc(256);
    while (1) {

        for (int idx = 0; 1; ++idx) {
            z_sleep_s(1);
            // snprintf(buf, 256, "[%4d] %s", idx, VALUE);

            bool port_states[SJA1105_NUM_PORTS];
            for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) {
                SJA1105_PortGetForwarding(&hsja1105, i, port_states + i);
            }

            snprintf(buf, 256, "Port states = [%d,%d,%d,%d,%d]", port_states[0], port_states[1], port_states[2], port_states[3], port_states[4]);

            // Create payload
            z_owned_bytes_t payload;
            z_bytes_copy_from_str(&payload, buf);

            z_publisher_put_options_t options;
            z_publisher_put_options_default(&options);
            z_publisher_put(z_loan(pub), z_move(payload), &options);
        }
    }

    // Clean-up
    z_drop(z_move(pub));
    z_drop(z_move(s));
}
