/*
 * sja1105_callbacks.c
 *
 *  Created on: Aug 5, 2025
 *      Author: bens1
 */

#include "tx_api.h"
#include "hal.h"

#include "main.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "sja1105.h"
#include "utils.h"
#include "sja1105q_default_conf.h"


TX_MUTEX            sja1105_mutex_handle;
static UCHAR        switch_byte_pool_buffer[SWITCH_MEM_POOL_SIZE] __ALIGNED(32);
static TX_BYTE_POOL switch_byte_pool;


sja1105_status_t switch_byte_pool_init() {

    sja1105_status_t status = SJA1105_OK;

    if (tx_byte_pool_create(&switch_byte_pool, "Switch memory pool", switch_byte_pool_buffer, SWITCH_MEM_POOL_SIZE) != TX_SUCCESS) {
        status = SJA1105_DYNAMIC_MEMORY_ERROR;
    }

    return status;
}

static void sja1105_write_rst_pin(sja1105_pinstate_t state, void *context) {

    if (state == SJA1105_PIN_RESET) {
        HAL_GPIO_WritePin(SWCH_RST_GPIO_Port, SWCH_RST_Pin, RESET);
    } else {
        HAL_GPIO_WritePin(SWCH_RST_GPIO_Port, SWCH_RST_Pin, SET);
    }
}

static void sja1105_write_cs_pin(sja1105_pinstate_t state, void *context) {

    if (state == SJA1105_PIN_RESET) {
        HAL_GPIO_WritePin(SWCH_CS_GPIO_Port, SWCH_CS_Pin, RESET);
    } else {
        HAL_GPIO_WritePin(SWCH_CS_GPIO_Port, SWCH_CS_Pin, SET);
    }
}

static sja1105_status_t sja1105_spi_transmit(const uint32_t *data, uint16_t size, uint32_t timeout, void *context) {

    sja1105_status_t status = SJA1105_OK;

    if (HAL_SPI_Transmit(&SWCH_SPI, (uint8_t *) data, size, timeout) != HAL_OK) {
        status = SJA1105_SPI_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_spi_receive(uint32_t *data, uint16_t size, uint32_t timeout, void *context) {

    sja1105_status_t status = SJA1105_OK;

    if (HAL_SPI_Receive(&SWCH_SPI, (uint8_t *) data, size, timeout) != HAL_OK) {
        status = SJA1105_SPI_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_spi_transmit_receive(const uint32_t *tx_data, uint32_t *rx_data, uint16_t size, uint32_t timeout, void *context) {

    sja1105_status_t status = SJA1105_OK;

    if (HAL_SPI_TransmitReceive(&SWCH_SPI, (uint8_t *) tx_data, (uint8_t *) rx_data, size, timeout) != HAL_OK) {
        status = SJA1105_SPI_ERROR;
    }

    return status;
}

static uint32_t sja1105_get_time_ms(void *context) {

    /* Use kernel time if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        return HAL_GetTick();
    } else {
        return tx_time_get_ms();
    }
}

static void sja1105_delay_ms(uint32_t ms, void *context) {

    /* Use kernel time if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        HAL_Delay(ms);
    } else {
        tx_thread_sleep_ms(ms);
    }
}

static void sja1105_delay_ns(uint32_t ns, void *context) {
    delay_ns(ns);
}

static sja1105_status_t sja1105_take_mutex(uint32_t timeout, void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't take the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    /* Take the mutex and work out the status */
    switch (tx_mutex_get(&sja1105_mutex_handle, MS_TO_TICKS(timeout))) {
        case TX_SUCCESS:
            status = SJA1105_OK;
            break;
        case TX_NOT_AVAILABLE:
            status = SJA1105_BUSY;
            break;
        default:
            status = SJA1105_MUTEX_ERROR;
            break;
    }

    return status;
}

static sja1105_status_t sja1105_give_mutex(void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't give the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_mutex_put(&sja1105_mutex_handle) != TX_SUCCESS) status = SJA1105_MUTEX_ERROR;

    return status;
}

static sja1105_status_t sja1105_allocate(uint32_t **memory_ptr, uint32_t size, void *context) {

    sja1105_status_t status = SJA1105_OK;

    if (tx_byte_allocate(&switch_byte_pool, (void **) memory_ptr, size * sizeof(uint32_t), TX_NO_WAIT) != TX_SUCCESS) {
        status = SJA1105_DYNAMIC_MEMORY_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_free(uint32_t *memory_ptr, void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't free memory if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_byte_release(memory_ptr) != TX_SUCCESS) {
        status = SJA1105_DYNAMIC_MEMORY_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_free_all(void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't free memory if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_byte_pool_delete(&switch_byte_pool) == TX_SUCCESS) {
        status = switch_byte_pool_init();
    } else {
        status = SJA1105_DYNAMIC_MEMORY_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_crc_reset(void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Reset the data register from the previous calculation */
    SWCH_CRC.Instance->CR |= CRC_CR_RESET;

    /* Make sure the initial value and polynomial are correct*/
    SWCH_CRC.Instance->INIT = 0xffffffff;
    SWCH_CRC.Instance->POL  = 0x04c11db7;

    return status;
}

static sja1105_status_t sja1105_crc_accumulate(const uint32_t *buffer, uint32_t size, uint32_t *result, void *context) {

    sja1105_status_t status = SJA1105_OK;

    *result = ~HAL_CRC_Accumulate(&SWCH_CRC, (uint32_t *) buffer, size * sizeof(uint32_t));

    return status;
}


const sja1105_callbacks_t sja1105_callbacks = {
    .callback_write_rst_pin        = &sja1105_write_rst_pin,
    .callback_write_cs_pin         = &sja1105_write_cs_pin,
    .callback_spi_transmit         = &sja1105_spi_transmit,
    .callback_spi_receive          = &sja1105_spi_receive,
    .callback_spi_transmit_receive = &sja1105_spi_transmit_receive,
    .callback_get_time_ms          = &sja1105_get_time_ms,
    .callback_delay_ms             = &sja1105_delay_ms,
    .callback_delay_ns             = &sja1105_delay_ns,
    .callback_take_mutex           = &sja1105_take_mutex,
    .callback_give_mutex           = &sja1105_give_mutex,
    .callback_allocate             = &sja1105_allocate,
    .callback_free                 = &sja1105_free,
    .callback_free_all             = &sja1105_free_all,
    .callback_crc_reset            = &sja1105_crc_reset,
    .callback_crc_accumulate       = &sja1105_crc_accumulate,
};
