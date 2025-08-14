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


extern const phy_callbacks_t phy_callbacks_88q2112;
extern const phy_callbacks_t phy_callbacks_lan8671;


#ifdef __cplusplus
}
#endif

#endif /* INC_PHY_CALLBACKS_H_ */
