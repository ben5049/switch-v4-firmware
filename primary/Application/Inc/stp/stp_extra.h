/*
 * stp_extra.h
 *
 *  Created on: Aug 3, 2025
 *      Author: bens1
 */

#ifndef INC_STP_STP_EXTRA_H_
#define INC_STP_STP_EXTRA_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdbool.h"
#include "hal.h"


extern volatile bool bpdu_transmitted;

bool stp_ReleaseTxPacket(ETH_HandleTypeDef *heth);


#ifdef __cplusplus
}
#endif

#endif /* INC_STP_STP_EXTRA_H_ */
