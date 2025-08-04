/*
 * utils.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "utils.h"
#include "config.h"

/* This function should be called in MX_ETH_Init */
void write_mac_addr(uint8_t *buf) {
    buf[0] = MAC_ADDR_OCTET1;
    buf[1] = MAC_ADDR_OCTET2;
    buf[2] = MAC_ADDR_OCTET3;
    buf[3] = MAC_ADDR_OCTET4;
    buf[4] = MAC_ADDR_OCTET5;
    buf[5] = MAC_ADDR_OCTET6;
}

uint32_t tx_thread_sleep_ms(uint32_t ms) {
    return tx_thread_sleep((uint32_t) MS_TO_TICKS((uint64_t) ms)); /* Cast to 64-bit uint to prevent premature overflow */
}

uint32_t tx_time_get_ms() {
    return (uint32_t) TICKS_TO_MS((uint64_t) tx_time_get()); /* Cast to 64-bit uint to prevent premature overflow */
}
