/*
 * phy_callbacks.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "hal.h"
#include "tx_api.h"
#include "main.h"

#include "88q211x.h"
#include "lan867x.h"
#include "phy_callbacks.h"
#include "phy_thread.h"
#include "stp_thread.h"
#include "utils.h"
#include "tx_app.h"
#include "phy_mdio.h"


TX_MUTEX             phy_mutex_handle;
TX_EVENT_FLAGS_GROUP phy_events_handle;


static phy_status_t phy_88q2112_callback_read_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    /* 88Q2112 only needs 1 preamble bit */
    /* Set the clock frequency to 9.62MHz (PHY supports up to 12.5MHz) */
    return phy_read_reg_c45(phy_addr, mmd_addr, reg_addr, data, timeout, true, ETH_MACMDIOAR_CR_DIV26);
}

static phy_status_t phy_lan8671_callback_read_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    phy_status_t status = PHY_OK;

    /* Clause 22 normal access */
    if (mmd_addr == 0) {

        /* Set the clock frequency to 2.45MHz (PHY supports up to 4MHz) */
        status = phy_read_reg_c22(phy_addr, reg_addr, data, timeout, ETH_MACMDIOAR_CR_DIV102);
    }

    /* Clause 45 indirect access */
    else if ((mmd_addr == 1) || (mmd_addr == 3) || (mmd_addr == 31)) {

        uint16_t reg_data;

        /* Step 1:
         * Write the MMD Access Control register with the MMD Function (FNCTN) field set to 00b and the
         * Device Address (DEVAD) field with the MDIO Management Device (MMD) address. */
        reg_data  = PHY_LAN867X_MMDCTRL_FNCTN_ADDR << PHY_LAN867X_MMDCTRL_FNCTN_SHIFT;
        reg_data |= mmd_addr << PHY_LAN867X_MMDCTRL_DEVAD_SHIFT;
        status    = phy_write_reg_c22(phy_addr, PHY_LAN867X_MMDCTRL, reg_data, timeout, ETH_MACMDIOAR_CR_DIV102);
        if (status != PHY_OK) return status;

        /* Step 2:
         * Write the address of the desired register to be read into the MMD Access Address/Data register. */
        status = phy_write_reg_c22(phy_addr, PHY_LAN867X_MMDAD, reg_addr, timeout, ETH_MACMDIOAR_CR_DIV102);
        if (status != PHY_OK) return status;

        /* Step 3:
         * Write the MMD Access Control register with the MMD Function field set to 01b, 10b, or 11b. */
        reg_data  = PHY_LAN867X_MMDCTRL_FNCTN_DATA << PHY_LAN867X_MMDCTRL_FNCTN_SHIFT;
        reg_data |= mmd_addr << PHY_LAN867X_MMDCTRL_DEVAD_SHIFT;
        status    = phy_write_reg_c22(phy_addr, PHY_LAN867X_MMDCTRL, reg_data, timeout, ETH_MACMDIOAR_CR_DIV102);
        if (status != PHY_OK) return status;

        /* Step 4:
         * Read the contents of the MMD’s selected register from the MMD Access Address/Data register. */
        status = phy_read_reg_c22(phy_addr, PHY_LAN867X_MMDAD, data, timeout, ETH_MACMDIOAR_CR_DIV102);
        if (status != PHY_OK) return status;
    }

    /* Invalid MMD */
    else {
        status = PHY_ADDR_ERROR;
    }

    return status;
}


static phy_status_t phy_88q2112_callback_write_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    /* 88Q2112 only needs 1 preamble bit */
    /* Set the clock frequency to 9.62MHz (PHY supports up to 12.5MHz) */
    return phy_write_reg_c45(phy_addr, mmd_addr, reg_addr, data, timeout, true, ETH_MACMDIOAR_CR_DIV26);
}

static phy_status_t phy_lan8671_callback_write_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    phy_status_t status = PHY_OK;

    /* Clause 22 normal access */
    if (mmd_addr == 0) {

        /* Set the clock frequency to 2.45MHz (PHY supports up to 4MHz) */
        status = phy_write_reg_c22(phy_addr, reg_addr, data, timeout, ETH_MACMDIOAR_CR_DIV102);
    }

    /* Clause 45 indirect access */
    else if ((mmd_addr == 1) || (mmd_addr == 3) || (mmd_addr == 31)) {

        uint16_t reg_data;

        /* Step 1:
         * Write the MMD Access Control register with the MMD Function (FNCTN) field set to 00b and the
         * Device Address (DEVAD) field with the MDIO Management Device (MMD) address. */
        reg_data  = PHY_LAN867X_MMDCTRL_FNCTN_ADDR << PHY_LAN867X_MMDCTRL_FNCTN_SHIFT;
        reg_data |= mmd_addr << PHY_LAN867X_MMDCTRL_DEVAD_SHIFT;
        status    = phy_write_reg_c22(phy_addr, PHY_LAN867X_MMDCTRL, reg_data, timeout, ETH_MACMDIOAR_CR_DIV102);
        if (status != PHY_OK) return status;

        /* Step 2:
         * Write the address of the desired register to be written into the MMD Access Address/Data register. */
        status = phy_write_reg_c22(phy_addr, PHY_LAN867X_MMDAD, reg_addr, timeout, ETH_MACMDIOAR_CR_DIV102);
        if (status != PHY_OK) return status;

        /* Step 3:
         * Write the MMD Access Control register with the MMD Function field set to 01b, 10b, or 11b. */
        reg_data  = PHY_LAN867X_MMDCTRL_FNCTN_DATA << PHY_LAN867X_MMDCTRL_FNCTN_SHIFT;
        reg_data |= mmd_addr << PHY_LAN867X_MMDCTRL_DEVAD_SHIFT;
        status    = phy_write_reg_c22(phy_addr, PHY_LAN867X_MMDCTRL, reg_data, timeout, ETH_MACMDIOAR_CR_DIV102);
        if (status != PHY_OK) return status;

        /* Step 4:
         * Write the contents of the MMD’s selected register into the MMD Access Address/Data register. */
        status = phy_write_reg_c22(phy_addr, PHY_LAN867X_MMDAD, data, timeout, ETH_MACMDIOAR_CR_DIV102);
        if (status != PHY_OK) return status;
    }

    /* Invalid MMD */
    else {
        status = PHY_ADDR_ERROR;
    }

    return status;
}


static uint32_t phy_callback_get_time_ms(void *context) {

    /* Use kernel time only if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        return HAL_GetTick();
    } else {
        return tx_time_get_ms();
    }
}


static void phy_callback_delay_ms(uint32_t ms, void *context) {

    /* Use kernel time only if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        HAL_Delay(ms);
    } else {
        tx_thread_sleep_ms(ms);
    }
}


static void phy_callback_delay_ns(uint32_t ns, void *context) {
    delay_ns(ns);
}


static phy_status_t phy_callback_take_mutex(uint32_t timeout, void *context) {

    phy_status_t status = PHY_OK;

    /* Don't take the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    /* Take the mutex and work out the status */
    switch (tx_mutex_get(&phy_mutex_handle, MS_TO_TICKS(timeout))) {
        case TX_SUCCESS:
            status = PHY_OK;
            break;
        case TX_NOT_AVAILABLE:
            status = PHY_BUSY;
            break;
        default:
            status = PHY_MUTEX_ERROR;
            break;
    }

    return status;
}


static phy_status_t phy_callback_give_mutex(void *context) {

    phy_status_t status = PHY_OK;

    /* Don't give the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_mutex_put(&phy_mutex_handle) != TX_SUCCESS) status = PHY_MUTEX_ERROR;

    return status;
}

static phy_status_t phy_callback_event(phy_event_t event, void *context) {

    phy_status_t status    = PHY_OK;

    switch (event) {
        case PHY_EVENT_LINK_UP:
        case PHY_EVENT_LINK_DOWN:

/* Notify the STP thread */
#if ENABLE_STP_THREAD == true

            /* Don't send notification if the kernel hasn't started */
            if (tx_thread_identify() == TX_NULL) return PHY_OK;

            tx_status_t  tx_status = TX_SUCCESS;

            if (context == &hphy0) {
                tx_status = tx_event_flags_set(&stp_events_handle, STP_PORT0_LINK_STATE_CHANGE_EVENT, TX_OR);
            } else if (context == &hphy1) {
                tx_status = tx_event_flags_set(&stp_events_handle, STP_PORT1_LINK_STATE_CHANGE_EVENT, TX_OR);
            } else if (context == &hphy2) {
                tx_status = tx_event_flags_set(&stp_events_handle, STP_PORT2_LINK_STATE_CHANGE_EVENT, TX_OR);
            } else if (context == &hphy3) {
                tx_status = tx_event_flags_set(&stp_events_handle, STP_PORT3_LINK_STATE_CHANGE_EVENT, TX_OR);
            }
            /* Return the tx_status */
            if (tx_status != TX_SUCCESS) {
                status = PHY_ERROR;
            } else {
                status = PHY_OK;
            }
#endif

            break;

        default:
            break;
    }

    return status;
}

const phy_callbacks_t phy_callbacks_88q2112 = {
    .callback_read_reg    = &phy_88q2112_callback_read_reg,
    .callback_write_reg   = &phy_88q2112_callback_write_reg,
    .callback_get_time_ms = &phy_callback_get_time_ms,
    .callback_delay_ms    = &phy_callback_delay_ms,
    .callback_delay_ns    = &phy_callback_delay_ns,
    .callback_take_mutex  = &phy_callback_take_mutex,
    .callback_give_mutex  = &phy_callback_give_mutex,
    .callback_event       = &phy_callback_event,
    .callback_write_log   = &log_write,
};

const phy_callbacks_t phy_callbacks_lan8671 = {
    .callback_read_reg    = &phy_lan8671_callback_read_reg,
    .callback_write_reg   = &phy_lan8671_callback_write_reg,
    .callback_get_time_ms = &phy_callback_get_time_ms,
    .callback_delay_ms    = &phy_callback_delay_ms,
    .callback_delay_ns    = &phy_callback_delay_ns,
    .callback_take_mutex  = &phy_callback_take_mutex,
    .callback_give_mutex  = &phy_callback_give_mutex,
    .callback_write_log   = &log_write,
};

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {

    tx_status_t status       = TX_SUCCESS;
    uint32_t    flags_to_set = 0;

    /* Return if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return;

    switch (GPIO_Pin) {

        case (PHY0_INT_Pin):
            flags_to_set |= PHY_PHY0_EVENT;
            break;

        case (PHY1_INT_Pin):
            flags_to_set |= PHY_PHY1_EVENT;
            break;

        case (PHY2_INT_Pin):
            flags_to_set |= PHY_PHY2_EVENT;
            break;

        case (PHY3_INT_Pin):
            flags_to_set |= PHY_PHY3_EVENT;
            break;

        default:
            Error_Handler();
            break;
    }

    /* Set the flag */
    status = tx_event_flags_set(&phy_events_handle, flags_to_set, TX_OR);
    if (status != TX_SUCCESS) Error_Handler();
}
