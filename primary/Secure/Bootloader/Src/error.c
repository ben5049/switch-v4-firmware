/*
 * error.c
 *
 *  Created on: Aug 20, 2025
 *      Author: bens1
 */

#include "error.h"
#include "logging.h"
#include "metadata.h"
#include "memory_tools.h"


extern RAMCFG_HandleTypeDef hramcfg_BKPRAM;


void error_handler(error_t major_error_code, uint8_t minor_error_code) {

    __disable_irq();

    /* Turn on the error LED */
    HAL_GPIO_WritePin(STAT_GPIO_Port, STAT_Pin, SET);

    /* NO_CHECK required to prevent recursion */
    LOG_ERROR_NO_CHECK("Error handler :(\n");

    /* Store the most recent logs into the FRAM */
    log_dump_to_fram(&hlog, &hmeta);

    /* Update and store the metadata and counters */
    bool previous_crash    = hmeta.metadata.crashed;
    hmeta.metadata.crashed = true;
    hmeta.counters.crashes++;
    META_dump_metadata(&hmeta);
    META_dump_counters(&hmeta);
    HAL_RAMCFG_Erase(&hramcfg_BKPRAM);

    /* Swap banks to the other bank to try and find a working firmware image. Do not flip-flop between broken images */
    if (!previous_crash) swap_banks();

    while (1) {
    }
}
