/*
 * utils.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"
#include "stdint.h"

void write_mac_addr(uint8_t *buf);

uint32_t tx_thread_sleep_ms(uint32_t ms);
uint32_t tx_time_get_ms();

#ifdef __cplusplus
}
#endif

#endif /* INC_UTILS_H_ */
