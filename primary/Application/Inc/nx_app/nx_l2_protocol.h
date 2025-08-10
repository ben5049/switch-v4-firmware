/*
 * nx_l2_protocol.h
 *
 *  Created on: Aug 10, 2025
 *      Author: bens1
 */

#ifndef INC_NX_L2_PROTOCOL_H_
#define INC_NX_L2_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"

#include "nx_app.h"


#define NX_LINK_L2_PACKET_SEND (NX_LINK_USER_COMMAND + 1)
#define NX_L2_PACKET           (NX_PHYSICAL_HEADER) /* Layer 2 packets have no IP or protocol header */


nx_status_t nx_l2_driver_packet_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr);


#ifdef __cplusplus
}
#endif

#endif /* INC_NX_L2_PROTOCOL_H_ */
