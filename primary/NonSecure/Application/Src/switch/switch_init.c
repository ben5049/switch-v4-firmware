/*
 * switch_init.c
 *
 *  Created on: Sep 04, 2025
 *      Author: bens1
 */

#include "stdatomic.h"

#include "main.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "sja1105.h"
#include "sja1105q_default_conf.h"
#include "utils.h"


sja1105_handle_t hsja1105;

const uint32_t *sja1105_static_conf;
uint32_t        sja1105_static_conf_size;

/* Imported variables */
extern SPI_HandleTypeDef hspi2;


sja1105_status_t switch_init(sja1105_handle_t *dev) {

    static sja1105_config_t sja1105_conf;
    static uint32_t         fixed_length_table_buffer[SJA1105_FIXED_BUFFER_SIZE] __ALIGNED(32);
    static sja1105_status_t status;

    /* Initialise the ThreadX byte pool */
    status = switch_byte_pool_init();
    if (status != SJA1105_OK) return status;

    /* Set the general switch parameters */
    sja1105_conf.variant      = VARIANT_SJA1105Q;
    sja1105_conf.spi_handle   = &hspi2;
    sja1105_conf.cs_port      = SWCH_CS_GPIO_Port;
    sja1105_conf.cs_pin       = SWCH_CS_Pin;
    sja1105_conf.rst_port     = SWCH_RST_GPIO_Port;
    sja1105_conf.rst_pin      = SWCH_RST_Pin;
    sja1105_conf.timeout      = 100;
    sja1105_conf.mgmt_timeout = 1000;
    sja1105_conf.host_port    = PORT_HOST;
    sja1105_conf.skew_clocks  = true;
    sja1105_conf.switch_id    = 0;

    /* Configure port speeds and interfaces */
    status = SJA1105_PortConfigure(&sja1105_conf, PORT_88Q2112_PHY0, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, false, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8);
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortConfigure(&sja1105_conf, PORT_88Q2112_PHY1, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, false, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8);
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortConfigure(&sja1105_conf, PORT_88Q2112_PHY2, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, false, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8);
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortConfigure(&sja1105_conf, PORT_LAN8671_PHY, SJA1105_INTERFACE_RMII, SJA1105_MODE_MAC, true, SJA1105_SPEED_10M, SJA1105_IO_3V3);
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortConfigure(&sja1105_conf, PORT_HOST, SJA1105_INTERFACE_RMII, SJA1105_MODE_PHY, true, SJA1105_SPEED_100M, SJA1105_IO_3V3);
    if (status != SJA1105_OK) return status;

    /* Set the static config to the default */
    sja1105_static_conf      = swv4_sja1105_static_config_default;
    sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;

    /* Initialise the switch */
    status = SJA1105_Init(&hsja1105, &sja1105_conf, &sja1105_callbacks, fixed_length_table_buffer, sja1105_static_conf, sja1105_static_conf_size);
    if (status != SJA1105_OK) return status;

    /* Set the speed of the dynamic ports. TODO: This should be after PHY auto-negotiaion */
    status = SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY0, SJA1105_SPEED_1G);
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY1, SJA1105_SPEED_1G);
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY2, SJA1105_SPEED_1G);
    if (status != SJA1105_OK) return status;

    return status;
}
