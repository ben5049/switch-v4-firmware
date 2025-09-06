/*
 * sja1105_callbacks.c
 *
 *  Created on: Aug 5, 2025
 *      Author: bens1
 */

#include "tx_api.h"
#include "hal.h"

#include "switch_thread.h"
#include "switch_callbacks.h"
#include "sja1105.h"
#include "utils.h"
#include "sja1105q_default_conf.h"


TX_MUTEX            sja1105_mutex_handle;
static UCHAR        switch_byte_pool_buffer[SWITCH_MEM_POOL_SIZE] __ALIGNED(32);
static TX_BYTE_POOL switch_byte_pool;

extern CRC_HandleTypeDef hcrc;


sja1105_status_t switch_byte_pool_init() {

    sja1105_status_t status = SJA1105_OK;

    if (tx_byte_pool_create(&switch_byte_pool, "Switch memory pool", switch_byte_pool_buffer, SWITCH_MEM_POOL_SIZE) != TX_SUCCESS) {
        status = SJA1105_DYNAMIC_MEMORY_ERROR;
    }

    return status;
}

static uint32_t sja1105_get_time_ms(sja1105_handle_t *dev) {

    /* Use kernel time if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        return HAL_GetTick();
    } else {
        return tx_time_get_ms();
    }
}

static void sja1105_delay_ms(sja1105_handle_t *dev, uint32_t ms) {

    /* Use kernel time if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        HAL_Delay(ms);
    } else {
        tx_thread_sleep_ms(ms);
    }
}

static void sja1105_delay_ns(sja1105_handle_t *dev, uint32_t ns) {
    delay_ns(ns);
}

static sja1105_status_t sja1105_take_mutex(sja1105_handle_t *dev, uint32_t timeout) {

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

static sja1105_status_t sja1105_give_mutex(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't give the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_mutex_put(&sja1105_mutex_handle) != TX_SUCCESS) status = SJA1105_MUTEX_ERROR;

    return status;
}

static sja1105_status_t sja1105_allocate(sja1105_handle_t *dev, uint32_t **memory_ptr, uint32_t size) {

    sja1105_status_t status = SJA1105_OK;

    if (tx_byte_allocate(&switch_byte_pool, (void **) memory_ptr, size * sizeof(uint32_t), TX_NO_WAIT) != TX_SUCCESS) {
        status = SJA1105_DYNAMIC_MEMORY_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_free(sja1105_handle_t *dev, uint32_t *memory_ptr) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't free memory if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_byte_release(memory_ptr) != TX_SUCCESS) {
        status = SJA1105_DYNAMIC_MEMORY_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_free_all(sja1105_handle_t *dev) {

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

static sja1105_status_t sja1105_crc_reset(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;

    /* Reset the data register from the previous calculation */
    hcrc.Instance->CR |= CRC_CR_RESET;

    /* Make sure the initial value and polynomial are correct*/
    hcrc.Instance->INIT = 0xffffffff;
    hcrc.Instance->POL  = 0x04c11db7;

    return status;
}

static sja1105_status_t sja1105_crc_accumulate(sja1105_handle_t *dev, const uint32_t *buffer, uint32_t size, uint32_t *result) {

    sja1105_status_t status = SJA1105_OK;

    *result = ~HAL_CRC_Accumulate(&hcrc, (uint32_t *) buffer, size * sizeof(uint32_t));

    return status;
}


const sja1105_callbacks_t sja1105_callbacks = {
    .callback_get_time_ms    = &sja1105_get_time_ms,
    .callback_delay_ms       = &sja1105_delay_ms,
    .callback_delay_ns       = &sja1105_delay_ns,
    .callback_take_mutex     = &sja1105_take_mutex,
    .callback_give_mutex     = &sja1105_give_mutex,
    .callback_allocate       = &sja1105_allocate,
    .callback_free           = &sja1105_free,
    .callback_free_all       = &sja1105_free_all,
    .callback_crc_reset      = &sja1105_crc_reset,
    .callback_crc_accumulate = &sja1105_crc_accumulate,
};
