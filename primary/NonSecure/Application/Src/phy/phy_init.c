/*
 * phy_init.c
 *
 *  Created on: 11 Sept 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "main.h"

#include "phy_thread.h"
#include "phy_callbacks.h"
#include "config.h"
#include "utils.h"


phy_handle_88q211x_t hphy0;
phy_handle_88q211x_t hphy1;
phy_handle_88q211x_t hphy2;
phy_handle_lan867x_t hphy3;

void *phy_handles[NUM_PHYS] = {&hphy0, &hphy1, &hphy2, &hphy3};

static phy_config_88q211x_t phy_config_0;
static phy_config_88q211x_t phy_config_1;
static phy_config_88q211x_t phy_config_2;
static phy_config_lan867x_t phy_config_3;

// static void *phy_configs[NUM_PHYS] = {&phy_config_0, &phy_config_1, &phy_config_2, &phy_config_3};


phy_status_t phys_init() {

    phy_status_t status = PHY_OK;

    phy_config_0.variant               = PHY_VARIANT_88Q2112;
    phy_config_0.phy_addr              = 0x00;
    phy_config_0.timeout               = PHY_TIMEOUT_MS;
    phy_config_0.interface             = PHY_INTERFACE_RGMII;
    phy_config_0.default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT0_SPEED_MBPS);
    phy_config_0.default_role          = PHY_ROLE_MASTER;
    phy_config_0.tx_clk_internal_delay = true;
    phy_config_0.rx_clk_internal_delay = true;
    phy_config_0.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_1.variant               = PHY_VARIANT_88Q2112;
    phy_config_1.phy_addr              = 0x01;
    phy_config_1.timeout               = PHY_TIMEOUT_MS;
    phy_config_1.interface             = PHY_INTERFACE_RGMII;
    phy_config_1.default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT1_SPEED_MBPS);
    phy_config_1.default_role          = PHY_ROLE_MASTER;
    phy_config_1.tx_clk_internal_delay = true;
    phy_config_1.rx_clk_internal_delay = true;
    phy_config_1.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_2.variant               = PHY_VARIANT_88Q2112;
    phy_config_2.phy_addr              = 0x02;
    phy_config_2.timeout               = PHY_TIMEOUT_MS;
    phy_config_2.interface             = PHY_INTERFACE_RGMII;
    phy_config_2.default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT2_SPEED_MBPS);
    phy_config_2.default_role          = PHY_ROLE_MASTER;
    phy_config_2.tx_clk_internal_delay = true;
    phy_config_2.rx_clk_internal_delay = true;
    phy_config_2.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_3.variant         = PHY_VARIANT_LAN8671;
    phy_config_3.phy_addr        = 0x08;
    phy_config_3.timeout         = PHY_TIMEOUT_MS;
    phy_config_3.interface       = PHY_INTERFACE_RMII;
    phy_config_3.plca_enabled    = true;
    phy_config_3.plca_id         = PHY_PLCA_COORDINATOR_ID;
    phy_config_3.plca_node_count = PHY_PLCA_DEFAULT_NODE_COUNT; /* Maximum of 16 devices on the bus by default, all devices must have the same node count setting. */

    /* Set pins to a known state */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, SET);
    HAL_GPIO_WritePin(PHY_WAKE_GPIO_Port, PHY_WAKE_Pin, SET);
    HAL_GPIO_WritePin(PHY_CLK_EN_GPIO_Port, PHY_CLK_EN_Pin, SET);

    /* Hardware reset all PHYs (TODO: Reduce times) */
    tx_thread_sleep_ms(10);
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, RESET);
    tx_thread_sleep_ms(10); /* 10ms required by 88Q2112 */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, SET);
    tx_thread_sleep_ms(10);

    /* Initialise all PHYs */
    status = PHY_Init(&hphy0, &phy_config_0, &phy_callbacks_88q2112, &hphy0);
    if (status != PHY_OK) Error_Handler();
    status = PHY_Init(&hphy1, &phy_config_1, &phy_callbacks_88q2112, &hphy1);
    if (status != PHY_OK) Error_Handler();
    status = PHY_Init(&hphy2, &phy_config_2, &phy_callbacks_88q2112, &hphy2);
    if (status != PHY_OK) Error_Handler();
    status = PHY_Init(&hphy3, &phy_config_3, &phy_callbacks_lan8671, &hphy3);
    if (status != PHY_OK) Error_Handler();

    /* Enable interrupts TODO: Fix */
    status = PHY_88Q211X_EnableInterrupts(&hphy0);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableInterrupts(&hphy1);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableInterrupts(&hphy2);
    if (status != PHY_OK) Error_Handler();
    /* TODO: Enable PHY3 interrupts */

    /* Enable the temperature sensors */
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy0);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy1);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy2);
    if (status != PHY_OK) Error_Handler();

    /* TODO: Perform other configuration */

    /* TODO: Enable End to End Transparent Clock and PTP hardware acceleration */

    return status;
}
