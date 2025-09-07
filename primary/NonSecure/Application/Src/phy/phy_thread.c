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

    phy_status_t              status      = PHY_OK;
    phy_cable_state_88q211x_t cable_state = PHY_CABLE_STATE_88Q211X_NOT_STARTED;

    phy_config_0.variant               = PHY_VARIANT_88Q2112;
    phy_config_0.phy_addr              = 0x00;
    phy_config_0.timeout               = PHY_TIMEOUT_MS;
    phy_config_0.interface             = PHY_INTERFACE_RGMII;
    phy_config_0.tx_clk_internal_delay = true;
    phy_config_0.rx_clk_internal_delay = true;
    phy_config_0.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_1.variant               = PHY_VARIANT_88Q2112;
    phy_config_1.phy_addr              = 0x01;
    phy_config_1.timeout               = PHY_TIMEOUT_MS;
    phy_config_1.interface             = PHY_INTERFACE_RGMII;
    phy_config_1.tx_clk_internal_delay = true;
    phy_config_1.rx_clk_internal_delay = true;
    phy_config_1.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_2.variant               = PHY_VARIANT_88Q2112;
    phy_config_2.phy_addr              = 0x02;
    phy_config_2.timeout               = PHY_TIMEOUT_MS;
    phy_config_2.interface             = PHY_INTERFACE_RGMII;
    phy_config_2.tx_clk_internal_delay = true;
    phy_config_2.rx_clk_internal_delay = true;
    phy_config_2.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_3.variant   = PHY_VARIANT_LAN8671;
    phy_config_3.phy_addr  = 0x08;
    phy_config_3.timeout   = PHY_TIMEOUT_MS;
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
    tx_thread_sleep_ms(10);

    status = PHY_Init(&hphy0, &phy_config_0, &phy_callbacks_88q2112, NULL);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_Init(&hphy1, &phy_config_1, &phy_callbacks_88q2112, NULL);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_Init(&hphy2, &phy_config_2, &phy_callbacks_88q2112, NULL);
    if (status != PHY_OK) Error_Handler();
    //    status = PHY_LAN867X_Init(&hphy3, &phy_config_3, &phy_callbacks_lan8671, NULL); TODO: Implement
    //    if (status != PHY_OK) Error_Handler();

    /* Enable the temperature sensors */
    int16_t temperature;
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy0);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy1);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy2);
    if (status != PHY_OK) Error_Handler();

    /* TODO: Perform other configuration */

    /* TODO: Testing config */
    status = PHY_88Q211X_SetRole(&hphy0, PHY_ROLE_SLAVE);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_SetSpeed(&hphy0, PHY_SPEED_100M);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableIEEEPowerDown(&hphy1);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableIEEEPowerDown(&hphy2);
    if (status != PHY_OK) Error_Handler();

    /* TODO: Move VCT to after a timeout if there is no link (to prioritise startup speed), also the PHY probably needs to be reset after due to magic numbers in undocumented registers */
    // /* Start the virtual cable tests (this can take up to 500ms) */
    // status = PHY_88Q211X_StartVCT(&hphy0);
    // if (status != PHY_OK) Error_Handler();
    // status = PHY_88Q211X_StartVCT(&hphy1);
    // if (status != PHY_OK) Error_Handler();
    // status = PHY_88Q211X_StartVCT(&hphy2);
    // if (status != PHY_OK) Error_Handler();

    // /* Get the VCT results */
    // tx_thread_sleep_ms(500);
    // uint32_t                  maximum_peak_distance;
    // status = PHY_88Q211X_GetVCTResults(&hphy0, &cable_state, &maximum_peak_distance);
    // if (status != PHY_OK) Error_Handler();
    // status = PHY_88Q211X_GetVCTResults(&hphy1, &cable_state, &maximum_peak_distance);
    // if (status != PHY_OK) Error_Handler();
    // status = PHY_88Q211X_GetVCTResults(&hphy2, &cable_state, &maximum_peak_distance);
    // if (status != PHY_OK) Error_Handler();

    /* TODO: Enable End to End Transparent Clock and PTP hardware acceleration */

    bool link_up = false;

    while (1) {
        tx_thread_sleep_ms(1000);

        status = PHY_88Q211X_ReadTemperature(&hphy0, &temperature);
        if (status != PHY_OK) Error_Handler();
        status = PHY_88Q211X_ReadTemperature(&hphy1, &temperature);
        if (status != PHY_OK) Error_Handler();
        status = PHY_88Q211X_ReadTemperature(&hphy2, &temperature);
        if (status != PHY_OK) Error_Handler();

        status = PHY_88Q211X_GetLinkState(&hphy0, &link_up);
        if (status != PHY_OK) Error_Handler();

        /* TODO: If the current thread holds the phy mutex when it shouldn't report an error */
    }
}
