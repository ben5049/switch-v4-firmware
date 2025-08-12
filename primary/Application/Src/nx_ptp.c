/*
 * nx_ptp.c
 *
 *  Created on: Aug 11, 2025
 *      Author: bens1
 */

#include "math.h"
#include "stdint.h"
#include "hal.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"

#include "nx_app.h"
#include "nx_ptp.h"
#include "utils.h"
#include "config.h"


#define NX_PTP_NANOSECONDS_PER_SEC 1000000000L

#define COPY_NX_TIMESTAMP(into, from)            \
    do {                                         \
        (into).nanosecond  = (from).nanosecond;  \
        (into).second_low  = (from).second_low;  \
        (into).second_high = (from).second_high; \
    } while (0)


TX_THREAD nx_ptp_tx_thread_ptr;
uint8_t   nx_ptp_tx_thread_stack[NX_PTP_TX_THREAD_STACK_SIZE];

TX_QUEUE nx_ptp_tx_queue_ptr; /* Stores the timestamps and pointers to sent packets */
uint8_t  nx_ptp_tx_queue_stack[NX_PTP_TX_QUEUE_SIZE * sizeof(nx_ptp_tx_info_t)];

SHORT ptp_utc_offset = 0;


extern ETH_HandleTypeDef heth;


static UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data);


/* Clock callback for NetX PTP client */
UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation, NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr, VOID *callback_data) {

    TX_INTERRUPT_SAVE_AREA

    NX_PARAMETER_NOT_USED(callback_data);

    nx_status_t     status = NX_STATUS_SUCCESS;
    ETH_TimeTypeDef eth_time;

    switch (operation) {

        /* Initialise the PTP clock in the ethernet peripheral */
        case NX_PTP_CLIENT_CLOCK_INIT:

            /* Increment is the period of the PTP clock. TimestampRolloverMode = 1 so the timer value is 1:1 with ns.
             * Using TimestampRolloverMode = 0 would improve the resolution from 1ns to 0.465ns, but this is enough
             * for most applications and simplifies timer reading and writing.
             */
            uint32_t increment = 20;                                                                                                         /* 20ns */
            uint32_t addend    = (((uint64_t) 1 << 32) * (uint64_t) 1000000000) / ((uint64_t) increment * (uint64_t) HAL_RCC_GetHCLKFreq()); /* 0x33333333 for 250MHx HCLK*/

            /* Configure the ethernet timestamping register for PTP */
            ETH_PTP_ConfigTypeDef ptp_config;

            /* Get the current config */
            if (HAL_ETH_PTP_GetConfig(&heth, &ptp_config) != HAL_OK) status = NX_STATUS_OPTION_ERROR;
            if (status != NX_STATUS_SUCCESS) return status;

            /* Update the config */
            ptp_config.TimestampUpdateMode   = ENABLE; /* Fine mode */
            ptp_config.TimestampAddend       = addend;
            ptp_config.TimestampAddendUpdate = ENABLE;
            ptp_config.TimestampAll          = DISABLE;
            ptp_config.TimestampRolloverMode = ENABLE;                   /* Every 1,000,000,000 nanoseconds the seconds count is incremented */
            ptp_config.TimestampV2           = ENABLE;                   /* IEE 1588-2008 (PTPv2) enabled */
            ptp_config.TimestampEthernet     = ENABLE;                   /* Enable processing of PTP frames embedded directly in ethernet packets */
            ptp_config.TimestampIPv6         = ENABLE;                   /* Enable processing of PTP frames embedded in IPv6-UDP packets */
            ptp_config.TimestampIPv4         = ENABLE;                   /* Enable processing of PTP frames embedded in IPv4-UDP packets */
            ptp_config.TimestampEvent        = DISABLE;                  /* ┐ These settings mean timestamps are taken for SYNC, Follow_Up, Delay_Req, */
            ptp_config.TimestampMaster       = DISABLE;                  /* ├ Delay_Resp, Pdelay_Req, Pdelay_Resp, and Pdelay_Resp_Follow_Up messages. */
            ptp_config.TimestampSnapshots    = ENABLE;                   /* ┘ These are all the master and slave message types. */
            ptp_config.TimestampFilter       = DISABLE;                  /* Don't bother filtering by destination address */
            ptp_config.TimestampStatusMode   = DISABLE;                  /* Don't overwrite transmit timestamps */
            ptp_config.TimestampSubsecondInc = (increment & 0xff) << 16; /* For a 50MHz PTP clock increment by 20ns each time (RM0481 page 2935)*/

            /* Write the new config */
            if (HAL_ETH_PTP_SetConfig(&heth, &ptp_config) != HAL_OK) status = NX_STATUS_OPTION_ERROR;
            if (status != NX_STATUS_SUCCESS) return status;
            break;

        /* Set clock time */
        case NX_PTP_CLIENT_CLOCK_SET:

            /* Can't set the nanoseconds to a negative number */
            if (time_ptr->nanosecond < 0) status = NX_STATUS_NOT_SUPPORTED;
            if (status != NX_STATUS_SUCCESS) return status;

            /* Update the time */
            TX_DISABLE
            eth_time.NanoSeconds = time_ptr->nanosecond;
            eth_time.Seconds     = time_ptr->second_low;
            TX_RESTORE

            if (HAL_ETH_PTP_SetTime(&heth, &eth_time) != HAL_OK) status = NX_STATUS_NOT_ENABLED;
            if (status != NX_STATUS_SUCCESS) return status;
            break;

        /* Extract timestamp from packet. This is stored before the payload where the MAC addresses & ethertype/size used to be */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_EXTRACT:

            /* Check PTP has been initialised */
            if (heth.IsPtpConfigured != HAL_ETH_PTP_CONFIGURED) status = NX_STATUS_NOT_ENABLED;
            if (status != NX_STATUS_SUCCESS) return status;

            /* Get the timestamp */
            time_ptr->nanosecond  = ((uint32_t *) packet_ptr->nx_packet_data_start)[0];
            time_ptr->second_low  = ((uint32_t *) packet_ptr->nx_packet_data_start)[1];
            time_ptr->second_high = ((uint32_t *) packet_ptr->nx_packet_data_start)[2];
            break;

        /* Get clock time */
        case NX_PTP_CLIENT_CLOCK_GET:

            /* Get the time */
            if (HAL_ETH_PTP_GetTime(&heth, &eth_time) != HAL_OK) status = NX_STATUS_NOT_ENABLED;
            if (status != NX_STATUS_SUCCESS) return status;

            TX_DISABLE
            time_ptr->nanosecond = eth_time.NanoSeconds;
            time_ptr->second_low = eth_time.Seconds;
            TX_RESTORE
            break;

        /* Adjust clock time */
        case NX_PTP_CLIENT_CLOCK_ADJUST:

            /* Enforce min/max values of offset */
            int32_t offset_ns = time_ptr->nanosecond;
            if (offset_ns > NX_PTP_NANOSECONDS_PER_SEC) {
                offset_ns = NX_PTP_NANOSECONDS_PER_SEC;
            } else if (offset_ns < -NX_PTP_NANOSECONDS_PER_SEC) {
                offset_ns = -NX_PTP_NANOSECONDS_PER_SEC;
            }

            /* Form the ETH_TimeTypeDef struct */
            eth_time.NanoSeconds = abs(offset_ns);
            eth_time.Seconds     = time_ptr->second_low;

            /* Update the time */
            if (offset_ns >= 0) {
                if (HAL_ETH_PTP_AddTimeOffset(&heth, HAL_ETH_PTP_POSITIVE_UPDATE, &eth_time) != HAL_OK) status = NX_STATUS_NOT_ENABLED;
            } else {
                if (HAL_ETH_PTP_AddTimeOffset(&heth, HAL_ETH_PTP_NEGATIVE_UPDATE, &eth_time) != HAL_OK) status = NX_STATUS_NOT_ENABLED;
            }
            if (status != NX_STATUS_SUCCESS) return status;
            break;

        /* Prepare timestamp for current packet */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_PREPARE:
            if (HAL_ETH_PTP_InsertTxTimestamp(&heth) != HAL_OK) status = NX_STATUS_NOT_ENABLED;
            if (status != NX_STATUS_SUCCESS) return status;
            break;

        /* Update soft timer */
        case NX_PTP_CLIENT_CLOCK_SOFT_TIMER_UPDATE:
            TX_DISABLE

            /* increment the nanosecond field of the software clock */
            time_ptr->nanosecond +=
                (LONG) (NX_PTP_NANOSECONDS_PER_SEC / NX_PTP_CLIENT_TIMER_TICKS_PER_SECOND);

            /* update the second field */
            if (time_ptr->nanosecond >= NX_PTP_NANOSECONDS_PER_SEC) {
                time_ptr->nanosecond -= NX_PTP_NANOSECONDS_PER_SEC;
                _nx_ptp_client_utility_inc64(&(time_ptr->second_high),
                                             &(time_ptr->second_low));
            }
            TX_RESTORE
            break;

        default:
            return (NX_PTP_PARAM_ERROR);
    }

    return (NX_SUCCESS);
}


void HAL_ETH_TxPtpCallback(uint32_t *buff, ETH_TimeStampTypeDef *timestamp) {

    /* Input validation */
    if (buff == NULL || timestamp == NULL) return;

    /* Create the tx info struct */
    nx_ptp_tx_info_t tx_info = {
        .packet_ptr = (NX_PACKET *) buff,
        .timestamp  = {
                       .nanosecond  = timestamp->TimeStampLow,
                       .second_low  = timestamp->TimeStampHigh,
                       .second_high = 0,
                       }
    };

    /* Send the info to be processed */
    tx_queue_send(&nx_ptp_tx_queue_ptr, &tx_info, TX_NO_WAIT);
}


void nx_ptp_tx_thread_entry(uint32_t initial_input) {

    TX_INTERRUPT_SAVE_AREA

    nx_ptp_tx_info_t tx_info;
    NX_PACKET       *packet_ptr;
    NX_PTP_TIME      timestamp;

    NX_PTP_TIME      time;
    NX_PTP_DATE_TIME date;

    /* start the PTP client */
    nx_ptp_client_start(&ptp_client, NX_NULL, 0, 0, 0, ptp_event_callback, NX_NULL);
    nx_ptp_client_master_enable(&ptp_client, NX_PTP_CLIENT_ROLE_SLAVE_AND_MASTER, 255, 248, 248, 254, NX_NULL, NX_NULL, NX_NULL); /* Enable master mode with the lowest priority so it is only used as a last restort. TODO: Randomise or make different */

    while (1) {

        /* Receive transmitted packet information from the queue */
        if (tx_queue_receive(&nx_ptp_tx_queue_ptr, &tx_info, NX_IP_PERIODIC_RATE) != TX_SUCCESS) {

            /* read the PTP clock */
            nx_ptp_client_time_get(&ptp_client, &time);

            /* convert PTP time to UTC date and time */
            nx_ptp_client_utility_convert_time_to_date(&time, -ptp_utc_offset, &date);

            /* display the current time */
            printf("%2u/%02u/%u %02u:%02u:%02u.%09lu\r\n", date.day, date.month, date.year, date.hour, date.minute, date.second, date.nanosecond);

            continue;
        }

        /* Successfully transmitted packet: update the PTP client */
        /* TODO: Check this logic */

        /* Unpack the information */
        packet_ptr = tx_info.packet_ptr;
        timestamp  = tx_info.timestamp;

        TX_DISABLE

#ifdef NX_ENABLE_GPTP

        /* process t1 send time */
        if ((ptp_client.nx_ptp_client_pdelay_initiator_state == NX_PTP_CLIENT_PDELAY_WAIT_REQ_TS) &&
            (ptp_client.nx_ptp_client_pdelay_req_packet_ptr == packet_ptr)) {

            /* store timestamp */
            COPY_NX_TIMESTAMP(ptp_client.nx_ptp_client_pdelay_req_ts, timestamp);

            /* update state */
            ptp_client.nx_ptp_client_pdelay_initiator_state = NX_PTP_CLIENT_PDELAY_WAIT_RESP;
        }

        /* process t3 response time */
        if (ptp_client.nx_ptp_client_pdelay_resp_packet_ptr == packet_ptr) {

            /* store timestamp */
            COPY_NX_TIMESTAMP(ptp_client.nx_ptp_client_pdelay_resp_origin, timestamp);

            /* set timer event */
            tx_event_flags_set(&(ptp_client.nx_ptp_client_events), NX_PTP_CLIENT_PDELAY_FOLLOW_EVENT, TX_OR);

            ptp_client.nx_ptp_client_pdelay_resp_packet_ptr = NX_NULL;
        }

#endif /* NX_ENABLE_GPTP */

        /* get timestamp of previous delay_req message */
        if ((ptp_client.nx_ptp_client_delay_state == NX_PTP_CLIENT_DELAY_WAIT_REQ_TS) &&
            (ptp_client.nx_ptp_client_delay_req_packet_ptr == packet_ptr)) {

            /* store timestamp */
            COPY_NX_TIMESTAMP(ptp_client.nx_ptp_client_delay_ts, timestamp);

            /* update state */
            ptp_client.nx_ptp_client_delay_state = NX_PTP_CLIENT_DELAY_WAIT_RESP;
        }

#if defined(NX_PTP_ENABLE_MASTER) || defined(NX_PTP_ENABLE_REVERSE_SYNC)

        /* get timestamp of previous sync message */
        if (ptp_client.nx_ptp_client_sync_packet_ptr == packet_ptr) {

            ptp_client.nx_ptp_client_sync_packet_ptr = NX_NULL;

            /* store timestamp */
            COPY_NX_TIMESTAMP(ptp_client.nx_ptp_client_sync_ts_local, timestamp);

            /* set follow up event */
            tx_event_flags_set(&(ptp_client.nx_ptp_client_events), NX_PTP_CLIENT_SYNC_FOLLOW_EVENT, TX_OR);
        }
#endif /* defined(NX_PTP_ENABLE_MASTER) || defined(NX_PTP_ENABLE_REVERSE_SYNC) */

        TX_RESTORE
    }
}


static UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data) {
    NX_PARAMETER_NOT_USED(callback_data);

    switch (event) {
        case NX_PTP_CLIENT_EVENT_MASTER: {
            printf("new MASTER clock!\r\n");
            break;
        }

        case NX_PTP_CLIENT_EVENT_SYNC: {
            nx_ptp_client_sync_info_get((NX_PTP_CLIENT_SYNC *) event_data, NX_NULL, &ptp_utc_offset);
            printf("SYNC event: utc offset=%d\r\n", ptp_utc_offset);
            break;
        }

        case NX_PTP_CLIENT_EVENT_TIMEOUT: {
            printf("Master clock TIMEOUT!\r\n");
            break;
        }
        default: {
            break;
        }
    }

    return 0;
}
