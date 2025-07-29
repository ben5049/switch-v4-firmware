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


/* Because of the multiply, care should be taken when putting large values into these functions (>4,000,000) to make sure the don't overflow */
#define TICKS_TO_MS(ticks)  (((ticks) * 1000) / TX_TIMER_TICKS_PER_SECOND)
#define MS_TO_TICKS(ms)     (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)

#define CONSTRAIN(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

void write_mac_addr(uint8_t *buf);

uint32_t tx_thread_sleep_ms(uint32_t ms);
uint32_t tx_time_get_ms();

#ifdef __cplusplus
}
#endif

#endif /* INC_UTILS_H_ */
