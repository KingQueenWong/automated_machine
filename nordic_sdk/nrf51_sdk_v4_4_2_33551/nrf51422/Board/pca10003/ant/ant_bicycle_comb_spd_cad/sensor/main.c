/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
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
 
/**@file
 * @brief ANT combined bicycle speed and cadence sensor application main entry. 
 * Configures and starts SoftDevice and all other needed modules.
 * @defgroup ant_combined_bicycle_speed_and_cadence_sensor ANT combined bicycle speed and cadence 
 * sensor example
 * @{
 * @ingroup nrf_ant_bicycle_speed_and_cadence
 *
 * @brief The combined bicycle speed and cadence sensor implementation.
 *
 * The combined bicycle speed and cadence sensor implementation implements all the features, which 
 * are required for this type of sensor to be compliant with the ANT+ Bicycle Speed and Cadence 
 * profile, as defined by the ANT+ Bike Speed and Cadence Device Profile specification:
 * - Transmit data about the users speed and cadence over a single ANT channel.
 * 
 * The general architecture of the combined bicycle speed and cadence sensor is illustrated in the 
 * picture below.
 *
 *
 * @image html bicycle_speed_and_cadence_sensor_design.png "Architecture overview"
 *
 * @par Combined bicycle speed and cadence sensor configuration options 
 *
 * The following compile time configuration options are available to suite various 
 * combined bicycle speed and cadence sensor implementations:
 * - CBSC_NETWORK_KEY        ANT PLUS network key.
 * - ANTPLUS_NETWORK_NUMBER  Network number associated with the channel.
 * - CBSC_TX_ANT_CHANNEL     Used channel number.
 * - CBSC_TX_DEVICE_NUMBER   Used device number.
 * - CBSC_DEVICE_TYPE        Used device type.
 * - CBSC_TX_TRANS_TYPE      Used transmission type.
 * - CBSC_RF_FREQ            Used channel radio frequency offset from 2400MHz.
 * - CBSC_MSG_PERIOD         Used channel period used in 32 kHz counts. 
 *
 * @par Development phase configuration options
 *
 * The combined bicycle speed and cadence sensor will trace error handler specific information to 
 * UART depending on the selected compile time configuration options. 
 *
 * The following compile time configuration options are available, and enabled by default, to assist
 * in the development phase of the combined bicycle speed and cadence sensor implementation:
 * - TRACE_UART              When defined UART trace outputting is enabled. 
 * 
 * @note The ANT+ Network Key is available for ANT+ Adopters. Please refer to http://thisisant.com 
 * to become an ANT+ Adopter and access the key. 
 */

#include <stdint.h>
#if defined(TRACE_UART)
    #include <stdio.h>  
    #include "app_uart.h"
#endif // defined(TRACE_UART) 
#include "app_error.h"
#include "ant_interface.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "boards.h"
#include "main_cbsc_tx.h"
#include "nordic_common.h"

#define UART_TX_BUF_SIZE 128u /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 1u   /**< UART RX buffer size. */


/**@brief Function for handling SoftDevice asserts, does not return.
 * 
 * Traces out the user supplied parameters and busy loops. 
 *
 * @param[in] pc          Value of the program counter.
 * @param[in] line_num    Line number where the assert occurred.
 * @param[in] p_file_name Pointer to the file name.
 */
void softdevice_assert_callback(uint32_t pc, uint16_t line_num, const uint8_t * p_file_name)
{
    printf("ASSERT-softdevice_assert_callback\n");
    printf("PC: %#x\n", pc);
    printf("File name: %s\n", (const char*)p_file_name);
    printf("Line number: %u\n", line_num);

    for (;;)
    {
        // No implementation needed.
    }
}


/**@brief Function for handling an error. 
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the error occurred.
 * @param[in] p_file_name Pointer to the file name. 
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    printf("ASSERT-app_error_handler\n");
    printf("Error code: %u\n", error_code);
    printf("File name: %s\n", (const char*)p_file_name);
    printf("Line number: %u\n", line_num);

    for (;;)
    {
        // No implementation needed.
    }
}


#if defined(TRACE_UART)    
/**@brief Function for handling an UART error.
 *
 * @param[in] p_event     Event supplied to the handler.
 */
void uart_error_handle(app_uart_evt_t * p_event)
{
    if ((p_event->evt_type == APP_UART_FIFO_ERROR) || 
        (p_event->evt_type == APP_UART_COMMUNICATION_ERROR))
    {
        // Copy parameters to static variables because parameters are not accessible in the 
        // debugger.
        static volatile app_uart_evt_t uart_event;

        uart_event.evt_type = p_event->evt_type;
        uart_event.data     = p_event->data;
        UNUSED_VARIABLE(uart_event);  
    
        for (;;)
        {
            // No implementation needed.
        }
    }
}
#endif // defined(TRACE_UART)         


/**@brief Function for configuring and setting up the SoftDevice. 
 */
static __INLINE void softdevice_setup(void)
{        
    uint32_t err_code = sd_softdevice_enable(NRF_CLOCK_LFCLKSRC_XTAL_50_PPM, 
                                             softdevice_assert_callback); 
    APP_ERROR_CHECK(err_code);

    // Configure application-specific interrupts. Set application IRQ to lowest priority and enable 
    // application IRQ (triggered from ANT protocol stack).
    err_code = sd_nvic_SetPriority(PROTOCOL_EVENT_IRQn, NRF_APP_PRIORITY_LOW);
    APP_ERROR_CHECK(err_code);
    
    err_code = sd_nvic_EnableIRQ(PROTOCOL_EVENT_IRQn);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main entry, does not return.
 */
int main(void)
{    
#if defined(TRACE_UART)    
    // Configure and make UART ready for usage.    
    const app_uart_comm_params_t comm_params =  
    {
        RX_PIN_NUMBER, 
        TX_PIN_NUMBER, 
        RTS_PIN_NUMBER, 
        CTS_PIN_NUMBER, 
        APP_UART_FLOW_CONTROL_DISABLED, 
        false, 
        UART_BAUDRATE_BAUDRATE_Baud38400
    }; 
        
    uint32_t err_code;
    APP_UART_FIFO_INIT(&comm_params, 
                       UART_RX_BUF_SIZE, 
                       UART_TX_BUF_SIZE, 
                       uart_error_handle, 
                       APP_IRQ_PRIORITY_LOW,
                       err_code);
    APP_ERROR_CHECK(err_code);
#endif // defined(TRACE_UART)     
        
    softdevice_setup();    
    
    // Run combined bicycle speed and cadence sensor main thread - does not return.    
    main_cbsc_tx_run();
    
    return 0;
}

/**
 *@}
 **/
