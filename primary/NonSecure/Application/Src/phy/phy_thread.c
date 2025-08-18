/*
 * phy_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "hal.h"
#include "main.h"

#include "88q211x.h"
#include "lan867x.h"
#include "phy_thread.h"
#include "phy_callbacks.h"
#include "utils.h"
#include "config.h"


phy_handle_88q211x_t hphy0;
phy_handle_88q211x_t hphy1;
phy_handle_88q211x_t hphy2;
phy_handle_lan867x_t hphy3;

void *phy_handles[NUM_PHYS] = {&hphy0, &hphy1, &hphy2, &hphy3};

static phy_config_88q211x_t phy_config_0;
static phy_config_88q211x_t phy_config_1;
static phy_config_88q211x_t phy_config_2;
static phy_config_lan867x_t phy_config_3;

void *phy_configs[NUM_PHYS] = {&phy_config_0, &phy_config_1, &phy_config_2, &phy_config_3};


uint8_t   phy_thread_stack[PHY_THREAD_STACK_SIZE];
TX_THREAD phy_thread_handle;


void phy_thread_entry(uint32_t initial_input) {

    phy_config_0.variant               = PHY_VARIANT_88Q2112;
    phy_config_0.phy_addr              = 0x00;
    phy_config_0.timeout               = PHY_TIMEOUT;
    phy_config_0.interface             = PHY_INTERFACE_RGMII;
    phy_config_0.tx_clk_internal_delay = true;
    phy_config_0.rx_clk_internal_delay = true;
    phy_config_0.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_1.variant               = PHY_VARIANT_88Q2112;
    phy_config_1.phy_addr              = 0x01;
    phy_config_1.timeout               = PHY_TIMEOUT;
    phy_config_1.interface             = PHY_INTERFACE_RGMII;
    phy_config_1.tx_clk_internal_delay = true;
    phy_config_1.rx_clk_internal_delay = true;
    phy_config_1.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_2.variant               = PHY_VARIANT_88Q2112;
    phy_config_2.phy_addr              = 0x02;
    phy_config_2.timeout               = PHY_TIMEOUT;
    phy_config_2.interface             = PHY_INTERFACE_RGMII;
    phy_config_2.tx_clk_internal_delay = true;
    phy_config_2.rx_clk_internal_delay = true;
    phy_config_2.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_3.variant   = PHY_VARIANT_LAN8671;
    phy_config_3.phy_addr  = 0x08;
    phy_config_3.timeout   = PHY_TIMEOUT;
    phy_config_3.interface = PHY_INTERFACE_RMII;

    /* Set pins to a known state */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, SET);
    HAL_GPIO_WritePin(PHY_WAKE_GPIO_Port, PHY_WAKE_Pin, SET);
    HAL_GPIO_WritePin(PHY_CLK_EN_GPIO_Port, PHY_CLK_EN_Pin, SET);

    /* Hardware reset all PHYs */
    tx_thread_sleep_ms(10);
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, RESET);
    tx_thread_sleep_ms(10); /* 10ms required by 88Q2112 */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, SET);
    tx_thread_sleep_ms(1);

    PHY_88Q211X_Init(&hphy0, &phy_config_0, &phy_callbacks_88q2112, NULL);
    PHY_88Q211X_Init(&hphy1, &phy_config_1, &phy_callbacks_88q2112, NULL);
    PHY_88Q211X_Init(&hphy2, &phy_config_2, &phy_callbacks_88q2112, NULL);
    PHY_LAN867X_Init(&hphy3, &phy_config_3, &phy_callbacks_lan8671, NULL);

    /* Start the virtual cable tests (this can take up to 500ms) */
    PHY_88Q211X_StartVCT(&hphy0);
    PHY_88Q211X_StartVCT(&hphy1);
    PHY_88Q211X_StartVCT(&hphy2);

    /* Enable the temperature sensors */
    int16_t temperature;
    PHY_88Q211X_EnableTemperatureSensor(&hphy0);
    PHY_88Q211X_EnableTemperatureSensor(&hphy1);
    PHY_88Q211X_EnableTemperatureSensor(&hphy2);

    /* TODO: Perform other configuration */

    /* Get the VCT results */
    tx_thread_sleep_ms(500);
    phy_cable_state_88q211x_t cable_state;
    uint32_t                  maximum_peak_distance;
    PHY_88Q211X_GetVCTResults(&hphy0, &cable_state, &maximum_peak_distance);
    PHY_88Q211X_GetVCTResults(&hphy1, &cable_state, &maximum_peak_distance);
    PHY_88Q211X_GetVCTResults(&hphy2, &cable_state, &maximum_peak_distance);

    /* TODO: Enable End to End Transparent Clock and PTP hardware acceleration */

    while (1) {
        tx_thread_sleep_ms(1000);

        PHY_88Q211X_ReadTemperature(&hphy0, &temperature);
        PHY_88Q211X_ReadTemperature(&hphy1, &temperature);
        PHY_88Q211X_ReadTemperature(&hphy2, &temperature);
    }
}