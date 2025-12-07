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
#include "phy_callbacks.h"
#include "phy_thread.h"
#include "stp_thread.h"
#include "utils.h"
#include "tx_app.h"


TX_MUTEX             phy_mutex_handle;
TX_EVENT_FLAGS_GROUP phy_events_handle;

extern ETH_HandleTypeDef heth;


/* This function performs a clause 45 register read */
static phy_status_t phy_callback_read_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, bool preamble_supression, uint32_t clk_div) {

    uint32_t tickstart;
    uint32_t tmp_dr = 0;
    uint32_t tmp_ar = 0;

    /* Check for the busy flag */
    if (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) != (uint32_t) RESET) return PHY_BUSY;

    /* Write the register address to the MACMDIODR register and set the data register to a known value (if 0xdead is read back later then it is likely that no data has been received) */
    tmp_dr = READ_REG(heth.Instance->MACMDIODR);
    MODIFY_REG(tmp_dr, ETH_MACMDIODR_RA, ((uint32_t) reg_addr) << ETH_MACMDIODR_RA_Pos);
    MODIFY_REG(tmp_dr, ETH_MACMDIODR_MD, ((uint32_t) 0xdead) << ETH_MACMDIODR_MD_Pos);
    WRITE_REG(heth.Instance->MACMDIODR, tmp_dr);

    /* Write the MACMDIOAR register */
    tmp_ar = READ_REG(heth.Instance->MACMDIOAR);
    if (preamble_supression) {
        SET_BIT(tmp_ar, ETH_MACMDIOAR_PSE);
    } else {
        CLEAR_BIT(tmp_ar, ETH_MACMDIOAR_PSE);
    }
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_PA, (((uint32_t) phy_addr) << ETH_MACMDIOAR_PA_Pos));
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_RDA, (((uint32_t) mmd_addr) << ETH_MACMDIOAR_RDA_Pos));
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_CR, clk_div);
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_MOC, ETH_MACMDIOAR_MOC_RD);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_C45E);
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_NTC, 1 << ETH_MACMDIOAR_NTC_Pos);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_MB);
    WRITE_REG(ETH->MACMDIOAR, tmp_ar);

    /* Wait for the busy flag */
    tickstart = HAL_GetTick();
    while (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) > 0U) {
        if (((HAL_GetTick() - tickstart) > timeout)) {
            return PHY_TIMEOUT;
        }
    }

    /* Get MACMIIDR value */
    WRITE_REG(*data, (uint16_t) heth.Instance->MACMDIODR);

    return PHY_OK;
}

static phy_status_t phy_88q2112_callback_read_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    /* 88Q2112 only needs 1 preamble bit */
    /* Set the clock frequency to 9.62MHz (PHY supports up to 12.5MHz) */
    return phy_callback_read_reg(phy_addr, mmd_addr, reg_addr, data, timeout, true, ETH_MACMDIOAR_CR_DIV26);
}

static phy_status_t phy_lan8671_callback_read_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 2.45MHz (PHY supports up to 4MHz) */
    return phy_callback_read_reg(phy_addr, mmd_addr, reg_addr, data, timeout, false, ETH_MACMDIOAR_CR_DIV102);
}


/* This function performs a clause 45 register write */
static phy_status_t phy_callback_write_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, bool preamble_supression, uint32_t clk_div) {

    uint32_t tickstart;
    uint32_t tmp_dr = 0;
    uint32_t tmp_ar = 0;

    /* Check for the busy flag */
    if (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) != (uint32_t) RESET) return PHY_BUSY;

    /* Write the register address to MACMDIODR */
    tmp_dr = READ_REG(heth.Instance->MACMDIODR);
    MODIFY_REG(tmp_dr, ETH_MACMDIODR_RA, ((uint32_t) reg_addr) << ETH_MACMDIODR_RA_Pos);
    WRITE_REG(heth.Instance->MACMDIODR, tmp_dr);

    /* Write the data to MACMDIODR */
    MODIFY_REG(tmp_dr, ETH_MACMDIODR_MD, ((uint32_t) data) << ETH_MACMDIODR_MD_Pos);
    WRITE_REG(heth.Instance->MACMDIODR, tmp_dr);

    /* Write the MACMDIOAR register */
    tmp_ar = READ_REG(heth.Instance->MACMDIOAR);
    if (preamble_supression) {
        SET_BIT(tmp_ar, ETH_MACMDIOAR_PSE);
    } else {
        CLEAR_BIT(tmp_ar, ETH_MACMDIOAR_PSE);
    }
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_PA, (((uint32_t) phy_addr) << ETH_MACMDIOAR_PA_Pos));
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_RDA, (((uint32_t) mmd_addr) << ETH_MACMDIOAR_RDA_Pos));
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_CR, clk_div);
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_MOC, ETH_MACMDIOAR_MOC_WR);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_C45E);
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_NTC, 1 << ETH_MACMDIOAR_NTC_Pos);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_MB);
    WRITE_REG(ETH->MACMDIOAR, tmp_ar);

    /* Wait for the busy flag */
    tickstart = HAL_GetTick();
    while (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) > 0U) {
        if (((HAL_GetTick() - tickstart) > timeout)) {
            return PHY_TIMEOUT;
        }
    }

    return PHY_OK;
}

static phy_status_t phy_88q2112_callback_write_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    /* 88Q2112 only needs 1 preamble bit */
    /* Set the clock frequency to 9.62MHz (PHY supports up to 12.5MHz) */
    return phy_callback_write_reg(phy_addr, mmd_addr, reg_addr, data, timeout, true, ETH_MACMDIOAR_CR_DIV26);
}

static phy_status_t phy_lan8671_callback_write_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 2.45MHz (PHY supports up to 4MHz) */
    return phy_callback_write_reg(phy_addr, mmd_addr, reg_addr, data, timeout, false, ETH_MACMDIOAR_CR_DIV102);
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

static phy_status_t phy_callback_link_status_change(bool linkup, void *context) {

    tx_status_t status = TX_SUCCESS;

    /* Don't send notification if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return PHY_OK;

    /* Notify the STP thread */
#if ENABLE_STP_THREAD == true
    if (context == &hphy0) {
        status = tx_event_flags_set(&stp_events_handle, STP_PORT0_LINK_STATE_CHANGE_EVENT, TX_OR);
    } else if (context == &hphy1) {
        status = tx_event_flags_set(&stp_events_handle, STP_PORT1_LINK_STATE_CHANGE_EVENT, TX_OR);
    } else if (context == &hphy2) {
        status = tx_event_flags_set(&stp_events_handle, STP_PORT2_LINK_STATE_CHANGE_EVENT, TX_OR);
    } else if (context == &hphy3) {
        status = tx_event_flags_set(&stp_events_handle, STP_PORT3_LINK_STATE_CHANGE_EVENT, TX_OR);
    }
#endif

    /* Return the status */
    if (status != TX_SUCCESS) {
        return PHY_ERROR;
    } else {
        return PHY_OK;
    }
}


const phy_callbacks_t phy_callbacks_88q2112 = {
    .callback_read_reg           = &phy_88q2112_callback_read_reg,
    .callback_write_reg          = &phy_88q2112_callback_write_reg,
    .callback_get_time_ms        = &phy_callback_get_time_ms,
    .callback_delay_ms           = &phy_callback_delay_ms,
    .callback_delay_ns           = &phy_callback_delay_ns,
    .callback_take_mutex         = &phy_callback_take_mutex,
    .callback_give_mutex         = &phy_callback_give_mutex,
    .callback_link_status_change = &phy_callback_link_status_change};


const phy_callbacks_t phy_callbacks_lan8671 = {
    .callback_read_reg    = &phy_lan8671_callback_read_reg,
    .callback_write_reg   = &phy_lan8671_callback_write_reg,
    .callback_get_time_ms = &phy_callback_get_time_ms,
    .callback_delay_ms    = &phy_callback_delay_ms,
    .callback_delay_ns    = &phy_callback_delay_ns,
    .callback_take_mutex  = &phy_callback_take_mutex,
    .callback_give_mutex  = &phy_callback_give_mutex,
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
