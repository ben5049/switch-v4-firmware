/*
 * nx_l2_protocol.c
 *
 *  Created on: Aug 10, 2025
 *      Author: bens1
 *
 */

#include "nx_api.h"

#include "nx_app.h"
#include "nx_l2_protocol.h"
#include "nx_packet.h"


/* Modeled after _nx_ip_driver_packet_send() */
nx_status_t nx_l2_driver_packet_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr) {

    nx_status_t  status = NX_STATUS_SUCCESS;
    NX_IP_DRIVER driver_request;

    /* Add debug information. */
    NX_PACKET_DEBUG(__FILE__, __LINE__, packet_ptr);

    /* Initialize the driver request. */
    driver_request.nx_ip_driver_ptr       = ip_ptr;
    driver_request.nx_ip_driver_packet    = packet_ptr;
    driver_request.nx_ip_driver_interface = packet_ptr->nx_packet_address.nx_packet_interface_ptr;
    driver_request.nx_ip_driver_command   = NX_LINK_L2_PACKET_SEND;

    /* Check whether the packet should be sent through driver. */
    if (driver_request.nx_ip_driver_interface) {

/* TODO: Decide if layer 2 protocols should incrment these counters*/
#ifndef NX_DISABLE_IP_INFO

        /* Increment the IP packet sent count.  */
        ip_ptr->nx_ip_total_packets_sent++;

        /* Increment the IP bytes sent count.  */
        ip_ptr->nx_ip_total_bytes_sent += packet_ptr->nx_packet_length;
#endif

        /* If trace is enabled, insert this event into the trace buffer.  */
        NX_TRACE_IN_LINE_INSERT(NX_TRACE_INTERNAL_IO_DRIVER_PACKET_SEND, ip_ptr, packet_ptr, packet_ptr->nx_packet_length, 0, NX_TRACE_INTERNAL_EVENTS, 0, 0);

        /* Add debug information. */
        NX_PACKET_DEBUG(__FILE__, __LINE__, packet_ptr);

        /* Driver entry must not be NULL. */
        NX_ASSERT(packet_ptr->nx_packet_address.nx_packet_interface_ptr->nx_interface_link_driver_entry != NX_NULL);

        /* Broadcast packet.  */
        (packet_ptr->nx_packet_address.nx_packet_interface_ptr->nx_interface_link_driver_entry)(&driver_request);
    } else {

        /* Release the transmit packet.  */
        status = _nx_packet_release(packet_ptr);
        if (status != NX_STATUS_SUCCESS) return status;
    }

    return status;
}