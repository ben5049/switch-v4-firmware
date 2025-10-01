/*
 * diagnostics_thread.c
 *
 *  Created on: Oct 1, 2025
 *      Author: bens1
 */


// {
//     bool port_states[SJA1105_NUM_PORTS];
//     for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) {
//         SJA1105_PortGetForwarding(&hsja1105, i, port_states + i);
//     }
//     ucdr_init_buffer(&writer, buffer, BUFFER_LENGTH);
//     ucdr_serialize_array_bool(&writer, port_states, SJA1105_NUM_PORTS);
// }






//{
//    sja1105_statistics_t stats;
//    if (SJA1105_ReadStatistics(&hsja1105, &stats) != SJA1105_OK) Error_Handler();
//    uint64_t total_received_bytes = 0;
//    for (uint_fast8_t i = 0; i < SJA1105_NUM_PORTS; i++) total_received_bytes += stats.rx_bytes[i];
//    ucdr_init_buffer(&writer, buffer, BUFFER_LENGTH);
//    ucdr_serialize_uint64_t(&writer, total_received_bytes);
//}
//
///* Create payload */
//z_owned_bytes_t payload;
//z_status = z_bytes_copy_from_buf(&payload, buffer, writer.offset);
//if (z_status < Z_OK) goto restart;
//
///* Publish the message */
//z_publisher_put_options_t options;
//z_publisher_put_options_default(&options);
//z_status = z_publisher_put(z_loan(stats_pub), z_move(payload), &options);
//if (z_status < Z_OK) goto restart;
