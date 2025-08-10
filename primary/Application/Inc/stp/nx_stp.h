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


#include "switch_thread.h"
#include "stdatomic.h"
#include "stdbool.h"
#include "hal.h"


#define NX_LINK_STP_SEND (NX_LINK_USER_COMMAND + 1)


extern atomic_bool bpdu_transmitted;

bool stp_ReleaseTxPacket(ETH_HandleTypeDef *heth);


#ifdef __cplusplus
}
#endif

#endif /* INC_STP_NX_STP_H_ */
