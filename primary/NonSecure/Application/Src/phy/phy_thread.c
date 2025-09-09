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
#include "tx_app.h"


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

    phy_status_t              phy_status  = PHY_OK;
    tx_status_t               tx_status   = TX_SUCCESS;
    phy_cable_state_88q211x_t cable_state = PHY_CABLE_STATE_88Q211X_NOT_STARTED;
    uint32_t                  event_flags = 0;
    int16_t                   temperature = 0;

    UNUSED(cable_state); // TODO: Use

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

    phy_status = PHY_Init(&hphy0, &phy_config_0, &phy_callbacks_88q2112, &hphy0);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_Init(&hphy1, &phy_config_1, &phy_callbacks_88q2112, &hphy1);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_Init(&hphy2, &phy_config_2, &phy_callbacks_88q2112, &hphy2);
    if (phy_status != PHY_OK) Error_Handler();
    //    phy_status = PHY_LAN867X_Init(&hphy3, &phy_config_3, &phy_callbacks_lan8671, &hphy3); TODO: Implement
    //    if (phy_status != PHY_OK) Error_Handler();

    /* Enable interrupts */
    phy_status = PHY_88Q211X_EnableInterrupts(&hphy0);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_EnableInterrupts(&hphy1);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_EnableInterrupts(&hphy2);
    if (phy_status != PHY_OK) Error_Handler();

    /* Enable the temperature sensors */
    phy_status = PHY_88Q211X_EnableTemperatureSensor(&hphy0);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_EnableTemperatureSensor(&hphy1);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_EnableTemperatureSensor(&hphy2);
    if (phy_status != PHY_OK) Error_Handler();

    /* TODO: Perform other configuration */

    /* TODO: Testing config */
    phy_status = PHY_88Q211X_SetRole(&hphy0, PHY_ROLE_SLAVE);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_SetSpeed(&hphy0, PHY_SPEED_100M);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_EnableIEEEPowerDown(&hphy1);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_EnableIEEEPowerDown(&hphy2);
    if (phy_status != PHY_OK) Error_Handler();

    /* TODO: Move VCT to after a timeout if there is no link (to prioritise startup speed), also the PHY probably needs to be reset after due to magic numbers in undocumented registers */
    // /* Start the virtual cable tests (this can take up to 500ms) */
    // phy_status = PHY_88Q211X_StartVCT(&hphy0);
    // if (phy_status != PHY_OK) Error_Handler();
    // phy_status = PHY_88Q211X_StartVCT(&hphy1);
    // if (phy_status != PHY_OK) Error_Handler();
    // phy_status = PHY_88Q211X_StartVCT(&hphy2);
    // if (phy_status != PHY_OK) Error_Handler();

    // /* Get the VCT results */
    // tx_thread_sleep_ms(500);
    // uint32_t                  maximum_peak_distance;
    // phy_status = PHY_88Q211X_GetVCTResults(&hphy0, &cable_state, &maximum_peak_distance);
    // if (phy_status != PHY_OK) Error_Handler();
    // phy_status = PHY_88Q211X_GetVCTResults(&hphy1, &cable_state, &maximum_peak_distance);
    // if (phy_status != PHY_OK) Error_Handler();
    // phy_status = PHY_88Q211X_GetVCTResults(&hphy2, &cable_state, &maximum_peak_distance);
    // if (phy_status != PHY_OK) Error_Handler();

    /* TODO: Enable End to End Transparent Clock and PTP hardware acceleration */

    bool link_up = false;

    /* Check if the link state is up and call the corresponding callback (this is needed because the link can go up before the interrupt is enabled) */
    phy_status = PHY_88Q211X_GetLinkState(&hphy0, &link_up);
    if (phy_status != PHY_OK) Error_Handler();
    if (link_up) {
        phy_status = hphy0.callbacks->callback_link_status_change(link_up, hphy0.callback_context);
        if (phy_status != PHY_OK) Error_Handler();
    }

    /* Setup timing control variables (done in ms) */
    uint32_t current_time   = tx_time_get_ms();
    uint32_t next_wake_time = current_time + PHY_THREAD_INTERVAL;

    while (1) {

        /* Sleep until the next wake time while also monitoring for PHY events */
        current_time = tx_time_get_ms();
        if (current_time < next_wake_time) {

            /* Wait for an interrupt */
            tx_status = tx_event_flags_get(&phy_events_handle, PHY_ALL_EVENTS, TX_OR_CLEAR, &event_flags, MS_TO_TICKS(next_wake_time - current_time));
            if ((tx_status != TX_SUCCESS) && (tx_status != TX_NO_EVENTS)) Error_Handler();

            /* Process any interrupts */
            if (event_flags && (tx_status != TX_NO_EVENTS)) {

                /* Call the interrupt handlers */
                if (event_flags & PHY_PHY0_EVENT) {
                    phy_status = PHY_88Q211X_ProcessInterrupt(&hphy0);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                if (event_flags & PHY_PHY1_EVENT) {
                    phy_status = PHY_88Q211X_ProcessInterrupt(&hphy1);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                if (event_flags & PHY_PHY2_EVENT) {
                    phy_status = PHY_88Q211X_ProcessInterrupt(&hphy2);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                // if (event_flags & PHY_PHY3_EVENT) { TODO:
                //     phy_status = PHY_88Q211X_ProcessInterrupt(&hphy3);
                //     if (phy_status != PHY_OK) Error_Handler();
                // }

                /* Go to the start of the loop and go back to sleep if necessary */
                continue;
            }

            /* Otherwise the sleep time is over and regular PHY processing must be done */
        }

        /* Schedule next wake time */
        next_wake_time += PHY_THREAD_INTERVAL;

        phy_status = PHY_88Q211X_ReadTemperature(&hphy0, &temperature);
        if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_ReadTemperature(&hphy1, &temperature);
        if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_ReadTemperature(&hphy2, &temperature);
        if (phy_status != PHY_OK) Error_Handler();

        phy_status = PHY_88Q211X_GetLinkState(&hphy0, &link_up);
        if (phy_status != PHY_OK) Error_Handler();

        /* TODO: If the current thread holds the phy mutex when it shouldn't report an error */
    }
}
