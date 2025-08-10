/*
 * config.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"


/* ---------------------------------------------------------------------------- */
/* Networking Common Config */
/* ---------------------------------------------------------------------------- */

#define MAC_ADDR_OCTET1                      (0x00)
#define MAC_ADDR_OCTET2                      (0x80)
#define MAC_ADDR_OCTET3                      (0xE1)
#define MAC_ADDR_OCTET4                      (0x00)
#define MAC_ADDR_OCTET5                      (0x00)
#define MAC_ADDR_OCTET6                      (0x00)

#define NX_APP_CABLE_CONNECTION_CHECK_PERIOD (1 * TX_TIMER_TICKS_PER_SECOND)  /* Check every second if the link is up */
#define NX_APP_DEFAULT_TIMEOUT               (10 * TX_TIMER_TICKS_PER_SECOND) /* Timeout after 10 seconds */
#define NX_APP_PACKET_POOL_SIZE              ((DEFAULT_PAYLOAD_SIZE + sizeof(NX_PACKET)) * 10)
#define NX_IP_INSTANCE_THREAD_SIZE           (2 * 1024)
#define NX_APP_INSTANCE_PRIORITY             (NX_APP_THREAD_PRIORITY)
#define NX_APP_DEFAULT_IP_ADDRESS            (0)
#define NX_APP_DEFAULT_NET_MASK              (0)

#define DEFAULT_PAYLOAD_SIZE                 (1536)
#define DEFAULT_ARP_CACHE_SIZE               (1024)

#define NX_APP_THREAD_STACK_SIZE             (2 * 1024)
#define NX_APP_THREAD_PRIORITY               (10)

#define NX_LINK_THREAD_STACK_SIZE            (2 * 1024)
#define NX_LINK_THREAD_PRIORITY              (10)

/* ---------------------------------------------------------------------------- */
/* Switch Config */
/* ---------------------------------------------------------------------------- */

#define SWITCH_THREAD_STACK_SIZE        (1024)
#define SWITCH_THREAD_PRIORIY           (15)
#define SWITCH_THREAD_PREMPTION_PRIORIY (15)

#define SWITCH_MEM_POOL_SIZE            (1024 * sizeof(uint32_t)) /* 1024 Words should be enough for most variable length tables. TODO Check */

/* ---------------------------------------------------------------------------- */
/* STP Config */
/* ---------------------------------------------------------------------------- */

#define STP_THREAD_STACK_SIZE        (1024)
#define STP_THREAD_PRIORIY           (15)
#define STP_THREAD_PREMPTION_PRIORIY (15)

#define STP_MEM_POOL_SIZE            (1024 * sizeof(uint8_t))

/* ---------------------------------------------------------------------------- */
/* Commmunications Config */
/* ---------------------------------------------------------------------------- */

#define COMMS_THREAD_STACK_SIZE        (1024)
#define COMMS_THREAD_PRIORIY           (15)
#define COMMS_THREAD_PREMPTION_PRIORIY (15)


#ifdef __cplusplus
}
#endif

#endif /* INC_CONFIG_H_ */
