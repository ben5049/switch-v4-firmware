/*
 * nx_stp.c
 *
 *  Created on: Aug 10, 2025
 *      Author: bens1
 */

#include "nx_api.h"
#include "nx_stm32_eth_driver.h"

#include "nx_app.h"
#include "nx_stp.h"
#include "stp_thread.h"


nx_stp_t nx_stp;


nx_status_t nx_stp_init(NX_IP *ip_ptr, char *name, TX_EVENT_FLAGS_GROUP *events) {

    nx_status_t status = NX_STATUS_SUCCESS;

    nx_stp.ip_ptr = ip_ptr;
    nx_stp.tx_packet_ptr        = NULL;
    nx_stp.rx_packet_queue_head = NULL;
    nx_stp.rx_packet_queue_tail = NULL;
    nx_stp.events = events;

    return status;
}


nx_status_t nx_stp_allocate_packet() {

    nx_status_t status     = NX_STATUS_SUCCESS;
    NX_PACKET  *packet_ptr = nx_stp.tx_packet_ptr;

    /* Check NetX has been initialised */
    if (nx_stp.ip_ptr->nx_ip_initialize_done != NX_TRUE) status = NX_NOT_ENABLED;
    if (status != NX_STATUS_SUCCESS) return status;

    /* Allocate a packet */
    status = nx_packet_allocate(&nx_packet_pool, &(packet_ptr), NX_PHYSICAL_HEADER, TX_WAIT_FOREVER);
    if (status != NX_STATUS_SUCCESS) return status;

    /* Check there is available space in the packet for the header */
    if ((ULONG) (packet_ptr->nx_packet_prepend_ptr - packet_ptr->nx_packet_data_start) < (BPDU_HEADER_SIZE - BPDU_LLC_SIZE)) status = NX_PACKET_OFFSET_ERROR;
    if (status != NX_STATUS_SUCCESS) return status;

    /* Assign the interface (this is done so the send request is passed to the ethernet driver).
     * Note that interface 0 is the loopback interface so use the first actual interface instead.
     */
    nx_stp.tx_packet_ptr->nx_packet_address.nx_packet_interface_ptr = &nx_stp.ip_ptr->nx_ip_interface[1];

    return status;
}


nx_status_t nx_stp_send_packet() {

    nx_status_t status     = NX_STATUS_SUCCESS;
    NX_PACKET  *packet_ptr = nx_stp.tx_packet_ptr;
    uint32_t   *ethernet_frame_ptr;

    /* Check packet is allocated */
    if (nx_stp.tx_packet_ptr == NULL) status = NX_STATUS_INVALID_PACKET;
    if (status != NX_STATUS_SUCCESS) return status;

    /* Check packet length */
    if (nx_stp.tx_packet_ptr->nx_packet_length == 0) status = NX_STATUS_INVALID_PACKET;
    if (status != NX_STATUS_SUCCESS) return status;

    /* Setup the ethernet frame pointer. Backup another 2 bytes to get 32-bit word alignment */
    ethernet_frame_ptr = (uint32_t *) (packet_ptr->nx_packet_prepend_ptr - 2);

    /* Change the endianess if required */
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_frame_ptr++));
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_frame_ptr++));
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_frame_ptr++));
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_frame_ptr++));

    /* Send the packet */
    status = nx_link_raw_packet_send(nx_stp.ip_ptr, 1, packet_ptr);

    /* Clear packet pointers to prevent retransmission */
    nx_stp.tx_packet_ptr = NULL;

    /* Add debug information */
    NX_PACKET_DEBUG(__FILE__, __LINE__, packet_ptr);

    return status;
}


/* Modeled after _nx_rarp_packet_deferred_receive() */
nx_status_t nx_stp_packet_deferred_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr) {

    nx_status_t status = NX_STATUS_SUCCESS;

    TX_INTERRUPT_SAVE_AREA

    /* Disable interrupts */
    TX_DISABLE

    /* Check to see if the STP deferred processing queue is empty */
    if (nx_stp.rx_packet_queue_head) {

        /* Not empty, place the packet at the end of the STP deferred queue */
        nx_stp.rx_packet_queue_tail->nx_packet_queue_next = packet_ptr;
        packet_ptr->nx_packet_queue_next                  = NX_NULL;
        nx_stp.rx_packet_queue_tail                       = packet_ptr;

        /* Restore interrupts */
        TX_RESTORE

    } else {

        /* Empty STP deferred receive processing queue. Setup the head pointers and
           set the event flags to ensure the STP thread looks at the STP deferred
           processing queue */
        nx_stp.rx_packet_queue_head      = packet_ptr;
        nx_stp.rx_packet_queue_tail      = packet_ptr;
        packet_ptr->nx_packet_queue_next = NX_NULL;

        /* Restore interrupts */
        TX_RESTORE

        /* Wake up STP thread to process the STP deferred receive */
        status = tx_event_flags_set(nx_stp.events, NX_STP_BPDU_REC_EVENT, TX_OR);
    }

    return status;
}
