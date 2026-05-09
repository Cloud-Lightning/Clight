/**
* @file
* Ethernet Interface Skeleton
*
*/

/*
* Copyright (c) 2001-2004 Swedish Institute of Computer Science.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
* SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
* This file is part of the lwIP TCP/IP stack.
*
* Author: Adam Dunkels <adam@sics.se>
*
*/

/*
* This file is a skeleton for developing Ethernet network interface
* drivers for lwIP. Add code to the low_level functions and do a
* search-and-replace for the word "ethernetif" to replace it with
* something that better describes your network interface.
*/

/*
 * Copyright (c) 2021-2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "common.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/err.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/prot/ethernet.h"
#include "ethernetif.h"
#include "lwip/sys.h"

#if defined(NO_SYS) && !NO_SYS
#if defined(__ENABLE_FREERTOS) && __ENABLE_FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
#elif defined(__ENABLE_RTTHREAD_NANO) && __ENABLE_RTTHREAD_NANO
#include "rtthread.h"
#endif
#endif

#define netifMTU                           (1500)
#define netifINTERFACE_TASK_STACK_SIZE     (1024)

#if defined(__ENABLE_FREERTOS) && __ENABLE_FREERTOS
#define netifINTERFACE_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#elif defined(__ENABLE_RTTHREAD_NANO) && __ENABLE_RTTHREAD_NANO
#define netifINTERFACE_TASK_PRIORITY       (RT_THREAD_PRIORITY_MAX - 1)
#endif

#define netifGUARD_BLOCK_TIME              (250)

/* The time to block waiting for input. */
#define emacBLOCK_TIME_WAITING_FOR_INPUT   ((portTickType)100)

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#if defined(NO_SYS) && !NO_SYS
xSemaphoreHandle s_xSemaphore = NULL;
#endif

LWIP_MEMPOOL_DECLARE(enet0_rx_pool, ENET_RX_BUFF_COUNT, sizeof(my_custom_pbuf_t), "Custom RX PBUF pool");

static uint32_t s_debug_rx_print_count;
static uint32_t s_debug_tx_print_count;
static uint32_t s_debug_input_err_count;
static uint32_t s_rx_alloc_count;
static uint32_t s_rx_free_count;
static uint32_t s_heartbeat_rx_count;
static uint32_t s_heartbeat_tx_count;

static uint32_t cacheline_align_down(uint32_t address)
{
    return address & ~((uint32_t)HPM_L1C_CACHELINE_SIZE - 1U);
}

static uint32_t cacheline_align_up(uint32_t address)
{
    return (address + (uint32_t)HPM_L1C_CACHELINE_SIZE - 1U) & ~((uint32_t)HPM_L1C_CACHELINE_SIZE - 1U);
}

static void writeback_tx_buffer_region(const uint8_t *buffer, uint32_t used_length)
{
    if ((buffer == NULL) || (used_length == 0U)) {
        return;
    }

    const uint32_t start = cacheline_align_down((uint32_t)buffer);
    const uint32_t end = cacheline_align_up((uint32_t)buffer + used_length);
    l1c_dc_writeback(start, end - start);
}

/**
* In this function, the hardware should be initialized.
* Called from ethernetif_init().
*
* @param netif the already initialized lwip network interface structure
*        for this ethernetif
*/
static void low_level_init(struct netif *netif)
{
    /* Set netif MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* Set netif MAC hardware address */
    memcpy(netif->hwaddr, mac, ETH_HWADDR_LEN);

    /* Set netif maximum transfer unit */
    netif->mtu = netifMTU;

    /* Accept broadcast address and ARP traffic */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

    LWIP_MEMPOOL_INIT(enet0_rx_pool);

#if defined(NO_SYS) && !NO_SYS
    /* create binary semaphore used for informing ethernetif of frame reception */
    if (s_xSemaphore == NULL) {
        vSemaphoreCreateBinary(s_xSemaphore);
        xSemaphoreTake(s_xSemaphore, 0);
    }
    /* create the task that handles the ETH_MAC */
    xTaskCreate(ethernetif_input, "Eth_if", netifINTERFACE_TASK_STACK_SIZE, netif,
                netifINTERFACE_TASK_PRIORITY, NULL);
#endif
}


/**
* This function should do the actual transmission of the packet. The packet is
* contained in the pbuf that is passed to the function. This pbuf
* might be chained.
*
* @param netif the lwip network interface structure for this ethernetif
* @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
* @return ERR_OK if the packet could be sent
*         an err_t value if the packet couldn't be sent
*
* @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
*       strange results. You might consider waiting for space in the DMA queue
*       to become available since the stack doesn't retry to send a packet
*       dropped because of memory failure (except for the TCP timers).
*/

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
#if defined(NO_SYS) && !NO_SYS
    static xSemaphoreHandle xTxSemaphore = NULL;
#endif
    struct pbuf *q;
    uint8_t *buffer;

    __IO enet_tx_desc_t *dma_tx_desc;
    uint16_t frame_length = 0;
    uint32_t buffer_offset = 0;
    uint32_t bytes_left_to_copy = 0;
    uint32_t payload_offset = 0;
    uint32_t copy_length = 0;
    enet_tx_desc_t  *tx_desc_list_cur = desc.tx_desc_list_cur;
    const struct eth_hdr *ethhdr = (const struct eth_hdr *)p->payload;

    if (netif == NULL || p == NULL) {
        return ERR_VAL;
    }

    if (s_debug_tx_print_count < 64U) {
        printf("[LWIP/TX] call=%lu len=%u type=0x%04x\n",
               (unsigned long)(s_debug_tx_print_count + 1U),
               (unsigned)p->tot_len,
               (unsigned)lwip_ntohs(ethhdr->type));
    }

#if defined(LWIP_PTP) && LWIP_PTP
    enet_ptp_ts_system_t timestamp;
#endif

#if defined(NO_SYS) && !NO_SYS
    if (xTxSemaphore == NULL) {
        vSemaphoreCreateBinary(xTxSemaphore);
    }

    if (xSemaphoreTake(xTxSemaphore, netifGUARD_BLOCK_TIME)) {
#endif
        dma_tx_desc = tx_desc_list_cur;
        /* Invalidate cache to see DMA clearing OWN bit after previous TX */
        l1c_dc_invalidate(HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)dma_tx_desc),
                          HPM_L1C_CACHELINE_ALIGN_UP(sizeof(enet_tx_desc_t)));
        buffer = (uint8_t *)(dma_tx_desc->tdes2_bm.buffer1);
        buffer_offset = 0;

        for (q = p; q != NULL; q = q->next) {
            /* Get bytes in current lwIP buffer  */
            bytes_left_to_copy = q->len;
            payload_offset = 0;

            while (bytes_left_to_copy > 0U) {
                if (dma_tx_desc->tdes0_bm.own != 0U) {
                    printf("[LWIP/TX] busy own=1 len=%u type=0x%04x\n",
                           (unsigned)p->tot_len,
                           (unsigned)lwip_ntohs(ethhdr->type));
                    return ERR_INPROGRESS;
                }

                copy_length = LWIP_MIN(bytes_left_to_copy, (uint32_t)ENET_TX_BUFF_SIZE - buffer_offset);
                memcpy((uint8_t *)((uint8_t *)buffer + buffer_offset),
                       (uint8_t *)((uint8_t *)q->payload + payload_offset),
                       copy_length);

                buffer_offset += copy_length;
                payload_offset += copy_length;
                bytes_left_to_copy -= copy_length;
                frame_length += copy_length;

                if ((buffer_offset == ENET_TX_BUFF_SIZE) && ((bytes_left_to_copy > 0U) || (q->next != NULL))) {
                    writeback_tx_buffer_region(buffer, buffer_offset);
                    dma_tx_desc = (enet_tx_desc_t *)(dma_tx_desc->tdes3_bm.next_desc);
                    buffer = (uint8_t *)(dma_tx_desc->tdes2_bm.buffer1);
                    buffer_offset = 0U;
                }
            }
        }
        /* Hand payload length to the MAC; FCS is inserted by hardware. */
        writeback_tx_buffer_region(buffer, buffer_offset == 0U ? ENET_TX_BUFF_SIZE : buffer_offset);

        /* Invalidate TX descriptor cache line before writing OWN=1.
           Prevents false sharing: without this, the subsequent writeback
           could push stale CPU cache data for an adjacent TX descriptor
           (sharing the same 64-byte cache line) over DMA's state. */
        l1c_dc_invalidate(HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)tx_desc_list_cur),
                          HPM_L1C_CACHELINE_ALIGN_UP(sizeof(enet_tx_desc_t)));

        #if defined(LWIP_PTP) && LWIP_PTP
            enet_prepare_tx_desc_with_ts_record(ENET, &desc.tx_desc_list_cur, &desc.tx_control_config, frame_length, desc.tx_buff_cfg.size, &timestamp);
            /* Get the transmit timestamp */
            p->time_sec  = timestamp.sec;
            p->time_nsec = timestamp.nsec;
        #else
            enet_prepare_tx_desc(ENET, &desc.tx_desc_list_cur, &desc.tx_control_config, frame_length, desc.tx_buff_cfg.size);
        #endif
        /* Flush TX descriptor to physical memory so DMA can see OWN=1.
           Use tx_desc_list_cur (the local copy) because enet_prepare_tx_desc
           already advanced desc.tx_desc_list_cur to the NEXT descriptor. */
        l1c_dc_writeback(HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)tx_desc_list_cur),
                         HPM_L1C_CACHELINE_ALIGN_UP(sizeof(enet_tx_desc_t)));
        /* Re-trigger DMA poll: enet_prepare_tx_desc already wrote DMA_TX_POLL_DEMAND,
           but at that point OWN=1 was still in CPU cache and DMA saw stale OWN=0.
           Now that the descriptor is flushed, wake DMA up again. */
        ENET->DMA_TX_POLL_DEMAND = 1;
        ++s_debug_tx_print_count;
        ++s_heartbeat_tx_count;

#if defined(NO_SYS) && !NO_SYS
        /* Give semaphore and exit */
        xSemaphoreGive(xTxSemaphore);
    }
#endif

    return ERR_OK;
}

void free_rx_dma_descriptor(void *p)
{
    enet_frame_t *frame;

    frame = (enet_frame_t *)p;

    /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
    enet_rx_desc_t *dma_rx_desc = frame->rx_desc;

    for (uint32_t i = 0; i < frame->seg; i++) {
        /* Invalidate then modify: prevents false sharing with adjacent
           descriptor in the same 64-byte cache line.  Without the
           invalidate, writeback would push stale CPU cache data for the
           neighbor descriptor over DMA's fresh frame data. */
        l1c_dc_invalidate(HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)dma_rx_desc),
                          HPM_L1C_CACHELINE_ALIGN_UP(sizeof(enet_rx_desc_t)));
        dma_rx_desc->rdes0_bm.own = 1;
        l1c_dc_writeback(HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)dma_rx_desc),
                         HPM_L1C_CACHELINE_ALIGN_UP(sizeof(enet_rx_desc_t)));
        dma_rx_desc = (enet_rx_desc_t *)(dma_rx_desc->rdes3_bm.next_desc);
    }

    /* Clear Segment_Count */
    frame->seg = 0;
    frame->used = 0;
    frame->length = 0;
}

void enet0_pbuf_free_custom(struct pbuf *p)
{
    SYS_ARCH_DECL_PROTECT(old_level);
    my_custom_pbuf_t *my_pbuf = (my_custom_pbuf_t *)p;

    SYS_ARCH_PROTECT(old_level);
    free_rx_dma_descriptor((void *)my_pbuf->dma_descriptor);
    LWIP_MEMPOOL_FREE(enet0_rx_pool, my_pbuf);
    ++s_rx_free_count;
    SYS_ARCH_UNPROTECT(old_level);
}

/**
* Should allocate a pbuf and transfer the bytes of the incoming
* packet from the interface into the pbuf.
*
* @param netif the lwip network interface structure for this ethernetif
* @return a pbuf filled with the received packet (including MAC header)
*         NULL on memory error
*/
static struct pbuf *low_level_input(struct netif *netif)
{
    struct pbuf *p = NULL;
    uint8_t *buffer;
    uint32_t len;
    my_custom_pbuf_t *my_pbuf;
    enet_frame_pointer_t *fp = (enet_frame_pointer_t *)netif->state;

    if (fp->frame[fp->idx].used == 0) {
        /* Invalidate cache on current RX descriptor so DMA writes are visible */
        l1c_dc_invalidate(HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)desc.rx_desc_list_cur),
                          HPM_L1C_CACHELINE_ALIGN_UP(sizeof(enet_rx_desc_t)));

        #if defined(__ENABLE_ENET_RECEIVE_INTERRUPT) && __ENABLE_ENET_RECEIVE_INTERRUPT || defined(NO_SYS) && !NO_SYS
        fp->frame[fp->idx] = enet_get_received_frame_interrupt(&desc.rx_desc_list_cur, &desc.rx_frame_info, ENET_RX_BUFF_COUNT);
        #else
        if (enet_check_received_frame(&desc.rx_desc_list_cur, &desc.rx_frame_info) == 1) {
            fp->frame[fp->idx] = enet_get_received_frame(&desc.rx_desc_list_cur, &desc.rx_frame_info);
        }
        #endif

        /* Obtain the size of the packet and put it into the "len" variable. */
        len = fp->frame[fp->idx].length;
        buffer = (uint8_t *)fp->frame[fp->idx].buffer;

        if (len > 0) {
            /* Allocate a pbuf chain of pbufs from the custom buffer pool */
            my_pbuf = (my_custom_pbuf_t *)LWIP_MEMPOOL_ALLOC(enet0_rx_pool);
            if (my_pbuf == NULL) {
                printf("[LWIP/RX] POOL EXHAUSTED alloc=%lu free=%lu\n",
                       (unsigned long)s_rx_alloc_count,
                       (unsigned long)s_rx_free_count);
                free_rx_dma_descriptor((void *)&fp->frame[fp->idx]);
                desc.rx_frame_info.seg_count = 0;
                return NULL;
            }
            ++s_rx_alloc_count;

            my_pbuf->p.custom_free_function = enet0_pbuf_free_custom;
            my_pbuf->dma_descriptor = (void *)&fp->frame[fp->idx];

            p = pbuf_alloced_custom(PBUF_RAW, fp->frame[fp->idx].length, PBUF_REF, &my_pbuf->p, (uint8_t *)fp->frame[fp->idx].buffer, ENET_RX_BUFF_SIZE);

            if (p != NULL) {
                fp->frame[fp->idx].used = 1;
                l1c_dc_invalidate((uint32_t)buffer, ENET_RX_BUFF_SIZE);
                if (s_debug_rx_print_count < 64U) {
                    const struct eth_hdr *ethhdr = (const struct eth_hdr *)p->payload;
                    printf("[LWIP/RX] frame=%lu len=%lu type=0x%04x dst=%02x:%02x:%02x:%02x:%02x:%02x src=%02x:%02x:%02x:%02x:%02x:%02x\n",
                           (unsigned long)(s_debug_rx_print_count + 1U),
                           (unsigned long)fp->frame[fp->idx].length,
                           (unsigned)lwip_ntohs(ethhdr->type),
                           (unsigned)ethhdr->dest.addr[0],
                           (unsigned)ethhdr->dest.addr[1],
                           (unsigned)ethhdr->dest.addr[2],
                           (unsigned)ethhdr->dest.addr[3],
                           (unsigned)ethhdr->dest.addr[4],
                           (unsigned)ethhdr->dest.addr[5],
                           (unsigned)ethhdr->src.addr[0],
                           (unsigned)ethhdr->src.addr[1],
                           (unsigned)ethhdr->src.addr[2],
                           (unsigned)ethhdr->src.addr[3],
                           (unsigned)ethhdr->src.addr[4],
                           (unsigned)ethhdr->src.addr[5]);
                }
                ++s_debug_rx_print_count;
                ++s_heartbeat_rx_count;
                /* Heartbeat: print summary every 64 RX frames */
                if ((s_heartbeat_rx_count & 63U) == 0U) {
                    printf("[LWIP/HB] rx_total=%lu tx_total=%lu rx_alloc=%lu rx_free=%lu\n",
                           (unsigned long)s_debug_rx_print_count,
                           (unsigned long)s_debug_tx_print_count,
                           (unsigned long)s_rx_alloc_count,
                           (unsigned long)s_rx_free_count);
                }
                #if defined(LWIP_PTP) && LWIP_PTP
                /* Get the received timestamp */
                p->time_sec  = fp->frame[fp->idx].rx_desc->rdes7_bm.rtsh;
                p->time_nsec = fp->frame[fp->idx].rx_desc->rdes6_bm.rtsl;
                #endif
            } else {
                LWIP_MEMPOOL_FREE(enet0_rx_pool, my_pbuf);
                free_rx_dma_descriptor((void *)&fp->frame[fp->idx]);
                desc.rx_frame_info.seg_count = 0;
                return NULL;
            }

            ++fp->idx;
            fp->idx %= ENET_RX_BUFF_COUNT;

            /* Clear Segment_Count */
            desc.rx_frame_info.seg_count = 0;
        }
    }

    /* Resume Rx Process */
    enet_rx_resume(ENET);

    return p;
}


/**
* This function is the ethernetif_input task, it is processed when a packet
* is ready to be read from the interface. It uses the function low_level_input()
* that should handle the actual reception of bytes from the network
* interface. Then the type of the received packet is determined and
* the appropriate input function is called.
*
* @param netif the lwip network interface structure for this ethernetif
*/

 /*
  invoked after receiving data packet
 */
#if defined(NO_SYS) && !NO_SYS
void ethernetif_input(void *pvParameters)
{
    struct netif *netif = (struct netif *)pvParameters;

    struct pbuf *p;

    for ( ;; ) {
        if (xSemaphoreTake(s_xSemaphore, emacBLOCK_TIME_WAITING_FOR_INPUT) == pdTRUE) {
GET_NEXT_FRAME:
            p = low_level_input(netif);
            if (p != NULL) {
                if (ERR_OK != netif->input(p, netif)) {
                    pbuf_free(p);
                } else {
                    goto GET_NEXT_FRAME;
                }
            }
        }
    }
}
#else
err_t ethernetif_input(struct netif *netif)
{
    err_t err = ERR_OK;
    struct pbuf *p = NULL;
#if defined(__ENABLE_ENET_RECEIVE_INTERRUPT) && __ENABLE_ENET_RECEIVE_INTERRUPT
    if (rx_flag) {
#endif
        GET_NEXT_FRAME:
        /* move received packet into a new pbuf */
        p = low_level_input(netif);

        /* no packet could be read, silently ignore this */
        if (p == NULL) {
            err = ERR_MEM;
        } else {
             /* entry point to the LwIP stack */
            err = netif->input(p, netif);

            if (err != ERR_OK) {
                ++s_debug_input_err_count;
                printf("[LWIP/IN] err=%d count=%lu\n",
                       (int)err,
                       (unsigned long)s_debug_input_err_count);
                LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                pbuf_free(p);
            } else {
                goto GET_NEXT_FRAME;
            }
        }
#if defined(__ENABLE_ENET_RECEIVE_INTERRUPT) && __ENABLE_ENET_RECEIVE_INTERRUPT
        rx_flag = false;
    }
#endif
    return err;
}
#endif

/**
* Should be called at the beginning of the program to set up the
* network interface. It calls the function low_level_init() to do the
* actual setup of the hardware.
*
* This function should be passed as a parameter to netif_add().
*
* @param netif the lwip network interface structure for this ethernetif
* @return ERR_OK if the loopif is initialized
*         ERR_MEM if private data couldn't be allocated
*         any other err_t on error
*/
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    low_level_init(netif);

    etharp_init();

    return ERR_OK;
}

uint32_t ethernetif_debug_rx_count(void)
{
    return s_debug_rx_print_count;
}

uint32_t ethernetif_debug_tx_count(void)
{
    return s_debug_tx_print_count;
}

uint32_t ethernetif_debug_input_err_count(void)
{
    return s_debug_input_err_count;
}

uint32_t ethernetif_debug_rx_alloc_count(void)
{
    return s_rx_alloc_count;
}

uint32_t ethernetif_debug_rx_free_count(void)
{
    return s_rx_free_count;
}
