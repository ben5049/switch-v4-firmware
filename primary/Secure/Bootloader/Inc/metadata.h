/*
 * metadata.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_METADATA_H_
#define INC_METADATA_H_


#include "fram.h"


typedef struct
{
    fram_handle_t hfram;
    bool          first_boot;
    uint32_t      device_id;
} metadata_handle_t;

typedef enum {
    META_OK      = HAL_OK,
    META_ERROR   = HAL_ERROR,
    META_BUSY    = HAL_BUSY,
    META_TIMEOUT = HAL_TIMEOUT,
    META_NOT_IMPLEMENTED,
    META_PARAMETER_ERROR,
    META_FRAM_ERROR,
} metadata_status_t;


metadata_status_t META_Init(metadata_handle_t *meta);


#endif /* INC_METADATA_H_ */
