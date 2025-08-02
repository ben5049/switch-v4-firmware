/*
 * stp_callbacks.h
 *
 *  Created on: Aug 2, 2025
 *      Author: bens1
 */

#ifndef INC_STP_STP_CALLBACKS_H_
#define INC_STP_STP_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "hal.h"
#include "stdbool.h"

#include "stp.h"


extern const STP_CALLBACKS stp_callbacks;

void bpdu_packet_init(void);
bool stp_ReleaseTxPacket(ETH_HandleTypeDef *heth);


#ifdef __cplusplus
}
#endif

#endif /* INC_STP_STP_CALLBACKS_H_ */
