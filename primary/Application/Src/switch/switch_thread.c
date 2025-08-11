/*
 * switch.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "stdatomic.h"

#include "main.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "sja1105.h"
#include "sja1105q_default_conf.h"
#include "utils.h"

#define CHECK(func)                                        \
    do {                                                   \
        status = (func);                                   \
        sja1105_check_status_msg(&hsja1105, status, true); \
    } while (0)

uint8_t   switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_ptr;

sja1105_handle_t     hsja1105;
atomic_uint_fast32_t sja1105_error_counter = 0;

static const uint32_t *sja1105_static_conf;
static uint32_t        sja1105_static_conf_size;

/* Imported variables */
extern SPI_HandleTypeDef hspi2;

/* Private function prototypes */
static void sja1105_check_status_msg(sja1105_handle_t *dev, sja1105_status_t to_check, bool recurse);

/* Attemt to handle errors resulting from SJA1105 user function calls
 * NOTE: When the system error handler is called, it is assumed that if it returns (as opposed to restarting the chip) then the error has been fixed.
 */
static void sja1105_check_status_msg(sja1105_handle_t *dev, sja1105_status_t to_check, bool recurse) {

    /* Return immediately if everything is fine */
    if (to_check == SJA1105_OK) return;

    sja1105_status_t status       = SJA1105_OK;
    bool             error_solved = false;

    /* to_check is an error, increment the counter and check what to do */
    sja1105_error_counter++;
    switch (to_check) {

        /* TODO: Log an error, but continue */
        case SJA1105_ALREADY_CONFIGURED_ERROR:
            error_solved = true;
            break;

        /* Parameter errors cannot be corrected on the fly, only at compile time. */
        case SJA1105_PARAMETER_ERROR:
            break;

        /* If there is a CRC error then rollback to the default config */
        case SJA1105_CRC_ERROR:
            sja1105_static_conf      = swv4_sja1105_static_config_default;
            sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;
            status                   = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
            error_solved             = true;
            break;

        /* If there is an error with the static configuration load the default config */
        case SJA1105_STATIC_CONF_ERROR:
            sja1105_static_conf      = swv4_sja1105_static_config_default;
            sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;
            status                   = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
            error_solved             = dev->initialised;
            break;

        /* If there is a RAM parity error the switch must be immediately reset */
        case SJA1105_RAM_PARITY_ERROR:
            status       = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
            error_solved = dev->initialised;
            break;

        /* Error has not been corrected */
        default:
            break;
    }

    /* A NEW ERROR has occured during the handling of the previous error... */
    if (status != SJA1105_OK) {
        sja1105_error_counter++;

        /* ...and the new error is the SAME as the previous error... */
        if (status == to_check) {

            /* ...but the previous error was SOLVED: the new error is also solved */
            if (error_solved)
                ;

            /* ...and the previous error was NOT SOLVED: the problem is deeper, call the system error handler */
            else
                Error_Handler();
        }

        /* ...and the new error is DIFFERENT from the previous error... */
        else {

            /* ...but the previous error was SOLVED: check the new error (recursively) */
            if (error_solved) {
                if (recurse) {
                    sja1105_error_counter--; /* Don't double count the new error */
                    sja1105_check_status_msg(dev, status, false);
                } else
                    Error_Handler(); /* An error occurred while checking an error that occurred while checking an error. Yikes */
            }

            /* ...and the previous error was NOT SOLVED: the problem is deeper, call the system error handler */
            else
                Error_Handler();
        }
        error_solved = true;
    }

    /* Unsolved error */
    if (!error_solved) Error_Handler();

    /* All errors have now been handled, check the status registers just to be safe */
    status = SJA1105_CheckStatusRegisters(dev);
    if (recurse) sja1105_check_status_msg(dev, status, false);

    /* An error occurring means the mutex could have been taken but not released. Release it now */
    while (dev->callbacks->callback_give_mutex(dev) == SJA1105_OK);
}


void switch_thread_entry(uint32_t initial_input) {

    static sja1105_config_t sja1105_conf;
    static uint32_t         fixed_length_table_buffer[SJA1105_FIXED_BUFFER_SIZE] __ALIGNED(32);
    static sja1105_status_t status;
    static int16_t          temp_x10;

    /* Initialise the ThreadX byte pool */
    switch_byte_pool_init();

    /* Set the general switch parameters */
    sja1105_conf.variant      = VARIANT_SJA1105Q;
    sja1105_conf.spi_handle   = &hspi2;
    sja1105_conf.cs_port      = SWCH_CS_GPIO_Port;
    sja1105_conf.cs_pin       = SWCH_CS_Pin;
    sja1105_conf.rst_port     = SWCH_RST_GPIO_Port;
    sja1105_conf.rst_pin      = SWCH_RST_Pin;
    sja1105_conf.timeout      = 100;
    sja1105_conf.mgmt_timeout = 1000;
    sja1105_conf.host_port    = PORT_HOST;
    sja1105_conf.skew_clocks  = true;
    sja1105_conf.switch_id    = 0;

    /* Configure port speeds and interfaces */
    CHECK(SJA1105_PortConfigure(&sja1105_conf, PORT_88Q2112_PHY0, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, false, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8));
    CHECK(SJA1105_PortConfigure(&sja1105_conf, PORT_88Q2112_PHY1, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, false, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8));
    CHECK(SJA1105_PortConfigure(&sja1105_conf, PORT_88Q2112_PHY2, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, false, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8));
    CHECK(SJA1105_PortConfigure(&sja1105_conf, PORT_LAN8671_PHY, SJA1105_INTERFACE_RMII, SJA1105_MODE_MAC, true, SJA1105_SPEED_10M, SJA1105_IO_3V3));
    CHECK(SJA1105_PortConfigure(&sja1105_conf, PORT_HOST, SJA1105_INTERFACE_RMII, SJA1105_MODE_PHY, true, SJA1105_SPEED_100M, SJA1105_IO_3V3));

    /* Set the static config to the default */
    sja1105_static_conf      = swv4_sja1105_static_config_default;
    sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;

    /* Initialise the switch */
    CHECK(SJA1105_Init(&hsja1105, &sja1105_conf, &sja1105_callbacks, fixed_length_table_buffer, sja1105_static_conf, sja1105_static_conf_size));

    /* Set the speed of the dynamic ports. TODO: This should be after PHY auto-negotiaion */
    CHECK(SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY0, SJA1105_SPEED_1G));
    CHECK(SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY1, SJA1105_SPEED_1G));
    CHECK(SJA1105_PortSetSpeed(&hsja1105, PORT_88Q2112_PHY2, SJA1105_SPEED_1G));

    while (1) {

        /* Perform regular maintenance */
        CHECK(SJA1105_CheckStatusRegisters(&hsja1105));
        CHECK(SJA1105_ManagementRouteFree(&hsja1105, false));
        /* TODO: Add byte pool maintenance */

        /* TODO: Ocassionally check no important MAC addresses have been learned by accident (PTP, STP, etc)*/

        /* Read the temperature */
        CHECK(SJA1105_ReadTemperatureX10(&hsja1105, &temp_x10));

        tx_thread_sleep_ms(500);
    }
}
