/*
 * nx_link_driver.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 *
 *  This file is the equivalent to nx_stm32_phy_driver.c
 *
 */

#include "tx_api.h"
#include "nx_stm32_eth_config.h"
#include "nx_stm32_phy_driver.h"

#include "nx_app.h"
#include "switch_thread.h"


/* TODO: Rethink this, both the PHYs and the switch need to be initialised */
int32_t nx_eth_phy_init(void) {

    int32_t ret = ETH_PHY_STATUS_OK;

    /* Check the initialised flag and return an error if not initialised */
    if (!hsja1105.initialised) ret = ETH_PHY_STATUS_ERROR;

    return ret;
}

int32_t nx_eth_phy_get_link_state(void) {

    int32_t          linkstate = ETH_PHY_STATUS_LINK_ERROR;
    sja1105_status_t status    = SJA1105_OK;
    sja1105_speed_t  speed;
    bool             forwarding;

    /* If SJA1105 isn't initialised return link down */
    if (!hsja1105.initialised) {
        int32_t linkstate = ETH_PHY_STATUS_LINK_DOWN;
        return linkstate;
    }

    /* Check the forwarding state */
    status = SJA1105_PortGetForwarding(&hsja1105, hsja1105.config->host_port, &forwarding);
    if (status != SJA1105_OK) {
        linkstate = ETH_PHY_STATUS_LINK_ERROR;
        return linkstate;
    } else if (!forwarding) {
        linkstate = ETH_PHY_STATUS_LINK_DOWN;
        return linkstate;
    }

    /* Otherwise get the speed */
    status = SJA1105_PortGetSpeed(&hsja1105, hsja1105.config->host_port, &speed);
    if (status != SJA1105_OK) {
        linkstate = ETH_PHY_STATUS_LINK_ERROR;
        return linkstate;
    }

    switch (speed) {
        case SJA1105_SPEED_10M:
            linkstate = ETH_PHY_STATUS_10MBITS_FULLDUPLEX;
            break;

        case SJA1105_SPEED_100M:
            linkstate = ETH_PHY_STATUS_100MBITS_FULLDUPLEX;
            break;

#if defined(ETH_PHY_1000MBITS_SUPPORTED)
        case SJA1105_SPEED_1G:
            linkstate = ETH_PHY_STATUS_1000MBITS_FULLDUPLEX;
            break;
#endif

        default:
            linkstate = ETH_PHY_STATUS_LINK_ERROR;
            break;
    }

    return linkstate;
}

nx_eth_phy_handle_t nx_eth_phy_get_handle(void) {

    nx_eth_phy_handle_t handle = &hsja1105;

    return handle;
}
