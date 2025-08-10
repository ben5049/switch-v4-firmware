/*
 * stp_callbacks.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

// TODO: Add parameter checks to all functions (see examples, especially STM32)

#include "stdatomic.h"
#include "hal.h"
#include "main.h"
#include "stp.h"
#include "tx_api.h"

#include "nx_stp.h"
#include "stp_thread.h"
#include "stp_callbacks.h"
#include "utils.h"
#include "sja1105.h"
#include "switch_thread.h"


#define INCR_TX_DESC_INDEX(inx, offset)                   \
    do {                                                  \
        (inx) += (offset);                                \
        if ((inx) >= (uint32_t) ETH_TX_DESC_CNT) {        \
            (inx) = ((inx) - (uint32_t) ETH_TX_DESC_CNT); \
        }                                                 \
    } while (0)


const uint8_t bpdu_dest_address[BPDU_DST_ADDR_SIZE]      = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};
const uint8_t bpdu_dest_address_mask[BPDU_DST_ADDR_SIZE] = {0xff, 0xff, 0xff, 0x00, 0x00, 0xff}; /* Bytes 1 and 2 are masked out because the switch puts the source port tag there */
const uint8_t bpdu_llc[BPDU_LLC_SIZE]                    = {0x42, 0x42, 0x03};

static UCHAR        stp_byte_pool_buffer[STP_MEM_POOL_SIZE] __ALIGNED(32);
static TX_BYTE_POOL stp_byte_pool;


UINT stp_byte_pool_init() {
    return tx_byte_pool_create(&stp_byte_pool, "STP memory pool", stp_byte_pool_buffer, STP_MEM_POOL_SIZE);
}


static void stp_enableBpduTrapping(const struct STP_BRIDGE* bridge, bool enable, unsigned int timestamp) {

    sja1105_status_t status = SJA1105_OK;

    if (enable) {

        /* Send a notification to switch_thread_entry() to tell it that it should start running */
        for (uint_fast8_t attempt = 0; !hsja1105.initialised && attempt < 25; attempt++) {

            /* TODO: Send a notification */

            tx_thread_sleep_ms(200);
        }

        /* If initialisation takes longer than 5 seconds (25 * 200ms) then call the error handler */
        if (!hsja1105.initialised) Error_Handler();

        /* SJA1105 is now initialised, check the BPDU address is trapped by MAC filters */
        bool trapped = false;
        status       = SJA1105_MACAddrTrapTest(&hsja1105, bpdu_dest_address, &trapped);
        if (status != SJA1105_OK) Error_Handler(); /* TODO: Handle this properly */

        /* Trapped by MAC filters: success */
        if (trapped) {
            return;
        }

        /* Not trapped by MAC filters */
        else {
            /* TODO: add a static L2 lookup rule */
        }
    } else {

        /* Send a notification to switch_thread_entry() to tell it that it should stop running */
        for (uint_fast8_t attempt = 0; hsja1105.initialised && attempt < 25; attempt++) {

            /* TODO: Send a notification */

            tx_thread_sleep_ms(200);
        }

        /* If deinitialisation takes longer than 5 seconds (25 * 200ms) then call the error handler */
        if (hsja1105.initialised) Error_Handler();
    }
}


void stp_enableLearning(const struct STP_BRIDGE* bridge, unsigned int portIndex, unsigned int treeIndex, bool enable, unsigned int timestamp) {

    sja1105_status_t status = SJA1105_OK;

    status = SJA1105_PortSetLearning(&hsja1105, portIndex, enable);

    if (status != SJA1105_OK) Error_Handler();
}


void stp_enableForwarding(const struct STP_BRIDGE* bridge, unsigned int portIndex, unsigned int treeIndex, bool enable, unsigned int timestamp) {

    sja1105_status_t status = SJA1105_OK;

    status = SJA1105_PortSetForwarding(&hsja1105, portIndex, enable);

    if (status != SJA1105_OK) Error_Handler();
}


static void* stp_transmitGetBuffer(const struct STP_BRIDGE* bridge, unsigned int portIndex, unsigned int bpduSize, unsigned int timestamp) {

    /* Don't send BPDUs to 10BASE-T1S bus since this should only contain devices, not switches */
    if (portIndex == PORT_LAN8671_PHY) {
        return NULL;
    }

    /* Check the port number is valid */
    if (portIndex >= SJA1105_NUM_PORTS) Error_Handler();

    /* Check the packet will fit */
    if (BPDU_HEADER_SIZE + bpduSize > DEFAULT_PAYLOAD_SIZE) {
        Error_Handler();
    }

    /* Create the management route to send the BPDU from a certain port */
    if (SJA1105_ManagementRouteCreate(&hsja1105, bpdu_dest_address, 1 << portIndex, false, false, &nx_stp) != SJA1105_OK) Error_Handler();

    /* Allocate the packet */
    nx_stp_allocate_packet();
    uint8_t offset = 0;

    /* Write the destination MAC address */
    memcpy(nx_stp.tx_packet_header_ptr + offset, bpdu_dest_address, BPDU_DST_ADDR_SIZE);
    offset += BPDU_DST_ADDR_SIZE;

    /* Write the source MAC address */
    write_mac_addr(nx_stp.tx_packet_header_ptr + offset);
    offset += BPDU_SRC_ADDR_SIZE;

    /* Generate the port address from the bridge address by adding (1 + portIndex) to the last byte */
    bool wrap                                    = (((uint16_t) *(nx_stp.tx_packet_header_ptr + offset - 1)) + 1 + portIndex) > UINT8_MAX;
    *(nx_stp.tx_packet_header_ptr + offset - 1) += (1 + portIndex);
    if (wrap) (*(nx_stp.tx_packet_header_ptr + offset - 2))++;

    /* Write the EtherType/Size, which specifies the size of the payload starting at the LLC field, so BPDU_LLC_SIZE (3) + bpduSize */
    uint16_t etherTypeOrSize                = BPDU_LLC_SIZE + bpduSize;
    *(nx_stp.tx_packet_header_ptr + offset) = (uint8_t) (etherTypeOrSize >> 8);
    offset++;
    *(nx_stp.tx_packet_header_ptr + offset) = (uint8_t) (etherTypeOrSize & 0xff);
    offset++;

    /* Check the size of the header is correct */
    if (offset != (BPDU_HEADER_SIZE - BPDU_LLC_SIZE)) Error_Handler();

    /* 3 bytes for the LLC field, which normally are 0x42, 0x42, 0x03 */
    memcpy(nx_stp.tx_packet_payload_ptr, bpdu_llc, 3);
    nx_stp.tx_packet_ptr->nx_packet_length     += BPDU_LLC_SIZE;
    nx_stp.tx_packet_ptr->nx_packet_append_ptr += BPDU_LLC_SIZE;

    /* Update the size and append pointer */
    nx_stp.tx_packet_ptr->nx_packet_length     += bpduSize;
    nx_stp.tx_packet_ptr->nx_packet_append_ptr += bpduSize;

    /* Return the pointer to after the LLC where the BPDU should be written */
    return nx_stp.tx_packet_payload_ptr + BPDU_LLC_SIZE;
}


static void stp_transmitReleaseBuffer(const struct STP_BRIDGE* bridge, void* bufferReturnedByGetBuffer) {

    nx_status_t status = NX_STATUS_SUCCESS;

    status = nx_stp_send_packet();
    if (status != NX_STATUS_SUCCESS) Error_Handler();
}


static void stp_flushFdb(const struct STP_BRIDGE* bridge, unsigned int portIndex, unsigned int treeIndex, enum STP_FLUSH_FDB_TYPE flushType, unsigned int timestamp) {
    SJA1105_FlushTCAM(&hsja1105); // TODO: check return value
}


static void stp_onTopologyChange(const struct STP_BRIDGE* bridge, unsigned int treeIndex, unsigned int timestamp) {
    /* TODO: Implement */
}

static void stp_onPortRoleChanged(const struct STP_BRIDGE* bridge, unsigned int portIndex, unsigned int treeIndex, enum STP_PORT_ROLE role, unsigned int timestamp) {
    /* TODO: Implement */
}


static void* stp_allocAndZeroMemory(unsigned int size) {

    void* memory_ptr;

    if (tx_byte_allocate(&stp_byte_pool, &memory_ptr, size, TX_NO_WAIT) != TX_SUCCESS) {
        Error_Handler();
    }

    memset(memory_ptr, 0, size);

    return memory_ptr;
}


static void stp_freeMemory(void* p) {

    if (tx_byte_release(p) != TX_SUCCESS) {
        Error_Handler();
    }
}


const STP_CALLBACKS stp_callbacks = {
    .enableBpduTrapping    = &stp_enableBpduTrapping,
    .enableLearning        = &stp_enableLearning,
    .enableForwarding      = &stp_enableForwarding,
    .transmitGetBuffer     = &stp_transmitGetBuffer,
    .transmitReleaseBuffer = &stp_transmitReleaseBuffer,
    .flushFdb              = &stp_flushFdb,
    .debugStrOut           = NULL,
    .onTopologyChange      = &stp_onTopologyChange,
    .onPortRoleChanged     = &stp_onPortRoleChanged,
    .allocAndZeroMemory    = &stp_allocAndZeroMemory,
    .freeMemory            = &stp_freeMemory,
};
