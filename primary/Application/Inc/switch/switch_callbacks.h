/*
 * sja1105_callbacks.h
 *
 *  Created on: Aug 5, 2025
 *      Author: bens1
 */

#ifndef INC_SWITCH_SJA1105_CALLBACKS_H_
#define INC_SWITCH_SJA1105_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "sja1105.h"


/* Exported variables */
extern TX_MUTEX                  sja1105_mutex_ptr;
extern const sja1105_callbacks_t sja1105_callbacks;

sja1105_status_t switch_byte_pool_init(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_SJA1105_CALLBACKS_H_ */
