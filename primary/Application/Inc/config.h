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

/* ---------------------------------------------------------------------------- */
/* Common Config */
/* ---------------------------------------------------------------------------- */

#define MAC_ADDR_OCTET1 0x00
#define MAC_ADDR_OCTET2 0x80
#define MAC_ADDR_OCTET3 0xE1
#define MAC_ADDR_OCTET4 0x00
#define MAC_ADDR_OCTET5 0x00
#define MAC_ADDR_OCTET6 0x00

/* ---------------------------------------------------------------------------- */
/* Switch Config */
/* ---------------------------------------------------------------------------- */

#define SWITCH_THREAD_STACK_SIZE        1024
#define SWITCH_THREAD_PRIORIY           15
#define SWITCH_THREAD_PREMPTION_PRIORIY 15

#define SWITCH_MEM_POOL_SIZE            (1024 * sizeof(uint32_t)) /* 1024 Words should be enough for most variable length tables. TODO Check */

/* ---------------------------------------------------------------------------- */
/* STP Config */
/* ---------------------------------------------------------------------------- */

#define STP_THREAD_STACK_SIZE        1024
#define STP_THREAD_PRIORIY           15
#define STP_THREAD_PREMPTION_PRIORIY 15

#define STP_MEM_POOL_SIZE            (1024 * sizeof(uint8_t))

/* ---------------------------------------------------------------------------- */
/* Commmunications Config */
/* ---------------------------------------------------------------------------- */

#define COMMS_THREAD_STACK_SIZE        1024
#define COMMS_THREAD_PRIORIY           15
#define COMMS_THREAD_PREMPTION_PRIORIY 15


#ifdef __cplusplus
}
#endif

#endif /* INC_CONFIG_H_ */
