/*
 * metadata.c
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#include "main.h"
#include "hal.h"

#include "metadata.h"
#include "fram.h"
#include "stm32_uidhash.h"


/* Local metadata variable addresses */
#define DEVICE_ID_ADDR 0x01ffc /* Last 4 bytes of the FRAM */
#define DEVICE_ID_SIZE 4


extern SPI_HandleTypeDef hspi1;


static void META_GetDeviceID(metadata_handle_t *self);


metadata_status_t META_Init(metadata_handle_t *self) {

    metadata_status_t status = META_OK;

    /* Wait for the FRAM to be available */
    while (HAL_GetTick() < FRAM_TPU) __NOP();

    /* Initialise the FRAM */
    if (FRAM_Init(&self->hfram, FRAM_VARIANT_FM25CL64B, &hspi1, FRAM_CS_GPIO_Port, FRAM_CS_Pin, FRAM_HOLD_GPIO_Port, FRAM_HOLD_Pin, FRAM_WP_GPIO_Port, FRAM_WP_Pin) != FRAM_OK) status = META_FRAM_ERROR;
    if (status != META_OK) return status;

    /* Get the device ID */
    META_GetDeviceID(self);

    /* Read the last 32 bits of the FRAM, which should contain the device ID */
    uint32_t buffer;
    _Static_assert(sizeof(buffer) >= DEVICE_ID_SIZE, "Buffer too small");
    if (FRAM_Read(&self->hfram, DEVICE_ID_ADDR, (uint8_t *) (&buffer), DEVICE_ID_SIZE) != FRAM_OK) status = META_FRAM_ERROR;
    if (status != META_OK) return status;

    /* Determine if this is the first boot or not */
    if (buffer != self->device_id) {
        self->first_boot = true;
    } else {
        self->first_boot = false;
    }

    return status;
}


static void META_GetDeviceID(metadata_handle_t *self) {

    /* Get the unique identifier (UID) of this microcontroller and put it into an array of bytes */
    uint8_t  UID[12];
    uint32_t UID_word1 = HAL_GetUIDw0();
    uint32_t UID_word2 = HAL_GetUIDw1();
    uint32_t UID_word3 = HAL_GetUIDw2();
    UID[0]             = (UID_word1 & 0x000000FF) >> 0;
    UID[1]             = (UID_word1 & 0x0000FF00) >> 8;
    UID[2]             = (UID_word1 & 0x00FF0000) >> 16;
    UID[3]             = (UID_word1 & 0xFF000000) >> 24;
    UID[4]             = (UID_word2 & 0x000000FF) >> 0;
    UID[5]             = (UID_word2 & 0x0000FF00) >> 8;
    UID[6]             = (UID_word2 & 0x00FF0000) >> 16;
    UID[7]             = (UID_word2 & 0xFF000000) >> 24;
    UID[8]             = (UID_word3 & 0x000000FF) >> 0;
    UID[9]             = (UID_word3 & 0x0000FF00) >> 8;
    UID[10]            = (UID_word3 & 0x00FF0000) >> 16;
    UID[11]            = (UID_word3 & 0xFF000000) >> 24;

    /* Hash the UID array into a 32 bit number */
    self->device_id = Hash32Len5to12(UID, 12);
}
