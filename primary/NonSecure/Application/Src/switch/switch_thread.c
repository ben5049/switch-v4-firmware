/*
 * switch_thread.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdatomic.h"
#include "main.h"
#include "zenoh-pico.h"

#include "pb_encode.h"
#include "switch.pb.h"

#include "tx_app.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "sja1105.h"
#include "sja1105q_default_conf.h"
#include "utils.h"
#include "encodings.h"
#include "state_machine.h"
#include "comms_thread.h"
#include "phy_thread.h"


#define CHECK(func)                                            \
    do {                                                       \
        sja_status = (func);                                   \
        sja1105_check_status_msg(&hsja1105, sja_status, true); \
    } while (0)

#define SWITCH_STATS_BUFFER_SIZE (128)


uint8_t   switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_handle;

atomic_uint_fast32_t        sja1105_error_counter = 0;
static sja1105_statistics_t switch_stats;


bool switch_stats_port_callback(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {

    port_index_t *port_index = (port_index_t *) (*arg);

    /* All ports have been written */
    if (*port_index >= SJA1105_NUM_PORTS) {
        return true;
    }

    /* Create the struct to store the port stats */
    PortDiag port       = PortDiag_init_zero;
    bool     forwarding = false;

    /* First time the interator is run load the high level statistics counters from the switch chip */
    if (*port_index == 0) {
        if (SJA1105_ReadStatistics(&hsja1105, &switch_stats) != SJA1105_OK) Error_Handler();
    }

    /* Assign the port high level statistics */
    port.rx_bytes           = switch_stats.rx_bytes[*port_index];
    port.has_rx_bytes       = true;
    port.tx_bytes           = switch_stats.tx_bytes[*port_index];
    port.has_tx_bytes       = true;
    port.dropped_frames     = 0; /* TODO: Get actual number */
    port.has_dropped_frames = true;

    /* Get and assign the port state */
    if (SJA1105_PortGetForwarding(&hsja1105, *port_index, &forwarding) != SJA1105_OK) Error_Handler();
    port.state = forwarding ? PortState_FORWARDING : PortState_DISABLED;

    /* Assign the PHY temperature */
    if (*port_index == PORT_HOST) {
        port.phy_temp     = 0.0; /* TODO: Get this value from the DTS */
        port.has_phy_temp = false;
    } else {
        port.phy_temp     = phy_temperatures[*port_index];
        port.has_phy_temp = true;
    }

    /* Encode this port message */
    if (!pb_encode_submessage(stream, PortDiag_fields, &port)) Error_Handler();

    /* Move to the next port */
    (*port_index)++;
    return true;
}


/* This thread perform regular maintenance for the switch and publishes periodic diagnostic messages */
void switch_thread_entry(uint32_t initial_input) {

    sja1105_status_t sja_status;
    tx_status_t      tx_status;
    _z_res_t         z_status;

    uint32_t current_time          = tx_time_get_ms();
    uint32_t next_publish_time     = current_time;
    uint32_t next_maintenance_time = current_time;
    int32_t  next_wakeup           = 0;

    float temperature;

    uint32_t flags;

    static uint8_t switch_stats_buffer[SWITCH_STATS_BUFFER_SIZE];
    port_index_t   switch_stats_port_index = 0;

    static SwitchDiag switch_diag  = SwitchDiag_init_zero;
    switch_diag.ports.funcs.encode = &switch_stats_port_callback;
    switch_diag.ports.arg          = &switch_stats_port_index;

    pb_ostream_t stream;

    z_owned_encoding_t stats_encoding;

    while (1) {

        current_time = tx_time_get_ms();

        /* Check if maintenance is necessary */
        if (current_time >= next_maintenance_time) {
            next_maintenance_time += SWITCH_MAINTENANCE_INTERVAL;

            /* Make sure local copies of tables match the copy on the switch chip (this doesn't check for differences, it only updates the internal copy) */
            sja_status = SJA1105_ReadAllTables(&hsja1105);
            if (sja_status != SJA1105_OK) Error_Handler();

            /* Check the sja_status registers for issues */
            sja_status = SJA1105_CheckStatusRegisters(&hsja1105); // TODO: look into buffer shifting issue
            if (sja_status != SJA1105_OK) Error_Handler();

            /* Free any management routes that have been used */
            sja_status = SJA1105_ManagementRouteFree(&hsja1105, false);
            if (sja_status != SJA1105_OK) Error_Handler();

            /* TODO: Occasionally check no important MAC addresses have been learned by accident (PTP, STP, etc) */

            /* Read the temperature */
            sja_status = SJA1105_ReadTemperature(&hsja1105, &temperature);
            if (sja_status != SJA1105_OK) Error_Handler();
        }

        /* Check if publishing stats is necessary */
        if (current_time >= next_publish_time) {
            next_publish_time += SWITCH_PUBLISH_STATS_INTERVAL;

            /* Check if publishing stats is allowed */
            tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_ZENOH_CONNECTED, TX_OR, &flags, TX_NO_WAIT);
            if (tx_status == TX_SUCCESS) {

                /* Reset the buffer */
                switch_stats_port_index = 0;
                stream                  = pb_ostream_from_buffer(switch_stats_buffer, SWITCH_STATS_BUFFER_SIZE);

                /* Get the stats and encode the message */
                switch_diag.has_timestamp         = true;
                switch_diag.timestamp.seconds     = current_time / 1000;
                switch_diag.timestamp.nanoseconds = (current_time % 1000) * 1000000;
                switch_diag.has_temp              = true;
                switch_diag.temp                  = temperature;
                if (!pb_encode(&stream, SwitchDiag_fields, &switch_diag)) Error_Handler();
                z_owned_bytes_t payload;
                z_status = z_bytes_from_static_buf(&payload, switch_stats_buffer, stream.bytes_written);
                if (z_status < Z_OK) ZENOH_DISCONNECTED(false);

                /* Check if publishing stats is still allowed */
                tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_ZENOH_CONNECTED, TX_OR, &flags, TX_NO_WAIT);
                if (tx_status == TX_SUCCESS) {

                    /* Publish the message */
                    z_publisher_put_options_t options;
                    z_publisher_put_options_default(&options);
                    z_status = z_encoding_from_str(&stats_encoding, ENCODING_SWITCH_STATS);
                    if (z_status < Z_OK) ZENOH_DISCONNECTED(false);
                    options.encoding = z_move(stats_encoding);
                    z_status         = z_publisher_put(z_loan(stats_pub), z_move(payload), &options);
                    if (z_status < Z_OK) ZENOH_DISCONNECTED(false);
                }

                /* Error occured */
                else if (tx_status != TX_NO_EVENTS) {
                    Error_Handler();
                }
            }

            /* Error occured */
            else if (tx_status != TX_NO_EVENTS) {
                Error_Handler();
            }
        }

        /* Schedule the next wakeup */
        next_wakeup = MIN(next_maintenance_time, next_publish_time);
        if (current_time < next_wakeup) {
            tx_thread_sleep_ms(next_wakeup - current_time);
        }

        /* Somehow we have gotten far behind so catch up */
        else if ((current_time - next_wakeup) > (MAX(SWITCH_MAINTENANCE_INTERVAL, SWITCH_PUBLISH_STATS_INTERVAL) * 3)) {
            next_maintenance_time = current_time;
            next_publish_time     = current_time;
        }

        /* TODO: If the current thread holds the switch mutex when it shouldn't report an error */
    }
}
