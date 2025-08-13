/*
 * phy_callbacks.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "hal.h"

#include "88q211x.h"
#include "phy_callbacks.h"


extern ETH_HandleTypeDef heth;


/* This function performs a clause 45 register read */
phy_status_88q211x_t phy_88q2112_callback_read_reg(phy_handle_88q211x_t *dev, uint8_t dev_addr, uint16_t reg_addr, uint16_t *data) {

    uint32_t tickstart;
    uint32_t tmp_dr;
    uint32_t tmp_ar;

    /* Check for the busy flag */
    if (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) != (uint32_t) RESET) return HAL_ERROR;

    /* ---------------------- Address phase ---------------------- */

    /* Write the register address to the MACMDIODR register */
    tmp_dr = READ_REG(heth.Instance->MACMDIODR);
    MODIFY_REG(tmp_dr, ETH_MACMDIODR_RA, ((uint32_t) reg_addr) << ETH_MACMDIODR_RA_Pos);
    WRITE_REG(heth.Instance->MACMDIODR, tmp_dr);

    /* Write the MACMDIOAR register */
    tmp_ar = READ_REG(heth.Instance->MACMDIOAR);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_PSE); /* 88Q2112 only needs 1 preamble bit */
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_PA, (((uint32_t) dev->config->phy_addr) << ETH_MACMDIOAR_PA_Pos));
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_RDA, (((uint32_t) dev_addr) << ETH_MACMDIOAR_RDA_Pos));
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_MOC, ETH_MACMDIOAR_MOC_WR);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_C45E);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_MB);
    WRITE_REG(ETH->MACMDIOAR, tmp_ar);

    /* Wait for the busy flag */
    tickstart = HAL_GetTick();
    while (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) > 0U) {
        if (((HAL_GetTick() - tickstart) > dev->config->timeout)) {
            return HAL_ERROR;
        }
    }

    /* ---------------------- Data phase ---------------------- */

    /* Change to a read operation */
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_MOC, ETH_MACMDIOAR_MOC_RD);
    WRITE_REG(ETH->MACMDIOAR, tmp_ar);

    /* Wait for the busy flag */
    tickstart = HAL_GetTick();
    while (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) > 0U) {
        if (((HAL_GetTick() - tickstart) > dev->config->timeout)) {
            return HAL_ERROR;
        }
    }

    /* Get MACMIIDR value */
    WRITE_REG(*data, (uint16_t) heth.Instance->MACMDIODR);

    return HAL_OK;
}

/* This function performs a clause 45 register write */
phy_status_88q211x_t phy_88q2112_callback_write_reg(phy_handle_88q211x_t *dev, uint8_t dev_addr, uint16_t reg_addr, uint16_t data) {

    uint32_t tickstart;
    uint32_t tmp_dr;
    uint32_t tmp_ar;

    /* Check for the busy flag */
    if (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) != (uint32_t) RESET) return HAL_ERROR;

    /* ---------------------- Address phase ---------------------- */

    /* Write the register address to the MACMDIODR register */
    tmp_dr = READ_REG(heth.Instance->MACMDIODR);
    MODIFY_REG(tmp_dr, ETH_MACMDIODR_RA, ((uint32_t) reg_addr) << ETH_MACMDIODR_RA_Pos);
    WRITE_REG(heth.Instance->MACMDIODR, tmp_dr);

    /* Write the MACMDIOAR register */
    tmp_ar = READ_REG(heth.Instance->MACMDIOAR);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_PSE); /* 88Q2112 only needs 1 preamble bit */
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_PA, (((uint32_t) dev->config->phy_addr) << ETH_MACMDIOAR_PA_Pos));
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_RDA, (((uint32_t) dev_addr) << ETH_MACMDIOAR_RDA_Pos));
    MODIFY_REG(tmp_ar, ETH_MACMDIOAR_MOC, ETH_MACMDIOAR_MOC_WR);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_C45E);
    SET_BIT(tmp_ar, ETH_MACMDIOAR_MB);
    WRITE_REG(ETH->MACMDIOAR, tmp_ar);

    /* Wait for the busy flag */
    tickstart = HAL_GetTick();
    while (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) > 0U) {
        if (((HAL_GetTick() - tickstart) > dev->config->timeout)) {
            return HAL_ERROR;
        }
    }

    /* ---------------------- Data phase ---------------------- */

    /* Write the data to MACMDIODR */
    MODIFY_REG(tmp_dr, ETH_MACMDIODR_MD, ((uint32_t) data) << ETH_MACMDIODR_MD_Pos);
    WRITE_REG(heth.Instance->MACMDIODR, tmp_dr);

    /* Perform another write */
    WRITE_REG(ETH->MACMDIOAR, tmp_ar);

    /* Wait for the busy flag */
    tickstart = HAL_GetTick();
    while (READ_BIT(heth.Instance->MACMDIOAR, ETH_MACMDIOAR_MB) > 0U) {
        if (((HAL_GetTick() - tickstart) > dev->config->timeout)) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

const phy_callbacks_88q211x_t phy_callbacks_88q2112 = {
    .callback_read_reg    = &phy_88q2112_callback_read_reg,
    .callback_write_reg   = &phy_88q2112_callback_write_reg,
    .callback_get_time_ms = NULL,
    .callback_delay_ms    = NULL,
    .callback_delay_ns    = NULL,
    .callback_take_mutex  = NULL,
    .callback_give_mutex  = NULL,
};
