/*
 * switch_thread.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdatomic.h"
#include "ucdr/microcdr.h"
#include "main.h"

#include "tx_app.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "sja1105.h"
#include "sja1105q_default_conf.h"
#include "utils.h"
#include "encodings.h"
#include "state_machine.h"
#include "comms_thread.h"


#define CHECK(func)                                            \
    do {                                                       \
        sja_status = (func);                                   \
        sja1105_check_status_msg(&hsja1105, sja_status, true); \
    } while (0)

#define SWITCH_STATS_BUFFER_SIZE (128)


uint8_t   switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_handle;

static uint8_t switch_stats_buffer[SWITCH_STATS_BUFFER_SIZE];

atomic_uint_fast32_t sja1105_error_counter = 0;


/* Private function prototypes */
// static void sja1105_check_status_msg(sja1105_handle_t *dev, sja1105_status_t to_check, bool recurse);

/* Attemt to handle errors resulting from SJA1105 user function calls
 * NOTE: When the system error handler is called, it is assumed that if it returns (as opposed to restarting the chip) then the error has been fixed.
 */
// static void sja1105_check_status_msg(sja1105_handle_t *dev, sja1105_status_t to_check, bool recurse) {
//
//     /* Return immediately if everything is fine */
//     if (to_check == SJA1105_OK) return;
//
//     sja1105_status_t sja_status       = SJA1105_OK;
//     bool             error_solved = false;
//
//     /* to_check is an error, increment the counter and check what to do */
//     sja1105_error_counter++;
//     switch (to_check) {
//
//         /* TODO: Log an error, but continue */
//         case SJA1105_ALREADY_CONFIGURED_ERROR:
//             error_solved = true;
//             break;
//
//         /* Parameter errors cannot be corrected on the fly, only at compile time. */
//         case SJA1105_PARAMETER_ERROR:
//             break;
//
//         /* If there is a CRC error then rollback to the default config */
//         case SJA1105_CRC_ERROR:
//             sja1105_static_conf      = swv4_sja1105_static_config_default;
//             sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;
//             sja_status                   = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
//             error_solved             = true;
//             break;
//
//         /* If there is an error with the static configuration load the default config */
//         case SJA1105_STATIC_CONF_ERROR:
//             sja1105_static_conf      = swv4_sja1105_static_config_default;
//             sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;
//             sja_status                   = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
//             error_solved             = dev->initialised;
//             break;
//
//         /* If there is a RAM parity error the switch must be immediately reset */
//         case SJA1105_RAM_PARITY_ERROR:
//             sja_status       = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
//             error_solved = dev->initialised;
//             break;
//
//         /* Error has not been corrected */
//         default:
//             break;
//     }
//
//     /* A NEW ERROR has occured during the handling of the previous error... */
//     if (sja_status != SJA1105_OK) {
//         sja1105_error_counter++;
//
//         /* ...and the new error is the SAME as the previous error... */
//         if (sja_status == to_check) {
//
//             /* ...but the previous error was SOLVED: the new error is also solved */
//             if (error_solved)
//                 ;
//
//             /* ...and the previous error was NOT SOLVED: the problem is deeper, call the system error handler */
//             else
//                 Error_Handler();
//         }
//
//         /* ...and the new error is DIFFERENT from the previous error... */
//         else {
//
//             /* ...but the previous error was SOLVED: check the new error (recursively) */
//             if (error_solved) {
//                 if (recurse) {
//                     sja1105_error_counter--; /* Don't double count the new error */
//                     sja1105_check_status_msg(dev, sja_status, false);
//                 } else
//                     Error_Handler(); /* An error occurred while checking an error that occurred while checking an error. Yikes */
//             }
//
//             /* ...and the previous error was NOT SOLVED: the problem is deeper, call the system error handler */
//             else
//                 Error_Handler();
//         }
//         error_solved = true;
//     }
//
//     /* Unsolved error */
//     if (!error_solved) Error_Handler();
//
//     /* All errors have now been handled, check the sja_status registers just to be safe */
//     sja_status = SJA1105_CheckStatusRegisters(dev);
//     if (recurse) sja1105_check_status_msg(dev, sja_status, false);
//
//     /* An error occurring means the mutex could have been taken but not released. Release it now */
//     while (dev->callbacks->callback_give_mutex(dev) == SJA1105_OK);
// }


/* This thread perform regular maintenance for the switch */
void switch_thread_entry(uint32_t initial_input) {

    sja1105_status_t   sja_status;
    tx_status_t        tx_status;
    _z_res_t           z_status;
    int16_t            temperature;
    uint32_t           current_time          = tx_time_get_ms();
    uint32_t           next_publish_time     = current_time;
    uint32_t           next_maintenance_time = current_time;
    int32_t            next_wakeup           = 0;
    ucdrBuffer         stats_writer;
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
            sja_status = SJA1105_ReadTemperatureX10(&hsja1105, &temperature);
            if (sja_status != SJA1105_OK) Error_Handler();
            temperature /= 10;
        }

        /* Check if publishing stats is necessary */
        if (current_time >= next_publish_time) {
            next_publish_time += SWITCH_PUBLISH_STATS_INTERVAL;

            /* Check if publishing messages is allowed */
            uint32_t flags;
            tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_ZENOH_CONNECTED, TX_OR, &flags, TX_NO_WAIT);
            if (tx_status == TX_SUCCESS) {

                /* Get the stats and publish the message */

                // {
                //     bool port_states[SJA1105_NUM_PORTS];
                //     for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) {
                //         SJA1105_PortGetForwarding(&hsja1105, i, port_states + i);
                //     }
                //     ucdr_init_buffer(&writer, buffer, BUFFER_LENGTH);
                //     ucdr_serialize_array_bool(&writer, port_states, SJA1105_NUM_PORTS);
                // }

                /* Get the stats */
                sja1105_statistics_t stats;
                if (SJA1105_ReadStatistics(&hsja1105, &stats) != SJA1105_OK) Error_Handler();
                uint64_t total_received_bytes = 0;
                for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) total_received_bytes += stats.rx_bytes[i];
                ucdr_init_buffer(&stats_writer, switch_stats_buffer, SWITCH_STATS_BUFFER_SIZE);
                ucdr_serialize_uint64_t(&stats_writer, total_received_bytes);

                /* Create payload */
                z_owned_bytes_t payload;
                z_status = z_bytes_from_static_buf(&payload, switch_stats_buffer, stats_writer.offset);
                if (z_status < Z_OK) ZENOH_DISCONNECTED(false);

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
