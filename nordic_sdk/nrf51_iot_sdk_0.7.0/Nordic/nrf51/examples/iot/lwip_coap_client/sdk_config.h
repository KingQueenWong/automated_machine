/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
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
#ifndef SDK_CONFIG_H__
#define SDK_CONFIG_H__

/**
 * @defgroup sdk_config SDK Configuration
 * @{
 * @ingroup sdk_common
 * @{
 * @details All parameters that allow configuring/tuning the SDK based on application/ use case
 *          are defined here.
 */

 
/**
 * @defgroup mem_manager_config Memory Manager Configuration
 * @{
 * @addtogroup sdk_config
 * @{
 * @details This section defines configuration of memory manager module.
 */

/**
 * @brief Maximum memory blocks identified as 'small' blocks.
 *
 * @details Maximum memory blocks identified as 'small' blocks.
 *          Minimum value : 0 (Setting to 0 disables all 'small' blocks)
 *          Maximum value : 255
 *          Dependencies  : None.
 */
#define  MEMORY_MANAGER_SMALL_BLOCK_COUNT                  8

/**
 * @brief Size of each memory blocks identified as 'small' block.
 *
 * @details Size of each memory blocks identified as 'small' block.
 *          Memory block are recommended to be word-sized.
 *          Minimum value : 32
 *          Maximum value : A value less than the next pool size. If only small pool is used, this
 *                          can be any value based on availability of RAM.
 *          Dependencies  : MEMORY_MANAGER_SMALL_BLOCK_COUNT is non-zero.
 */
#define  MEMORY_MANAGER_SMALL_BLOCK_SIZE                   64

/**
 * @brief Maximum memory blocks identified as 'medium' blocks.
 *
 * @details Maximum memory blocks identified as 'medium' blocks.
 *          Minimum value : 0 (Setting to 0 disables all 'medium' blocks)
 *          Maximum value : 255
 *          Dependencies  : None.
 */
#define  MEMORY_MANAGER_MEDIUM_BLOCK_COUNT                 8

/**
 * @brief Size of each memory blocks identified as 'medium' block.
 *
 * @details Size of each memory blocks identified as 'medium' block.
 *          Memory block are recommended to be word-sized.
 *          Minimum value : A value greater than the small pool size if defined, else 32.
 *          Maximum value : A value less than the next pool size. If only medium pool is used, this
 *                          can be any value based on availability of RAM.
 *          Dependencies  : MEMORY_MANAGER_MEDIUM_BLOCK_COUNT is non-zero.
 */
#define  MEMORY_MANAGER_MEDIUM_BLOCK_SIZE                  256

/**
 * @brief Maximum memory blocks identified as 'medium' blocks.
 *
 * @details Maximum memory blocks identified as 'medium' blocks.
 *          Minimum value : 0 (Setting to 0 disables all 'large' blocks)
 *          Maximum value : 255
 *          Dependencies  : None.
 */
#define  MEMORY_MANAGER_LARGE_BLOCK_COUNT                  1

/**
 * @brief Size of each memory blocks identified as 'medium' block.
 *
 * @details Size of each memory blocks identified as 'medium' block.
 *          Memory block are recommended to be word-sized.
 *          Minimum value : A value greater than the small &/ medium pool size if defined, else 32.
 *          Maximum value : Any value based on availability of RAM.
 *          Dependencies  : MEMORY_MANAGER_MEDIUM_BLOCK_COUNT is non-zero.
 */
#define  MEMORY_MANAGER_LARGE_BLOCK_SIZE                   1024

/**
 * @brief Disable debug trace in the module.
 *
 * @details Set this define to 1 to enable debug trace in the module, else set to 0.
 *          Possible values : 0 or 1.
 *          Dependencies    : ENABLE_DEBUG_LOG_SUPPORT. If this flag is not defined, no
 *                            trace is observed even if this define is set to 1.
 */
#define MEM_MANAGER_DISABLE_LOGS                           1

/**
 * @brief Disables API parameter checks in the module.
 *
 * @details Set this define to 1 to disable checks on API parameters in the module.
 *          API parameter checks are added to ensure right parameters are passed to the
 *          module. These checks are useful during development phase but be redundant
 *          once application is developed. Disabling this can result in some code saving.
 *          Possible values : 0 or 1.
 *          Dependencies    : None.
 */
#define MEM_MANAGER_DISABLE_API_PARAM_CHECK                0
/** @} */
/** @} */


/**
 * @defgroup iot_context_manager Context Manager Configurations.
 * @{
 * @addtogroup iot_config
 * @{
 * @details This section defines configuration of Context Manager.
 */
 
/**
 * @brief Disable debug trace in the module.
 *
 * @details Set this define to 1 to enable debug trace in the module, else set to 0.
 *          Possible values : 0 or 1.
 *          Dependencies    : ENABLE_DEBUG_LOG_SUPPORT. If this flag is not defined, no
 *                            trace is observed even if this define is set to 1.
 */
#define IOT_CONTEXT_MANAGER_DISABLE_LOGS                    1

/**
 * @brief Disables API parameter checks in the module.
 *
 * @details Set this define to 1 to disable checks on API parameters in the module.
 *          API parameter checks are added to ensure right parameters are passed to the
 *          module. These checks are useful during development phase but be redundant
 *          once application is developed. Disabling this can result in some code saving.
 *          Possible values : 0 or 1.
 *          Dependencies    : None.
 */
#define IOT_CONTEXT_MANAGER_DISABLE_API_PARAM_CHECK        0

/**
 * @brief Maximum number of supported context identifiers.
 *
 * @details Maximum value of 16 is preferable to correct decompression.
 *          Minimum value : 1
 *          Maximum value : 16
 *          Dependencies  : None.
 */
#define  IOT_CONTEXT_MANAGER_MAX_CONTEXTS                  16

/**
 * @brief Maximum number of supported context's table.
 *
 * @details If value is equal to BLE_IPSP_MAX_CHANNELS then all interface will have
 *          its own table which is preferable.
 *          Minimum value : 1
 *          Maximum value : BLE_IPSP_MAX_CHANNELS
 *          Dependencies  : None.
 */
#define  IOT_CONTEXT_MANAGER_MAX_TABLES                    1
/** @} */
/** @} */


/**
 * @defgroup lwip_nrf51_driver nRF51 lwIP driver
 * @{
 * @addtogroup iot_config
 * @{
 * @details This section defines configuration of nRF51 lwIP driver.
 */
 
/**
 * @brief Disable debug trace in the module.
 *
 * @details Set this define to 1 to enable debug trace in the module, else set to 0.
 *          Possible values : 0 or 1.
 *          Dependencies    : ENABLE_DEBUG_LOG_SUPPORT. If this flag is not defined, no
 *                            trace is observed even if this define is set to 1.
 */
#define NRF51_LWIP_DRIVER_DISABLE_LOGS                     1

/** @} */
/** @} */


/**
 * @defgroup iot_coap_config CoAP Configuration
 * @{
 * @addtogroup iot_config
 * @{
 * @details This section defines configuration of CoAP module.
 */

/*
 * @brief Disable debug trace in the module.
 *
 * @details Set this define to 1 to enable debug trace in the module, else set to 0.
 *          Possible values : 0 or 1.
 *          Dependencies    : ENABLE_DEBUG_LOG_SUPPORT. If this flag is not defined, no
 *                            trace is observed even if this define is set to 1.
 */
#define COAP_DISABLE_LOGS                                 1

/**
 * @brief Disables API parameter checks in the module.
 *
 * @details Set this define to 1 to disable checks on API parameters in the module.
 *          API parameter checks are added to ensure right parameters are passed to the
 *          module. These checks are useful during development phase but be redundant
 *          once application is developed. Disabling this can result in some code saving.
 *          Possible values : 0 or 1.
 *          Dependencies    : None.
 */
#define COAP_DISABLE_API_PARAM_CHECK                      0

/**
 * @brief CoAP version number.
 *
 * @details  The version of CoAP which all CoAP messages will be populated with.
 *           Minimum value     : 0
 *           Maximum value     : 3
 *           Recommended value : 1
 *           Dependencies      : None
 */
#define COAP_VERSION                                      1 

/**
 * @brief Number of local ports used by CoAP.
 * 
 * @details  The max number of client/server ports used by the application. One socket
 *           will be created for each port. 
 *           Minimum value : 0
 *           Maximum value : UDP6_MAX_SOCKET_COUNT
 *           Dependencies  : UDP6_MAX_SOCKET_COUNT
 */
#define COAP_PORT_COUNT                                   2

/**
 * @brief The port number used for a CoAP server. 
 *
 * @details  Minimum value : 1
 *           Maximum value : 65535
 *           Dependencies  : None
 */
#define COAP_SERVER_PORT                                  5683

/**
 * @brief The port number used for a CoAP client. 
 *
 * @details  Minimum value : 1
 *           Maximum value : 65535
 *           Dependencies  : None
 */
#define COAP_CLIENT_PORT                                  9999

/**
 * @brief Maximum number of CoAP message options.
 *
 * @details Maximum number of CoAP options that could be present in a message.
 *          Minimum value : 1
 *          Maximum value : 255
 *          Dependencies  : None.
 */
#define COAP_MAX_NUMBER_OF_OPTIONS                        8

/**
 * @brief The maximum size of a nCoAP message excluding the mandatory CoAP header.
 * 
 * @details  Minimum value : 1
 *           Maximum value : 65535
 *           Dependencies  : None
 */
#define COAP_MESSAGE_DATA_MAX_SIZE                        256

/**
 * @brief Maximum size of CoAP client request transmission queue.
 *
 * @details  The maximum number of nCoAP request messages that can be in transmission at the 
 *           same time (client). As nCoAP is using the memory manager also used by the underlying
 *           transport, the number of buffers should be increased as well. Depending on the 
 *           COAP_MESSAGE_DATA_MAX_SIZE + 4 byte CoAP header, you need to either increase 
 *           MEMORY_MANAGER_SMALL_BLOCK_COUNT or MEMORY_MANAGER_MEDIUM_BLOCK_COUNT to make sure you
 *           have additional buffers for CoAP message queue. The macro to increase depends on the 
 *           size of the buffer that is sufficient for the CoAP message. 
 *           
 *           Minimum value     : 1
 *           Maximum value     : 65535
 *           Recommended value : 4
 *           Dependencies      : MEMORY_MANAGER_SMALL_BLOCK_COUNT 
 *                               MEMORY_MANAGER_MEDIUM_BLOCK_COUNT 
 *                               MEMORY_MANAGER_SMALL_BLOCK_SIZE
 *                               MEMORY_MANAGER_MEDIUM_BLOCK_SIZE
 */
#define COAP_MESSAGE_QUEUE_SIZE                           4

/**
 * @brief Maximum length of CoAP resource verbose name.
 *
 * @details  The maximum length of resource name that can be supplied from the application. 
 *           Minimum value : 1
 *           Maximum value : 65535
 *           Dependencies  : None
 *
 * @note: 1 extra byte will be added to the resource name to make sure the string provided 
 *        is zero terminated. 
 */
#define COAP_RESOURCE_MAX_NAME_LEN                        19

/**
 * @brief Maximum number of CoAP resource levels.
 *
 * @details  The maximum number of resource depth levels uCoAP will use. The number will be used 
 *           when adding resource to the resource structure, or when traversing the resources for 
 *           a matching resource name given in a request. Each level added will increase the stack 
 *           usage runtime with 4 bytes.
 *           Minimum value     : 1
 *           Maximum value     : 255
 *           Recommended value : 1 - 10
 *           Dependencies      : None
 */
#define COAP_RESOURCE_MAX_DEPTH                           5

/** @} */
/** @} */

#endif // SDK_CONFIG_H__
