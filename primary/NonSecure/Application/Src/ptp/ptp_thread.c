/*
 * ptp_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "math.h"
#include "stdint.h"
#include "hal.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"

#include "nx_app.h"
#include "ptp_callbacks.h"
#include "utils.h"
#include "config.h"


SHORT ptp_utc_offset = 0;

TX_THREAD ptp_thread_handle;
uint8_t   ptp_thread_stack[PTP_THREAD_STACK_SIZE];

TX_QUEUE ptp_tx_queue_handle; /* Stores the timestamps and pointers to sent packets */
uint8_t  ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * sizeof(nx_ptp_tx_info_t)];


/* This Thread starts the PTP client, processes transmitted timestamps, and prints status information */
void ptp_thread_entry(uint32_t initial_input) {

    TX_INTERRUPT_SAVE_AREA

    static nx_status_t      status = NX_STATUS_SUCCESS;
    static nx_ptp_tx_info_t tx_info;
    static NX_PACKET       *packet_ptr;
    static NX_PTP_TIME      timestamp;

    static NX_PTP_TIME      time;
    static NX_PTP_DATE_TIME date;

    static uint32_t next_print_time;
    static uint32_t current_time;

    /* Start the PTP client */
    nx_ptp_client_start(&ptp_client, NX_NULL, 0, 0, 0, ptp_event_callback, NX_NULL);
    nx_ptp_client_master_enable(&ptp_client, NX_PTP_CLIENT_ROLE_SLAVE_AND_MASTER, NX_PTP_CLIENT_MASTER_PRIORITY, PTP_CLIENT_MASTER_SUB_PRIORITY, NX_PTP_CLIENT_MASTER_CLOCK_CLASS, NX_PTP_CLIENT_MASTER_ACCURACY, NX_PTP_CLIENT_MASTER_CLOCK_VARIANCE, NX_PTP_CLIENT_MASTER_CLOCK_STEPS_REMOVED, NX_NULL); /* Enable master mode with the lowest priority so it is only used as a last restort. TODO: Randomise or make different */

    while (1) {

        /* Receive transmitted packet information from the queue */
        status = tx_queue_receive(&ptp_tx_queue_handle, &tx_info, CONSTRAIN(PTP_PRINT_TIME_INTERVAL, MS_TO_TICKS(100), TX_WAIT_FOREVER));

        /* Successfully transmitted packet: update the PTP client */
        if (status == NX_SUCCESS) {

            /* TODO: Check this logic */

            /* Unpack the information */
            packet_ptr = tx_info.packet_ptr;
            timestamp  = tx_info.timestamp;

            TX_DISABLE

#ifdef NX_ENABLE_GPTP

            /* Process t1 send time */
            if ((ptp_client.nx_ptp_client_pdelay_initiator_state == NX_PTP_CLIENT_PDELAY_WAIT_REQ_TS) &&
                (ptp_client.nx_ptp_client_pdelay_req_packet_ptr == packet_ptr)) {

                /* Store timestamp */
                COPY_NX_TIMESTAMP(ptp_client.nx_ptp_client_pdelay_req_ts, timestamp);

                /* Update state */
                ptp_client.nx_ptp_client_pdelay_initiator_state = NX_PTP_CLIENT_PDELAY_WAIT_RESP;
            }

            /* Process t3 response time */
            if (ptp_client.nx_ptp_client_pdelay_resp_packet_ptr == packet_ptr) {

                /* Store timestamp */
                COPY_NX_TIMESTAMP(ptp_client.nx_ptp_client_pdelay_resp_origin, timestamp);

                /* Set timer event */
                tx_event_flags_set(&(ptp_client.nx_ptp_client_events), NX_PTP_CLIENT_PDELAY_FOLLOW_EVENT, TX_OR);

                ptp_client.nx_ptp_client_pdelay_resp_packet_ptr = NX_NULL;
            }

#endif /* NX_ENABLE_GPTP */

            /* Get timestamp of previous delay_req message */
            if ((ptp_client.nx_ptp_client_delay_state == NX_PTP_CLIENT_DELAY_WAIT_REQ_TS) &&
                (ptp_client.nx_ptp_client_delay_req_packet_ptr == packet_ptr)) {

                /* Store timestamp */
                COPY_NX_TIMESTAMP(ptp_client.nx_ptp_client_delay_ts, timestamp);

                /* Update state */
                ptp_client.nx_ptp_client_delay_state = NX_PTP_CLIENT_DELAY_WAIT_RESP;
            }

#if defined(NX_PTP_ENABLE_MASTER) || defined(NX_PTP_ENABLE_REVERSE_SYNC)

            /* Get timestamp of previous sync message */
            if (ptp_client.nx_ptp_client_sync_packet_ptr == packet_ptr) {

                ptp_client.nx_ptp_client_sync_packet_ptr = NX_NULL;

                /* Store timestamp */
                COPY_NX_TIMESTAMP(ptp_client.nx_ptp_client_sync_ts_local, timestamp);

                /* Set follow up event */
                tx_event_flags_set(&(ptp_client.nx_ptp_client_events), NX_PTP_CLIENT_SYNC_FOLLOW_EVENT, TX_OR);
            }

#endif /* defined(NX_PTP_ENABLE_MASTER) || defined(NX_PTP_ENABLE_REVERSE_SYNC) */

            TX_RESTORE
        }
    }

#if (PTP_PRINT_TIME_INTERVAL != UINT32_MAX)

    /* Get, convert, and print the PTP time (this ironically uses the non-precise threadx time to delay between prints) */
    current_time = tx_time_get_ms();
    if (current_time >= next_print_time) {
        nx_ptp_client_time_get(&ptp_client, &time);
        nx_ptp_client_utility_convert_time_to_date(&time, -ptp_utc_offset, &date);
        printf("%2u/%02u/%u %02u:%02u:%02u.%09lu\r\n", date.day, date.month, date.year, date.hour, date.minute, date.second, date.nanosecond);
        next_print_time = current_time + PTP_PRINT_TIME_INTERVAL;
    }

#endif /* (PTP_PRINT_TIME_INTERVAL != TX_WAIT_FOREVER) */
}
