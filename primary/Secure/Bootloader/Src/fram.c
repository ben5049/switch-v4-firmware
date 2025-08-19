/*
 * fram.c
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#include "stdint.h"

#include "fram.h"


static fram_status_t FRAM_WriteEnable(fram_handle_t *dev);
static fram_status_t FRAM_WriteDisable(fram_handle_t *dev);
static fram_status_t FRAM_ReadSR(fram_handle_t *dev, uint8_t *sr);
static fram_status_t FRAM_WriteSR(fram_handle_t *dev, uint8_t sr);


fram_status_t FRAM_Init(
    fram_handle_t     *dev,
    fram_variant_t     variant,
    SPI_HandleTypeDef *hspi,
    GPIO_TypeDef      *cs_port,
    uint16_t           cs_pin,
    GPIO_TypeDef      *hold_port,
    uint16_t           hold_pin,
    GPIO_TypeDef      *wp_port,
    uint16_t           wp_pin) {

    fram_status_t status = FRAM_OK;

    dev->configured    = false;
    dev->variant       = variant;
    dev->block_protect = FRAM_PROTECT_ALL; /* Default is protect all blocks */
    dev->hspi          = hspi;
    dev->cs_port       = cs_port;
    dev->cs_pin        = cs_pin;
    dev->hold_port     = hold_port;
    dev->hold_pin      = hold_pin;
    dev->wp_port       = wp_port;
    dev->wp_pin        = wp_pin;

    /* Set pins to default state */
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
    HAL_GPIO_WritePin(dev->hold_port, dev->hold_pin, SET);
    HAL_GPIO_WritePin(dev->wp_port, dev->wp_pin, RESET);

    /* Enable WP pin functionality */
    status = FRAM_EnableWP(dev);
    if (status != FRAM_OK) return status;

    /* Set the default protection status (all) */
    status = FRAM_SetBlockProtection(dev, dev->block_protect);
    if (status != FRAM_OK) return status;

    /* Clear the write enable latch to be safe */
    status = FRAM_WriteDisable(dev);
    if (status != FRAM_OK) return status;

    /* Update the device struct */
    dev->configured = true;

    return status;
}


static fram_status_t FRAM_WriteEnable(fram_handle_t *dev) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_WREN;

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, RESET);

    status = HAL_SPI_Transmit(dev->hspi, &opcode, 1, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);

    return status;
}


static fram_status_t FRAM_WriteDisable(fram_handle_t *dev) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_WRDI;

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, RESET);

    status = HAL_SPI_Transmit(dev->hspi, &opcode, 1, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);

    return status;
}


static fram_status_t FRAM_ReadSR(fram_handle_t *dev, uint8_t *sr) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_RDSR;

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, RESET);

    status = HAL_SPI_Transmit(dev->hspi, &opcode, 1, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    status = HAL_SPI_Receive(dev->hspi, sr, 1, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);

    return status;
}


static fram_status_t FRAM_WriteSR(fram_handle_t *dev, uint8_t sr) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_WRSR;

    status = FRAM_WriteEnable(dev);
    if (status != FRAM_OK) return status;

    HAL_GPIO_WritePin(dev->wp_port, dev->wp_pin, SET);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, RESET);

    status = HAL_SPI_Transmit(dev->hspi, &opcode, 1, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        HAL_GPIO_WritePin(dev->wp_port, dev->wp_pin, RESET);
        return status;
    }

    status = HAL_SPI_Receive(dev->hspi, &sr, 1, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        HAL_GPIO_WritePin(dev->wp_port, dev->wp_pin, RESET);
        return status;
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
    HAL_GPIO_WritePin(dev->wp_port, dev->wp_pin, RESET);

    return status;
}


fram_status_t FRAM_EnableWP(fram_handle_t *dev) {

    fram_status_t status = FRAM_OK;
    uint8_t       reg_data;

    /* Read the status register */
    status = FRAM_ReadSR(dev, &reg_data);
    if (status != FRAM_OK) return status;

    /* Check and update the status register */
    if (!(reg_data & FRAM_SR_WPEN)) {
        reg_data |= FRAM_SR_WPEN;

        /* Write to the status register */
        status = FRAM_WriteSR(dev, reg_data);
        if (status != FRAM_OK) return status;
    }

    return status;
}


fram_status_t FRAM_DisableWP(fram_handle_t *dev) {

    fram_status_t status = FRAM_OK;
    uint8_t       reg_data;

    /* Read the status register */
    status = FRAM_ReadSR(dev, &reg_data);
    if (status != FRAM_OK) return status;

    /* Check and update the status register */
    if (reg_data & FRAM_SR_WPEN) {
        reg_data &= ~FRAM_SR_WPEN;

        /* Write to the status register */
        status = FRAM_WriteSR(dev, reg_data);
        if (status != FRAM_OK) return status;
    }

    return status;
}


fram_status_t FRAM_SetBlockProtection(fram_handle_t *dev, fram_block_protect_t protect) {

    fram_status_t status = FRAM_OK;
    uint8_t       reg_data;

    /* Read the status register */
    status = FRAM_ReadSR(dev, &reg_data);
    if (status != FRAM_OK) return status;

    /* Determine the new status register value */
    switch (protect) {
        case FRAM_PROTECT_NONE:
            reg_data &= ~FRAM_SR_BP0;
            reg_data &= ~FRAM_SR_BP1;
            break;

        case FRAM_PROTECT_UPPER_QUARTER:
            reg_data |= FRAM_SR_BP0;
            reg_data &= ~FRAM_SR_BP1;
            break;

        case FRAM_PROTECT_UPPER_HALF:
            reg_data &= ~FRAM_SR_BP0;
            reg_data |= FRAM_SR_BP1;
            break;

        case FRAM_PROTECT_ALL:
            reg_data |= FRAM_SR_BP0;
            reg_data |= FRAM_SR_BP1;
            break;

        default:
            status = FRAM_PARAMETER_ERROR;
            break;
    }
    if (status != FRAM_OK) return status;

    /* Write the status register */
    status = FRAM_WriteSR(dev, reg_data);
    if (status != FRAM_OK) return status;

    /* Update the device struct */
    dev->block_protect = protect;

    return status;
}


fram_status_t FRAM_Write(fram_handle_t *dev, uint16_t addr, uint8_t *data, uint16_t size) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_WRITE;

    status = FRAM_WriteEnable(dev);
    if (status != FRAM_OK) return status;

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, RESET);

    status = HAL_SPI_Transmit(dev->hspi, &opcode, 1, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    addr   &= 0x1fff; /* 13-bit address */
    status  = HAL_SPI_Transmit(dev->hspi, (uint8_t *) (&addr), 2, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    status = HAL_SPI_Transmit(dev->hspi, data, size, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);

    return status;
}


fram_status_t FRAM_Read(fram_handle_t *dev, uint16_t addr, uint8_t *data, uint16_t size) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_READ;

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, RESET);

    status = HAL_SPI_Transmit(dev->hspi, &opcode, 1, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    addr   &= 0x1fff; /* 13-bit address */
    status  = HAL_SPI_Transmit(dev->hspi, (uint8_t *) (&addr), 2, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    status = HAL_SPI_Receive(dev->hspi, data, size, FRAM_DEFAULT_TIMEOUT);
    if (status != FRAM_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);
        return status;
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, SET);

    return status;
}
