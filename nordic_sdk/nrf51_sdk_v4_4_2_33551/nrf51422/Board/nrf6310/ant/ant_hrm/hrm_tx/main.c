/*
This software is subject to the license described in the license.txt file included with this software distribution. 
You may not use this file except in compliance with this license. 
Copyright � Dynastream Innovations Inc. 2012
All rights reserved.
*/

/**@file
 * @brief ANT HRM TX profile device simulator main entry. 
 * Configures and starts the SoftDevice and all other necessary modules.
 * This file is based on implementation originally made by Dynastream Innovations Inc. - June 2012.
 *
 * @defgroup ant_hrm_tx_main ANT HRM TX example
 * @{
 * @ingroup nrf_ant_hrm
 *
 * @brief Example of ANT HRM TX profile.
 *
 * This example is a Heart Rate Monitor (HRM) transmitter simulator. 
 * It will transmit the users heart rate information in the main data page (page 4).
 *
 * Device specific information is transmitted at a slower rate in the background data pages (pages 1-3).
 * It will print out profile state information to UART when data is transmitted out of the device.
 *
 * @par HRM specific configuration options
 * 
 * The following compile time configuration options are available to suit various HRM transmitter implementations:
 * - HRMTX_NETWORK_KEY       ANT PLUS network key.
 * - ANTPLUS_NETWORK_NUMBER  Network key associated with the channel.
 * - HRMTX_ANT_CHANNEL       Channel number.
 * - HRM_DEVICE_NUMBER       Device number.
 * - HRMTX_DEVICE_TYPE       Device type.
 * - HRM_TRANSMISSION_TYPE   Transmission type.
 * - ANTPLUS_RF_FREQ         Channel radio frequency offset from 2400 MHz.
 * - HRMTX_MSG_PERIOD        Channel period in 32 kHz counts.
 *
 * @par Development phase configuration options
 *
 *  The HRM transmitter simulator will trace out various types of state information to the UART depending on the 
 * selected compile time configuration options. 
 *
 * The following compile time configuration options are available to assist in the development phase of the 
 * HRM transmitter implementation:
 * - TX_PAGE1 - output page 1 data to UART.
 * - TX_PAGE2 - output page 2 data to UART.
 * - TX_PAGE3 - output page 3 data to UART.
 * - TRACE_UART - UART output enabled.
 *
 * @note The ANT+ Network Key is available for ANT+ Adopters. Please refer to http://thisisant.com to become an ANT+ Adopter and access the key.
 */

#include <stdio.h>
#include "main_hrm_tx.h"
#include "app_uart.h"
#include "ant_interface.h"
#include "nrf51.h"
#include "nrf_sdm.h"
#include "nrf_soc.h"
#include "boards.h"
#include "app_error.h"
#include "nordic_common.h"

#define UART_TX_BUF_SIZE 256u /**< UART Tx buffer size. */
#define UART_RX_BUF_SIZE 1u   /**< UART Rx buffer size. */


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
#endif // TRACE_UART  


/**@brief Function for configuring and setting up the SoftDevice. 
 */
static __INLINE void softdevice_setup(void)
{
    printf("+softdevice_setup\n");

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
  
    printf("+enter main\n");

    softdevice_setup();

    // Run HRM TX main thread - does not return.
    main_hrm_tx_run();   

    return 0;
} 

/**
 *@}
 **/
