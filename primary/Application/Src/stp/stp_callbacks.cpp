/*
 * stp_callbacks.cpp
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#include "assert.h"
#include "hal.h"

#include "stp_callbacks.h"
#include "utils.h"
#include "sja1105.h"


#define BPDU_BPDU_MAX_BUFFER_SIZE    (128)
#define BPDU_DST_ADDR_SIZE           (6)
#define BPDU_SRC_ADDR_SIZE           (6)
#define BPDU_SIZE_OR_ETHERTYPE_SIZE  (2)
#define BPDU_LLC_SIZE                (3)
#define BPDU_HEADER_SIZE             (BPDU_DST_ADDR_SIZE + BPDU_SRC_ADDR_SIZE + BPDU_SIZE_OR_ETHERTYPE_SIZE + BPDU_LLC_SIZE)

#define BPDU_MAX_BUFFER_SIZE         (128)

#define INCR_TX_DESC_INDEX(inx, offset) do {\
                                             (inx) += (offset);\
                                             if ((inx) >= (uint32_t)ETH_TX_DESC_CNT){\
                                             (inx) = ((inx) - (uint32_t)ETH_TX_DESC_CNT);}\
                                           } while (0)


/* Imported variables */
extern ETH_HandleTypeDef heth;


static uint8_t tx_bpdu_buffer[BPDU_MAX_BUFFER_SIZE] __attribute__((aligned(32)));
static uint8_t tx_bpdu_size;

static const uint8_t bpdu_dest_address[BPDU_DST_ADDR_SIZE] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};
static const uint8_t bpdu_llc[BPDU_LLC_SIZE] = {0x42, 0x42, 0x03};

static ETH_BufferTypeDef TxBuffer;
static ETH_TxPacketConfigTypeDef TxPacketCfg;


void bpdu_packet_init(){
    memset(&TxPacketCfg, 0, sizeof(ETH_TxPacketConfigTypeDef)); /* Set the ethernet packet parameters to default */
}


static void* stp_transmitGetBuffer(const struct STP_BRIDGE* bridge, unsigned int portIndex, unsigned int bpduSize, unsigned int timestamp){

    uint8_t offset = 0;
    tx_bpdu_size = bpduSize;

    assert(portIndex < SJA1105_NUM_PORTS);
    assert(BPDU_HEADER_SIZE + bpduSize <= BPDU_MAX_BUFFER_SIZE);

    /* Write the destination MAC address */
    memcpy(&tx_bpdu_buffer[offset], bpdu_dest_address, BPDU_DST_ADDR_SIZE);
    offset += BPDU_DST_ADDR_SIZE;

    /* Write the source MAC address */
    write_mac_addr(&tx_bpdu_buffer[offset]);
    offset += BPDU_SRC_ADDR_SIZE;

    /* Generate the port address from the bridge address by adding (1 + portIndex) to the last byte */
    bool wrap = (((uint16_t) tx_bpdu_buffer[offset - 1]) + 1 + portIndex) > UINT8_MAX;
    tx_bpdu_buffer[offset - 1] += (1 + portIndex);
    if (wrap) tx_bpdu_buffer[offset - 2]++;

	/* Write the EtherType/Size, which specifies the size of the payload starting at the LLC field, so BPDU_LLC_SIZE (3) + bpduSize */
	uint16_t etherTypeOrSize = BPDU_LLC_SIZE + bpduSize;
	tx_bpdu_buffer[offset++] = uint8_t (etherTypeOrSize >> 8);
	tx_bpdu_buffer[offset++] = uint8_t (etherTypeOrSize & 0xFF);

	/* 3 bytes for the LLC field, which normally are 0x42, 0x42, 0x03 */
	memcpy(&tx_bpdu_buffer[offset], bpdu_llc, 3);
    offset += BPDU_LLC_SIZE;

    assert(offset == BPDU_HEADER_SIZE);
	return &tx_bpdu_buffer[BPDU_HEADER_SIZE];
}

static void stp_transmitReleaseBuffer(const struct STP_BRIDGE* bridge, void* bufferReturnedByGetBuffer){

    uint8_t packet_length = BPDU_HEADER_SIZE + tx_bpdu_size;   /* Header + payload */

    /* Set the buffer parameters */
    TxBuffer.buffer = tx_bpdu_buffer;
    TxBuffer.len    = packet_length;
    TxBuffer.next   = NULL;

    /* There is no data cache for internal memories in the STM32H573, but if there was then
     * the cache for tx_bpdu_buffer shouLd be cleaned so the DMA has access to it.
     */

    /* Set the packet parameters */
    TxPacketCfg.TxBuffer     = &TxBuffer;
    TxPacketCfg.Length       = packet_length;
    TxPacketCfg.pData        = (uint32_t *) tx_bpdu_buffer;
    TxPacketCfg.ChecksumCtrl = ETH_CHECKSUM_DISABLE;
    TxPacketCfg.CRCPadCtrl   = ETH_DMATXNDESCRF_CPC_CRCPAD_INSERT;

    /* Transmit the packet (non-blocking) */
    assert(HAL_ETH_Transmit_IT(&heth, &TxPacketCfg) == HAL_OK);
}


/* Called by HAL_ETH_TxCpltCallback() to free the packet TODO: Notify this thread */
bool stp_ReleaseTxPacket(ETH_HandleTypeDef *heth){
    ETH_TxDescListTypeDef *dmatxdesclist = &heth->TxDescList;
    uint32_t numOfBuf =  dmatxdesclist->BuffersInUse;
    uint32_t idx =       dmatxdesclist->releaseIndex;
    uint8_t pktTxStatus = 1U;
    uint8_t pktInUse;

    /* Loop through buffers in use.  */
    while ((numOfBuf != 0U) && (pktTxStatus != 0U)){
        pktInUse = 1U;
        numOfBuf--;

        /* If no packet, just examine the next packet.  */
        if (dmatxdesclist->PacketAddress[idx] == NULL){
            /* No packet in use, skip to next.  */
            INCR_TX_DESC_INDEX(idx, 1U);
            pktInUse = 0U;
        }

        if (pktInUse != 0U){

            /* Determine if the packet has been transmitted.  */
            if ((heth->Init.TxDesc[idx].DESC3 & ETH_DMATXNDESCRF_OWN) == 0U){

                /* Determine if the packet is the bpdu */
                if (dmatxdesclist->PacketAddress[idx] == (uint32_t *) tx_bpdu_buffer){

                    /* Clear the entry in the in-use array.  */
                    dmatxdesclist->PacketAddress[idx] = NULL;
                    
                    /* Update the transmit release index and number of buffers in use.  */
                    INCR_TX_DESC_INDEX(idx, 1U);
                    dmatxdesclist->BuffersInUse = numOfBuf;
                    dmatxdesclist->releaseIndex = idx;
                    
                    return true;
                }
            }
            else {
                /* Get out of the loop!  */
                pktTxStatus = 0U;
            }
        }
    }
    return false;
}


const STP_CALLBACKS stp_callbacks = {
    .enableBpduTrapping    = nullptr,
    .enableLearning        = nullptr,
    .enableForwarding      = nullptr,
    .transmitGetBuffer     = &stp_transmitGetBuffer,
    .transmitReleaseBuffer = &stp_transmitReleaseBuffer,
    .flushFdb              = nullptr,
    .debugStrOut           = nullptr,
    .onTopologyChange      = nullptr,
    .onPortRoleChanged     = nullptr,
    .allocAndZeroMemory    = nullptr,
    .freeMemory            = nullptr,
};
