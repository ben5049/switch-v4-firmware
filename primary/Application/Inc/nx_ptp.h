/*
 * nx_ptp.h
 *
 *  Created on: Aug 11, 2025
 *      Author: bens1
 */

#ifndef INC_NX_PTP_H_
#define INC_NX_PTP_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "hal.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"


typedef struct {
    NX_PACKET  *packet_ptr;
    NX_PTP_TIME timestamp;
} nx_ptp_tx_info_t;


extern TX_THREAD nx_ptp_tx_thread_ptr;
extern uint8_t   nx_ptp_tx_thread_stack[NX_PTP_TX_THREAD_STACK_SIZE];
extern TX_QUEUE  nx_ptp_tx_queue_ptr;
extern uint8_t   nx_ptp_tx_queue_stack[NX_PTP_TX_QUEUE_SIZE * sizeof(nx_ptp_tx_info_t)];


UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation, NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr, VOID *callback_data);
void HAL_ETH_TxPtpCallback(uint32_t *buff, ETH_TimeStampTypeDef *timestamp);
void nx_ptp_tx_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_NX_PTP_H_ */
