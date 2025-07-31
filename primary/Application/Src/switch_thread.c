/*
 * switch.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "switch_thread.h"
#include "sja1105.h"
#include "sja1105q_default_conf.h"
#include "utils.h"

#define CHECK(func) {status = (func); sja1105_check_status_msg(&hsja1105, status, true);}

uint8_t   switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_ptr;
TX_MUTEX  sja1105_mutex_ptr;
uint32_t  sja1105_error_counter = 0;

static const uint32_t *sja1105_static_conf;
static       uint32_t  sja1105_static_conf_size;

/* Imported variables */
extern SPI_HandleTypeDef hspi2;

/* Private function prototype */
static void sja1105_delay_ms(uint32_t ms);
static void sja1105_delay_ns(uint32_t ns);
static SJA1105_StatusTypeDef sja1105_take_mutex(uint32_t timeout);
static SJA1105_StatusTypeDef sja1105_give_mutex(void);
static void sja1105_check_status_msg(SJA1105_HandleTypeDef *dev, SJA1105_StatusTypeDef to_check, bool recurse);

/* Enums */
enum Port_Enum{
    PORT_88Q2112_PHY0 = 0x0,
    PORT_88Q2112_PHY1 = 0x1,
    PORT_88Q2112_PHY2 = 0x2,
    PORT_LAN8671_PHY  = 0x3,
    PORT_HOST         = 0x4,
};


static void sja1105_delay_ms(uint32_t ms){
    tx_thread_sleep_ms(ms);
}

static void sja1105_delay_ns(uint32_t ns){

    /* CPU runs at 250MHz so one instruction is 4ns.
    * The loop contains a NOP, ADDS, CMP and branch instruction per cycle.
    * This means the loop delay is 4 * 4ns = 16ns.
    * This is true for O3 but will take longer for O0.
    */
    for (uint32_t t = 0; t < ns; t += 16){
        __NOP();
    }
}

static SJA1105_StatusTypeDef sja1105_take_mutex(uint32_t timeout){

    SJA1105_StatusTypeDef status = SJA1105_OK;

    switch (tx_mutex_get(&sja1105_mutex_ptr, MS_TO_TICKS(timeout))){
    case TX_SUCCESS:
        status = SJA1105_OK;
        break;
    case TX_NOT_AVAILABLE:
        status = SJA1105_BUSY;
        break;
    default:
        status = SJA1105_ERROR;
        break;
    }

    return status;
}

static SJA1105_StatusTypeDef sja1105_give_mutex(void){

    SJA1105_StatusTypeDef status = SJA1105_OK;

    if (tx_mutex_put(&sja1105_mutex_ptr) != TX_SUCCESS) status = SJA1105_ERROR;

    return status;
}

static const SJA1105_CallbacksTypeDef sja1105_callbacks = {
    .callback_delay_ms   = &sja1105_delay_ms,
    .callback_delay_ns   = &sja1105_delay_ns,
    .callback_take_mutex = &sja1105_take_mutex,
    .callback_give_mutex = &sja1105_give_mutex
};

/* Attemt to handle errors resulting from SJA1105 user function calls
 * NOTE: When the system error handler is called, it is assumed that if it returns (as opposed to restarting the chip) then the error has been fixed.
 */
static void sja1105_check_status_msg(SJA1105_HandleTypeDef *dev, SJA1105_StatusTypeDef to_check, bool recurse){

    /* Return immediately if everything is fine */
    if (to_check == SJA1105_OK) return;

    SJA1105_StatusTypeDef status = SJA1105_OK;
    bool error_solved = false;

    /* to_check is an error, increment the counter and check what to do */
    sja1105_error_counter++;
    switch (to_check){

        /* TODO: Log an error, but continue */
        case SJA1105_ALREADY_CONFIGURED_ERROR:
            error_solved = true;
            break;

        /* Parameter errors cannot be corrected on the fly, only at compile time. */
        case SJA1105_PARAMETER_ERROR:
            Error_Handler();
            error_solved = false;
            break;

        /* If there is a CRC error then rollback to the default config */
        case SJA1105_CRC_ERROR:
            sja1105_static_conf      = swv4_sja1105_static_config_default;
            sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;
            status = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
            error_solved = true;
            break;

        /* If there is an error with the static configuration load the default config. If that is loaded already then call the system error handler */
        case SJA1105_STATIC_CONF_ERROR:
            if ((dev->static_conf_crc32 == swv4_sja1105_static_config_default[SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE-1]) ||
                (dev->static_conf_crc32 == 0)){
                    Error_Handler();   
                }
            else {
                sja1105_static_conf      = swv4_sja1105_static_config_default;
                sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;
                status = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
            }
            error_solved = dev->initialised;
            break;

        /* If there is a RAM parity error the switch must be immediately reset */
        case SJA1105_RAM_PARITY_ERROR:
            status = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
            error_solved = dev->initialised;
            break;

        /* Error has not been corrected, call the system error handler */
        default:
            Error_Handler();
            error_solved = true;
            break;
    }

    /* A NEW ERROR has occured during the handling of the previous error... */
    if (status != SJA1105_OK){
        sja1105_error_counter++;

        /* ...and the new error is the SAME as the previous error... */
        if (status == to_check){
            
            /* ...but the previous error was SOLVED: the new error is also solved */
            if (error_solved);

            /* ...and the previous error was NOT SOLVED: the problem is deeper, call the system error handler */
            else Error_Handler();
        }

        /* ...and the new error is DIFFERENT from the previous error... */
        else{
            
            /* ...but the previous error was SOLVED: check the new error (recursively) */
            if (error_solved){
                if (recurse){
                    sja1105_error_counter--;  /* Don't double count the new error */
                    sja1105_check_status_msg(dev, status, false);
                }
                else Error_Handler();  /* An error occured while checking an error that occured while checking an error. Yikes */
            }
            
            /* ...and the previous error was NOT SOLVED: the problem is deeper, call the system error handler */
            else Error_Handler();
        }
        error_solved = true;
    }

    /* Unsolved error slipped through */
    if (!error_solved) Error_Handler();

    /* All errors have now been handled, check the status registers just to be safe */
    status = SJA1105_CheckStatusRegisters(dev);
    if (recurse) sja1105_check_status_msg(dev, status, false);

    /* An error occuring means the mutex could have been taken but not released. Release it now */
    while (dev->callbacks->callback_give_mutex() == SJA1105_OK);
}


void switch_thread_entry(uint32_t initial_input){

    static SJA1105_HandleTypeDef hsja1105;
    static SJA1105_ConfigTypeDef sja1105_conf;
    static SJA1105_PortTypeDef   sja1105_ports[SJA1105_NUM_PORTS];

    static SJA1105_StatusTypeDef status;
    static int16_t               temp_x10;

    /* Set the general switch parameters */
    sja1105_conf.variant    = VARIANT_SJA1105Q;
    sja1105_conf.spi_handle = &hspi2;
    sja1105_conf.cs_port    = SWCH_CS_GPIO_Port;
    sja1105_conf.cs_pin     = SWCH_CS_Pin;
    sja1105_conf.rst_port   = SWCH_RST_GPIO_Port;
    sja1105_conf.rst_pin    = SWCH_RST_Pin;
    sja1105_conf.timeout    = 100;

    /* Configure port speeds and interfaces */
    CHECK(SJA1105_ConfigurePort(sja1105_ports, PORT_88Q2112_PHY0, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8));
    CHECK(SJA1105_ConfigurePort(sja1105_ports, PORT_88Q2112_PHY1, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8));
    CHECK(SJA1105_ConfigurePort(sja1105_ports, PORT_88Q2112_PHY2, SJA1105_INTERFACE_RGMII, SJA1105_MODE_MAC, SJA1105_SPEED_DYNAMIC, SJA1105_IO_1V8));
    CHECK(SJA1105_ConfigurePort(sja1105_ports, PORT_LAN8671_PHY,  SJA1105_INTERFACE_RMII,  SJA1105_MODE_MAC, SJA1105_SPEED_10M,     SJA1105_IO_3V3));
    CHECK(SJA1105_ConfigurePort(sja1105_ports, PORT_HOST,         SJA1105_INTERFACE_RMII,  SJA1105_MODE_PHY, SJA1105_SPEED_100M,    SJA1105_IO_3V3));

    /* Set the static config to the default */
    sja1105_static_conf      = swv4_sja1105_static_config_default;
    sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;

    /* Initialise the switch */
    CHECK(SJA1105_Init(&hsja1105, &sja1105_conf, sja1105_ports, &sja1105_callbacks, sja1105_static_conf, sja1105_static_conf_size));

    /* Set the speed of the dynamic ports. TODO: This should be after PHY auto-negotiaion */
    CHECK(SJA1105_UpdatePortSpeed(&hsja1105, PORT_88Q2112_PHY0, SJA1105_SPEED_1G));
    CHECK(SJA1105_UpdatePortSpeed(&hsja1105, PORT_88Q2112_PHY1, SJA1105_SPEED_1G));
    CHECK(SJA1105_UpdatePortSpeed(&hsja1105, PORT_88Q2112_PHY2, SJA1105_SPEED_1G));

    while (1){
        tx_thread_sleep_ms(200);

        /* Perform a regular maintenance check */
        CHECK(SJA1105_CheckStatusRegisters(&hsja1105));

        /* Read the temperature */
        CHECK(SJA1105_ReadTemperatureX10(&hsja1105, &temp_x10));
    }
}
