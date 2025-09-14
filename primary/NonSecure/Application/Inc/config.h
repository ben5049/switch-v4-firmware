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
/* Thread Enables */
/* ---------------------------------------------------------------------------- */

#define ENABLE_STP_THREAD false /* TODO: fix this thread. Each time it calls for a flush the 50MHz REF_CLK is reset which is probably very bad */

/* ---------------------------------------------------------------------------- */
/* Common Config */
/* ---------------------------------------------------------------------------- */

#define LOGGING_STACK_SIZE (512) /* Any thread that calls secure logging functions should allocate this ammount of secure stack */

/* ---------------------------------------------------------------------------- */
/* State Machine Config */
/* ---------------------------------------------------------------------------- */

#define STATE_MACHINE_THREAD_STACK_SIZE         (1024)
#define STATE_MACHINE_THREAD_PRIORITY           (13)
#define STATE_MACHINE_THREAD_PREMPTION_PRIORITY (13)

/* ---------------------------------------------------------------------------- */
/* Networking Common Config */
/* ---------------------------------------------------------------------------- */

#define MAC_ADDR_OCTET1                  (0x2e)
#define MAC_ADDR_OCTET2                  (0x0d)
#define MAC_ADDR_OCTET3                  (0x8f)
#define MAC_ADDR_OCTET4                  (0xd3)
#define MAC_ADDR_OCTET5                  (0x6a)
#define MAC_ADDR_OCTET6                  (0x48)

#define NX_APP_DEFAULT_TIMEOUT           (10000)                                           /* Generic timeout for nx events (e.g. TCP send) in ms */
#define NX_APP_PACKET_POOL_SIZE          ((DEFAULT_PAYLOAD_SIZE + sizeof(NX_PACKET)) * 32) /* Enough space for 32 max size packets */

#define NX_DEFAULT_IP_ADDRESS            (0)                                               /* TODO: Set this */
#define NX_DEFAULT_NET_MASK              (0)                                               /* TODO: Set this */
#define NX_INTERNAL_IP_THREAD_STACK_SIZE (2 * 1024)
#define NX_INTERNAL_IP_THREAD_PRIORITY   (NX_APP_THREAD_PRIORITY)

#define DEFAULT_PAYLOAD_SIZE             (1536) /* Ethernet payload size field (0x600) */

#define DEFAULT_ARP_CACHE_SIZE           (1024)

#define NX_APP_THREAD_STACK_SIZE         (2 * 1024)
#define NX_APP_THREAD_PRIORITY           (10)

#define NUM_VLANS                        (8)    /* Currently only used for STP (unused due to RSTP not MSTP) */

#define PRIMARY_INTERFACE                (0)    /* Primary NetXduo interface (0 = first normal interface, 1 = loopback) */

#define PORT0_SPEED_MBPS                 (100)  /* 88Q2112 #1 (100 or 1000 Mbps) */
#define PORT1_SPEED_MBPS                 (1000) /* 88Q2112 #2 (100 or 1000 Mbps) */
#define PORT2_SPEED_MBPS                 (1000) /* 88Q2112 #3 (100 or 1000 Mbps) */
#define PORT3_SPEED_MBPS                 (10)   /* 10BASE-T1S (10 Mbps) */
#define PORT4_SPEED_MBPS                 (100)  /* Host (10 or 100 Mbps) */

/* ---------------------------------------------------------------------------- */
/* Link Config */
/* ---------------------------------------------------------------------------- */

#define NX_LINK_THREAD_STACK_SIZE            (2 * 1024)
#define NX_LINK_THREAD_PRIORITY              (9)

#define NX_APP_CABLE_CONNECTION_CHECK_PERIOD (1000) /* Interval between link checks in ms */

/* ---------------------------------------------------------------------------- */
/* PTP Config */
/* ---------------------------------------------------------------------------- */

#define NX_INTERNAL_PTP_THREAD_STACK_SIZE (1024)
#define NX_INTERNAL_PTP_THREAD_PRIORITY   (2) /* This must be very high priority. Firstly to minimise delays, and secondly to prevent another thread prempting it and sending a packet that receives the timestamp meant for a PTP packet. */

#define PTP_THREAD_STACK_SIZE             (1024)
#define PTP_THREAD_PRIORITY               (4)
#define PTP_TX_QUEUE_SIZE                 (10)
#define PTP_PRINT_TIME_INTERVAL           (1000) /* Time interval between printing the PTP time in ms. Must be >= 100ms. Set to UINT32_MAX to disable printing */

#define PTP_CLIENT_MASTER_SUB_PRIORITY    (248)  /* The subpriority of this device for BMCA. Default for an end instance is 248 */

/* ---------------------------------------------------------------------------- */
/* Switch Config */
/* ---------------------------------------------------------------------------- */

#define SWITCH_THREAD_STACK_SIZE         (2 * 1024)
#define SWITCH_THREAD_PRIORITY           (15)
#define SWITCH_THREAD_PREMPTION_PRIORITY (15)

#define SWITCH_MEM_POOL_SIZE             (1024 * sizeof(uint32_t)) /* 1024 Words should be enough for most variable length tables. TODO Check */

/* ---------------------------------------------------------------------------- */
/* PHY Config */
/* ---------------------------------------------------------------------------- */

#define NUM_PHYS                      (4)
#define PHY_TIMEOUT_MS                (100) /* Default timeout for PHY operations in ms */

#define PHY_THREAD_STACK_SIZE         (1024)
#define PHY_THREAD_PRIORITY           (15)
#define PHY_THREAD_PREMPTION_PRIORITY (15)
#define PHY_THREAD_INTERVAL           (1000) /* Execute once per second */

/* ---------------------------------------------------------------------------- */
/* STP Config */
/* ---------------------------------------------------------------------------- */

#define STP_THREAD_STACK_SIZE         (1024)
#define STP_THREAD_PRIORITY           (12)
#define STP_THREAD_PREMPTION_PRIORITY (12)

#define STP_MEM_POOL_SIZE             (2 * 1024 * sizeof(uint8_t))

/* ---------------------------------------------------------------------------- */
/* Commmunications Config */
/* ---------------------------------------------------------------------------- */

#define COMMS_THREAD_STACK_SIZE         (1024)
#define COMMS_THREAD_PRIORITY           (15)
#define COMMS_THREAD_PREMPTION_PRIORITY (15)


#ifdef __cplusplus
}
#endif

#endif /* INC_CONFIG_H_ */
