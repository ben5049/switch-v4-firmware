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


sja1105_handle_t        hsja1105;
static sja1105_config_t sja1105_conf;
static uint32_t         fixed_length_table_buffer[SJA1105_FIXED_BUFFER_SIZE] __ALIGNED(32);

const uint32_t *sja1105_static_conf;
uint32_t        sja1105_static_conf_size;

/* Imported variables */
extern SPI_HandleTypeDef hspi2;


sja1105_status_t switch_init(sja1105_handle_t *dev) {

    sja1105_status_t status;
    sja1105_port_t   port_config;

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
    sja1105_conf.timeout      = SWITCH_TIMEOUT_MS;
    sja1105_conf.mgmt_timeout = SWITCH_MANAGMENT_ROUTE_TIMEOUT_MS;
    sja1105_conf.host_port    = PORT_HOST;
    sja1105_conf.skew_clocks  = true; /* Improves EMI performance */
    sja1105_conf.switch_id    = 0;

    /* Port 0 config */
    port_config.port_num  = PORT_88Q2112_PHY0;
    port_config.interface = SJA1105_INTERFACE_RGMII;
    port_config.mode      = SJA1105_MODE_MAC;
    port_config.speed     = SJA1105_SPEED_DYNAMIC;
    port_config.voltage   = SJA1105_IO_1V8;
    status                = SJA1105_PortConfigure(&sja1105_conf, &port_config);
    if (status != SJA1105_OK) return status;

    /* Port 1 config */
    port_config.port_num  = PORT_88Q2112_PHY1;
    port_config.interface = SJA1105_INTERFACE_RGMII;
    port_config.mode      = SJA1105_MODE_MAC;
    port_config.speed     = SJA1105_SPEED_DYNAMIC;
    port_config.voltage   = SJA1105_IO_1V8;
    status                = SJA1105_PortConfigure(&sja1105_conf, &port_config);
    if (status != SJA1105_OK) return status;

    /* Port 2 config */
    port_config.port_num  = PORT_88Q2112_PHY2;
    port_config.interface = SJA1105_INTERFACE_RGMII;
    port_config.mode      = SJA1105_MODE_MAC;
    port_config.speed     = SJA1105_SPEED_DYNAMIC;
    port_config.voltage   = SJA1105_IO_1V8;
    status                = SJA1105_PortConfigure(&sja1105_conf, &port_config);
    if (status != SJA1105_OK) return status;

    /* Port 3 config */
    port_config.port_num           = PORT_LAN8671_PHY;
    port_config.interface          = SJA1105_INTERFACE_RMII;
    port_config.mode               = SJA1105_MODE_MAC;
    port_config.speed              = SJA1105_SPEED_MBPS_TO_ENUM(PORT3_SPEED_MBPS);
    port_config.voltage            = SJA1105_IO_3V3;
    port_config.output_rmii_refclk = true;
    port_config.rx_error_unused    = false;
    status                         = SJA1105_PortConfigure(&sja1105_conf, &port_config);
    if (status != SJA1105_OK) return status;

    /* Port 4 config */
    port_config.port_num           = PORT_HOST;
    port_config.interface          = SJA1105_INTERFACE_RMII;
    port_config.mode               = SJA1105_MODE_PHY;
    port_config.speed              = SJA1105_SPEED_MBPS_TO_ENUM(PORT4_SPEED_MBPS);
    port_config.voltage            = SJA1105_IO_3V3;
    port_config.output_rmii_refclk = true;
    port_config.rx_error_unused    = true;
    status                         = SJA1105_PortConfigure(&sja1105_conf, &port_config);
    if (status != SJA1105_OK) return status;

    /* Set the static config to the default */
    sja1105_static_conf      = swv4_sja1105_static_config_default;
    sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;

    /* Initialise the switch */
    status = SJA1105_Init(&hsja1105, &sja1105_conf, &sja1105_callbacks, fixed_length_table_buffer, sja1105_static_conf, sja1105_static_conf_size);
    if (status != SJA1105_OK) return status;

    /* TODO: Remove */
    status = SJA1105_ReadAllTables(&hsja1105);
    if (status != SJA1105_OK) return status;

    /* TODO: Remove. Disables port 3 */
    status = SJA1105_PortSetForwarding(&hsja1105, PORT_LAN8671_PHY, false);
    if (status != SJA1105_OK) return status;

    /* Set the speed of the dynamic ports. TODO: This should be after PHY auto-negotiaion */
    status = SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY0, SJA1105_SPEED_MBPS_TO_ENUM(PORT0_SPEED_MBPS));
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY1, SJA1105_SPEED_MBPS_TO_ENUM(PORT1_SPEED_MBPS));
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY2, SJA1105_SPEED_MBPS_TO_ENUM(PORT2_SPEED_MBPS));
    if (status != SJA1105_OK) return status;

    return status;
}
