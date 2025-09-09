/*
 * phy_callbacks.h
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#ifndef INC_PHY_CALLBACKS_H_
#define INC_PHY_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "88q211x.h"
#include "lan867x.h"


#define PHY_ALL_EVENTS ((ULONG) 0xffffffff)
#define PHY_PHY0_EVENT ((ULONG) 1 << 0)
#define PHY_PHY1_EVENT ((ULONG) 1 << 1)
#define PHY_PHY2_EVENT ((ULONG) 1 << 2)
#define PHY_PHY3_EVENT ((ULONG) 1 << 3)


extern const phy_callbacks_t phy_callbacks_88q2112;
extern const phy_callbacks_t phy_callbacks_lan8671;

extern TX_MUTEX phy_mutex_handle;

extern TX_EVENT_FLAGS_GROUP phy_events_handle;


#ifdef __cplusplus
}
#endif

#endif /* INC_PHY_CALLBACKS_H_ */
