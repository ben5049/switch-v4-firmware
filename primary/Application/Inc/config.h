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

#define MAC_ADDR_OCTET1 0x00
#define MAC_ADDR_OCTET2 0x80
#define MAC_ADDR_OCTET3 0xE1
#define MAC_ADDR_OCTET4 0x00
#define MAC_ADDR_OCTET5 0x00
#define MAC_ADDR_OCTET6 0x00

#define SWITCH_THREAD_STACK_SIZE 1024
#define SWITCH_THREAD_PRIORIY 15
#define SWITCH_THREAD_PREMPTION_PRIORIY 15

#define STP_THREAD_STACK_SIZE 1024
#define STP_THREAD_PRIORIY 15
#define STP_THREAD_PREMPTION_PRIORIY 15


#ifdef __cplusplus
}
#endif

#endif /* INC_CONFIG_H_ */
