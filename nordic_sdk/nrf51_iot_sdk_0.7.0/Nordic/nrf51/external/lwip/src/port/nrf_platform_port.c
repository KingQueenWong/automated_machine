/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/netif.h"
#include "lwip/ip6.h"
#include "sdk_config.h"
#include "app_timer.h"
#include "ble_ipsp.h"
#include "ble_6lowpan.h"
#include "mem_manager.h"
#include "iot_context_manager.h"
#include "app_trace.h"
#include "nrf_platform_port.h"

/**
 * @defgroup nrf51_driver_debug_log Module's Log Macros
 * @details Macros used for creating module logs which can be useful in understanding handling
 *          of events or actions on API requests. These are intended for debugging purposes and
 *          can be disabled by defining the NRF51_DRIVER_DISABLE_LOGS.
 * @note That if ENABLE_DEBUG_LOG_SUPPORT is disabled, having NRF51_DRIVER_DISABLE_LOGS has no effect.
 * @{
 */
#if (NRF51_LWIP_DRIVER_DISABLE_LOGS == 0)

#define NRF51_DRIVER_LOG  app_trace_log                           /**< Used for logging details. */
#define NRF51_DRIVER_ERR  app_trace_log                           /**< Used for logging errors in the module. */
#define NRF51_DRIVER_TRC  app_trace_log                           /**< Used for getting trace of execution in the module. */
#define NRF51_DRIVER_DUMP app_trace_dump                          /**< Used for dumping octet information to get details of bond information etc. */

#else // NRF51_LWIP_DRIVER_DISABLE_LOGS

#define NRF51_DRIVER_DUMP(...)                                    /**< Diasbles dumping of octet streams. */
#define NRF51_DRIVER_LOG(...)                                     /**< Diasbles detailed logs. */
#define NRF51_DRIVER_ERR(...)                                     /**< Diasbles error logs. */
#define NRF51_DRIVER_TRC(...)                                     /**< Diasbles traces. */

#endif // NRF51_LWIP_DRIVER_DISABLE_LOGS


/** Driver struct to maintain mapping between IP interface and Bluetooth as transport. */
struct blenetif {
  iot_interface_t * p_ble_interface;
  struct netif      netif;
};

struct blenetif m_blenetif_table[BLE_6LOWPAN_MAX_INTERFACE];   /**< Table maintaining network interface of LwIP and also corresponding 6lowpan interface. */


void  nrf51_mem_init(void)
{
    // Do nothing intialization of the module to happen in application.
}


void *nrf51_mem_alloc(mem_size_t size)
{
    uint8_t * buffer = NULL;
    uint32_t allocated_size = size;

    uint32_t retval = nrf51_sdk_mem_alloc(&buffer, &allocated_size);

    if (retval != NRF_SUCCESS)
    {
        buffer = NULL;
    }

    return buffer;
}


void *nrf51_mem_calloc(mem_size_t count, mem_size_t size)
{
    uint8_t * buffer = NULL;
    uint32_t allocated_size = size;

    uint32_t retval = nrf51_sdk_mem_alloc(&buffer,&allocated_size);
    if (retval == NRF_SUCCESS)
    {
        memset(buffer,0, size);
    }
    else
    {
        buffer = NULL;
    }
    return buffer;
}


void nrf51_mem_free(void *mem)
{
    UNUSED_VARIABLE(nrf51_sdk_mem_free(mem));
}


void * nrf51_mem_trim(void *mem, mem_size_t size)
{
    return mem;
}


/** @brief Function to add received packet to pbuf queue and notify the stack of received packet. */
static void blenetif_input(struct netif  * p_netif,  uint8_t * p_payload, uint16_t payload_len)
{
    struct pbuf  * p_buffer;

    NRF51_DRIVER_LOG("[IP-DRI]: >> blenetif_input\r\n");
    NRF51_DRIVER_DUMP(p_payload, payload_len);

    //Allocate a pbuf chain of pbufs from the pool.
    p_buffer = pbuf_alloc(PBUF_RAW, payload_len, PBUF_REF);

    if (p_buffer != NULL)
    {
        p_buffer->payload = p_payload;
        p_buffer->len     = payload_len;
        p_buffer->tot_len = payload_len;
        if (ip6_input(p_buffer, p_netif) != ERR_OK)
        {
            NRF51_DRIVER_LOG("[IP-DRI]: IP Stack returned error.\r\n");
        }
        UNUSED_VARIABLE(pbuf_free(p_buffer));
    }

    NRF51_DRIVER_LOG("[IP-DRI]: << blenetif_input\r\n");
}


/** @brief Network interface output function registered with LwIP to send packets on 6lowpan. */
static err_t blenetif_output(struct netif * p_netif, struct pbuf * p_buffer, ip6_addr_t * p_addr)
{
    struct blenetif * p_blenetif    = (struct blenetif *)p_netif->state;
    uint8_t         * p_payload;
    err_t             error_code    = ERR_MEM;
    const    uint16_t requested_len = p_buffer->len;


    NRF51_DRIVER_LOG("[IP-DRI]: >> blenetif_output\r\n");
    NRF51_DRIVER_DUMP(p_buffer->payload, requested_len);

    p_payload = nrf51_mem_alloc(p_buffer->len);

    if (NULL != p_payload)
    {
        memcpy(p_payload, p_buffer->payload, p_buffer->len);
        uint32_t retval = ble_6lowpan_interface_send(p_blenetif->p_ble_interface,
                                                     p_payload,
                                                     requested_len);
        if (retval != NRF_SUCCESS)
        {
            NRF51_DRIVER_ERR("[IP-DRI]: Failed to send IP packet, reason 0x%08X\r\n", retval);
            nrf51_mem_free(p_payload);
        }
        else
        {
            error_code = ERR_OK;
        }
    }
    else
    {
         NRF51_DRIVER_ERR("[IP-DRI]: Failed to allocate memory for output packet\r\n");
    }

    NRF51_DRIVER_LOG("[IP-DRI]: << blenetif_output\r\n");

    return error_code;

}


/** @brief Initialization once network interface has been added. */
static err_t blenetif_init(struct netif * p_netif)
{
    //Set link MTU size.
    p_netif->mtu = BLE_IPSP_MTU;

    //Device capabilities and link status
    p_netif->flags = NETIF_FLAG_LINK_UP;

    //Indicate interface link is up.
    netif_set_up(p_netif);

    //Enable Address Auto configuration.
    p_netif->ip6_autoconfig_enabled = 1;

    return ERR_OK;
}


/**@brief Callback registered with 6lowpan to receive asynchronous events. */
static void blenetif_transport_callback(iot_interface_t     * p_interface,
                                        ble_6lowpan_event_t * p_event)
{
    uint32_t index;
    uint32_t err_code;
    struct blenetif * p_blenetif = NULL;

    // Search network interface added for the 6lowpan interface that has generated the event.
    for (index = 0; index < BLE_6LOWPAN_MAX_INTERFACE; index++)
    {
        if (p_interface == m_blenetif_table[index].p_ble_interface)
        {
            p_blenetif = &m_blenetif_table[index];
            break;
        }
    }

    switch(p_event->event_id)
    {
        case BLE_6LO_EVT_INTERFACE_ADD:
        {
            if (p_blenetif == NULL)
            {
                for (index = 0; index < BLE_6LOWPAN_MAX_INTERFACE; index++)
                {
                    if (m_blenetif_table[index].p_ble_interface == NULL)
                    {
                        p_blenetif = &m_blenetif_table[index];
                        p_blenetif->p_ble_interface = p_interface;
                        break;
                    }
                }
            }
            else
            {
                // Interface already exists, this is an error, should not happen.
                break;
            }

            err_code = iot_context_manager_table_alloc(p_interface);

            if(err_code == NRF_SUCCESS)
            {
                NRF51_DRIVER_LOG("[IP-DRI]: Successfully allocated context table!\r\n");
            }
            else
            {
                NRF51_DRIVER_ERR("[IP-DRI]: Failed to allocate context table!\r\n");
            }

            p_blenetif->netif.hwaddr[0] = p_interface->local_addr.identifier[0];
            p_blenetif->netif.hwaddr[1] = p_interface->local_addr.identifier[1];
            p_blenetif->netif.hwaddr[2] = p_interface->local_addr.identifier[2];
            p_blenetif->netif.hwaddr[3] = p_interface->local_addr.identifier[5];
            p_blenetif->netif.hwaddr[4] = p_interface->local_addr.identifier[6];
            p_blenetif->netif.hwaddr[5] = p_interface->local_addr.identifier[7];
            p_blenetif->netif.hwaddr_len = BLE_GAP_ADDR_LEN;

            UNUSED_VARIABLE(netif_add(&p_blenetif->netif, NULL, NULL, NULL, p_blenetif, blenetif_init, NULL));

            p_blenetif->netif.ip6_addr[0].addr[0]  = HTONL(0xFE800000);
            p_blenetif->netif.ip6_addr[0].addr[1]  = HTONL(0x00000000);
            p_blenetif->netif.ip6_addr[0].addr[2]  = *((u32_t *) (&p_interface->local_addr.identifier[0]));
            p_blenetif->netif.ip6_addr[0].addr[3]  = *((u32_t *) (&p_interface->local_addr.identifier[4]));

            // Flip local/universal bit.
            p_blenetif->netif.ip6_addr[0].addr[2] ^= IPV6_IID_FLIP_VALUE;
            p_blenetif->netif.ip6_addr_state[0]    = IP6_ADDR_TENTATIVE;

            p_blenetif->netif.output_ip6           = blenetif_output;

            nrf51_driver_interface_up();
            break;
        }
        case BLE_6LO_EVT_INTERFACE_DELETE:
        {
            if (p_blenetif != NULL)
            {
                netif_remove(&p_blenetif->netif);
                p_blenetif->p_ble_interface = NULL;

                 err_code = iot_context_manager_table_free(p_interface);

                if(err_code == NRF_SUCCESS)
                {
                    NRF51_DRIVER_ERR("[IP-DRI]: Successfully freed context table!\r\n");
                }
                nrf51_driver_interface_down();
            }
            break;
        }
        case BLE_6LO_EVT_INTERFACE_DATA_RX:
        {
            if (p_blenetif != NULL)
            {
                // Handle received IP packet.
                blenetif_input(&p_blenetif->netif,
                               p_event->event_param.rx_event_param.p_packet,
                               p_event->event_param.rx_event_param.packet_len);
                nrf51_mem_free( p_event->event_param.rx_event_param.p_packet);
            }
            else
            {
                NRF51_DRIVER_ERR("[IP-DRI]: Dropping packet, unknown interface.\r\n");
            }
            break;
        }
        default:
            break;
    }
}


/**@brief Driver initialization. */
uint32_t nrf51_driver_init(void)
{
    uint32_t           err_code;
    uint32_t           index;
    ble_6lowpan_init_t init_param;

    init_param.event_handler = blenetif_transport_callback;
    init_param.p_eui64       = EUI64_LOCAL_IID;

    err_code = iot_context_manager_init();

    if(err_code == NRF_SUCCESS)
    {
        /* 6lowpan initialization and callback registration. */
        err_code = ble_6lowpan_init(&init_param);

        for (index = 0; index < BLE_6LOWPAN_MAX_INTERFACE; index++)
        {
            m_blenetif_table[index].p_ble_interface = NULL;
        }
    }

    return err_code;
}


/**@brief  Message function to rediect LwIP debug traces to nrf51 tracing. */
void nrf51_message(const char * m)
{
    NRF51_DRIVER_LOG(m);
}


/**@breif Interface implementation expected by the LwIP stack to get system ticks. */
u32_t sys_now(void)
{
    u32_t sys_tick;

    UNUSED_VARIABLE(app_timer_cnt_get(&sys_tick));

    //Note: In case prescaler value is changed, this calculation to get sys tick applies.
    //Currently, this is not used for saving in code execution as sys_now is called
    //often.

    //sys_tick = ROUNDED_DIV(((sys_tick & 0x00FFFFF) * (NRF51_DRIVER_TIMER_PRESCALER + 1) * 1000), APP_TIMER_CLOCK_FREQ);
    //sys_tick *= 8;
    return sys_tick;
}
