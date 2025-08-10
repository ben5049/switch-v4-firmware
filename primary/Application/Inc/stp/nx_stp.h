/*
 * stp_extra.h
 *
 *  Created on: Aug 3, 2025
 *      Author: bens1
 */

#ifndef INC_STP_NX_STP_H_
#define INC_STP_NX_STP_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdatomic.h"
#include "stdbool.h"
#include "nx_api.h"
#include "nx_app.h"
#include "hal.h"

#include "switch_thread.h"


#define BPDU_BPDU_MAX_BUFFER_SIZE   (128)
#define BPDU_DST_ADDR_SIZE          (6)
#define BPDU_SRC_ADDR_SIZE          (6)
#define BPDU_SIZE_OR_ETHERTYPE_SIZE (2)
#define BPDU_LLC_SIZE               (3)
#define BPDU_HEADER_SIZE            (BPDU_DST_ADDR_SIZE + BPDU_SRC_ADDR_SIZE + BPDU_SIZE_OR_ETHERTYPE_SIZE + BPDU_LLC_SIZE)

#define NX_STP_ALL_EVENTS           ((ULONG) 0xffffffff)
#define NX_STP_BPDU_REC_EVENT       ((ULONG) 0x00000008)


typedef struct {
    NX_PACKET            *tx_packet_ptr;
    uint8_t              *tx_packet_header_ptr;
    uint8_t              *tx_packet_payload_ptr;
    NX_IP                *ip_ptr;
    NX_PACKET            *rx_packet_queue_head;
    NX_PACKET            *rx_packet_queue_tail;
    TX_EVENT_FLAGS_GROUP *events;
} nx_stp_t;


extern nx_stp_t nx_stp;

nx_status_t nx_stp_init(NX_IP *ip_ptr, char *name, TX_EVENT_FLAGS_GROUP *events);
nx_status_t nx_stp_allocate_packet(void);
nx_status_t nx_stp_send_packet(void);
nx_status_t nx_stp_packet_deferred_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);

#ifdef __cplusplus
}
#endif

#endif /* INC_STP_NX_STP_H_ */
