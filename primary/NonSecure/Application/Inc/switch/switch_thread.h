/*
 * switch.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_SWITCH_THREAD_H_
#define INC_SWITCH_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"
#include "stdint.h"
#include "stdatomic.h"
#include "hal.h"

#include "config.h"
#include "sja1105.h"


/* Enums */
typedef enum {
    PORT_88Q2112_PHY0 = 0x0,
    PORT_88Q2112_PHY1 = 0x1,
    PORT_88Q2112_PHY2 = 0x2,
    PORT_LAN8671_PHY  = 0x3,
    PORT_HOST         = 0x4,
} port_index_t;


/* Exported variables */
extern uint8_t              switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
extern TX_THREAD            switch_thread_handle;
extern atomic_uint_fast32_t sja1105_error_counter;
extern sja1105_handle_t     hsja1105;
extern const uint32_t      *sja1105_static_conf;
extern uint32_t             sja1105_static_conf_size;

/* Exported functions*/
sja1105_status_t switch_init(sja1105_handle_t *dev);
void             switch_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_THREAD_H_ */
