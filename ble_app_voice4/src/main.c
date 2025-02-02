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

/** @file
 *
 * @defgroup ble_sdk_app_proximity_main main.c
 * @{
 * @ingroup ble_sdk_app_proximity_eval
 * @brief Proximity Application main file.
 *
 * This file contains is the source code for a sample proximity application using the
 * Immediate Alert, Link Loss and Tx Power services.
 *
 * This application would accept pairing requests from any peer device.
 *
 * It demonstrates the use of fast and slow advertising intervals.
 */

#include <simple_uart.h>
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_tps.h"
#include "ble_ias.h"
#include "ble_lls.h"
#include "ble_bas.h"
#include "ble_lbs.h"
#include "ble_hts.h"
#include "ble_conn_params.h"
#include "ble_eval_board_pins.h"
#include "ble_sensorsim.h"
#include "ble_stack_handler.h"
#include "app_timer.h"
#include "ble_bondmngr.h"
#include "ble_ias_c.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "app_util.h"
#include "ble_radio_notification.h"
#include "ble_flash.h"
#include "ble_debug_assert_handler.h"
#include "boards.h"
#include "nrf_delay.h"
#include "EasyVR.h"
#include "OneWire.h"

#define MAX_TEMP_SENSORS	16			// Max number of DS18B20 sensors
#define ONEWIRE_ADDR_LEN	 8			// Size of an OneWire device address

#define SIGNAL_ALERT_BUTTON               EVAL_BOARD_BUTTON_0                               /**< Button used for send or cancel High Alert to the peer. */
#define STOP_ALERTING_BUTTON              EVAL_BOARD_BUTTON_1                               /**< Button used for clearing the Alert LED that may be blinking or turned ON because of alerts from the master. */
#define BONDMNGR_DELETE_BUTTON_PIN_NO     EVAL_BOARD_BUTTON_1                               /**< Button used for deleting all bonded masters during startup. */

#define ALERT_PIN_NO                      EVAL_BOARD_LED_1                                  /**< Pin used as LED to signal an alert. */

// Keypad lights and buttons
#define KEYPAD_GREEN_LED  12
#define KEYPAD_RED_LED    13
#define KEYPAD_YELLOW_LED 14
#define KEYPAD_BLUE_LED   15
#define KEYPAD_GREEN_KEY   1
#define KEYPAD_RED_KEY     0
#define KEYPAD_YELLOW_KEY  3
#define KEYPAD_BLUE_KEY    2

#define DEVICE_NAME                       "DataV_HMI2"                                     /**< Name of device. Will be included in the advertising data. */
#define APP_ADV_INTERVAL_FAST             0x0028                                            /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 25 ms.). */
#define APP_ADV_INTERVAL_SLOW             0x0C80                                            /**< Slow advertising interval (in units of 0.625 ms. This value corresponds to 2 seconds). */
#define APP_SLOW_ADV_TIMEOUT              180                                               /**< The duration of the slow advertising period (in seconds). */
#define APP_FAST_ADV_TIMEOUT              30                                                /**< The duration of the fast advertising period (in seconds). */

#define APP_TIMER_PRESCALER               0                                                 /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS              6                                                 /**< Maximum number of simultaneously created timers. 1 for Battery measurement, 2 for flashing Advertising LED and Alert LED, 1 for connection parameters module, 1 for button polling timer needed by the app_button module,  */
#define APP_TIMER_OP_QUEUE_SIZE           7                                                 /**< Size of timer operation queues. */

#define TEMP_TYPE_AS_CHARACTERISTIC          0                                          /**< Determines if temperature type is given as characteristic (1) or as a field of measurement (0). */

#define MIN_CELCIUS_DEGREES                  3688                                       /**< Minimum temperature in celcius for use in the simulated measurement function (multiplied by 100 to avoid floating point arithmetic). */
#define MAX_CELCIUS_DEGRESS                  3972                                       /**< Maximum temperature in celcius for use in the simulated measurement function (multiplied by 100 to avoid floating point arithmetic). */
#define CELCIUS_DEGREES_INCREMENT            36                                         /**< Value by which temperature is incremented/decremented for each call to the simulated measurement function (multiplied by 100 to avoid floating point arithmetic). */



#define BATTERY_LEVEL_MEAS_INTERVAL       APP_TIMER_TICKS(120000, APP_TIMER_PRESCALER)      /**< Battery level measurement interval (ticks). This value corresponds to 120 seconds. */
#define TEMP_MEAS_INTERVAL				  APP_TIMER_TICKS(5 * 1000, APP_TIMER_PRESCALER)

#define ADV_LED_ON_TIME                   APP_TIMER_TICKS(100, APP_TIMER_PRESCALER)         /**< Advertisement LED ON period when in blinking state. */
#define ADV_LED_OFF_TIME                  APP_TIMER_TICKS(900, APP_TIMER_PRESCALER)         /**< Advertisement LED OFF period when in blinking state. */

#define MILD_ALERT_LED_ON_TIME            APP_TIMER_TICKS(100, APP_TIMER_PRESCALER)         /**< Alert LED ON period when in blinking state. */
#define MILD_ALERT_LED_OFF_TIME           APP_TIMER_TICKS(900, APP_TIMER_PRESCALER)         /**< Alert LED OFF period when in blinking state. */

#define MIN_CONN_INTERVAL                 MSEC_TO_UNITS(500, UNIT_1_25_MS)                  /**< Minimum acceptable connection interval (0.5 seconds).  */
#define MAX_CONN_INTERVAL                 MSEC_TO_UNITS(1000, UNIT_1_25_MS)                 /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                     0                                                 /**< Slave latency. */
#define CONN_SUP_TIMEOUT                  MSEC_TO_UNITS(4000, UNIT_10_MS)                   /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(5 * 1000, APP_TIMER_PRESCALER)    /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY     APP_TIMER_TICKS(5 * 1000, APP_TIMER_PRESCALER)    /**< Time between each call to sd_ble_gap_conn_param_update after the first (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT      3                                                 /**< Number of attempts before giving up the connection parameter negotiation. */

#define APP_GPIOTE_MAX_USERS              1                                                 /**< Maximum number of users of the GPIOTE handler. */

#define BUTTON_DETECTION_DELAY            APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)          /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define SEC_PARAM_TIMEOUT                 30                                                /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                    1                                                 /**< Perform bonding. */
#define SEC_PARAM_MITM                    0                                                 /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES         BLE_GAP_IO_CAPS_NONE                              /**< No I/O capabilities. */
#define SEC_PARAM_OOB                     0                                                 /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE            7                                                 /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE            16                                                /**< Maximum encryption key size. */

#define INITIAL_LLS_ALERT_LEVEL           BLE_CHAR_ALERT_LEVEL_NO_ALERT                     /**< Initial value for the Alert Level characteristic in the Link Loss service. */
#define TX_POWER_LEVEL                    (-8)                                              /**< TX Power Level value. This will be set both in the TX Power service, in the advertising data, and also used to set the radio transmit power. */

#define FLASH_PAGE_SYS_ATTR              (BLE_FLASH_PAGE_END - 3)                           /**< Flash page used for bond manager system attribute information. */
#define FLASH_PAGE_BOND                  (BLE_FLASH_PAGE_END - 1)                           /**< Flash page used for bond manager bonding information. */

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS     1200                                              /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION      3                                                 /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS    270                                               /**< Typical forward voltage drop of the diode (Part no: SD103ATW-7-F) that is connected in series with the voltage supply. This is the voltage drop when the forward current is 1mA. Source: Data sheet of 'SURFACE MOUNT SCHOTTKY BARRIER DIODE ARRAY' available at www.diodes.com. */

#define DEAD_BEEF                         0xDEADBEEF                                        /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

/**@brief Macro to convert the result of ADC conversion in millivolts.
 *
 * @param[in]  ADC_VALUE   ADC result.
 * @retval     Result converted to millivolts.
 */
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / 255) * ADC_PRE_SCALING_COMPENSATION)

/**@brief Advertisement states. */
typedef enum
{
    BLE_NO_ADV,                                                                             /**< No advertising running. */
    BLE_FAST_ADV_WHITELIST,                                                                 /**< Advertising with whitelist. */
    BLE_FAST_ADV,                                                                           /**< Fast advertising running. */
    BLE_SLOW_ADV,                                                                           /**< Slow advertising running. */
    BLE_SLEEP                                                                               /**< Go to system-off. */
} ble_advertising_mode_t;

static ble_tps_t                          m_tps;                                            /**< Structure used to identify the TX Power service. */
static ble_ias_t                          m_ias;                                            /**< Structure used to identify the Immediate Alert service. */
static ble_lls_t                          m_lls;                                            /**< Structure used to identify the Link Loss service. */
static ble_lbs_t						  m_lbs;											// LED Service

static bool                               m_is_link_loss_alerting;                          /**< Variable to indicate if a link loss has been detected. */
static bool                               m_is_alert_led_blinking;                          /**< Variable to indicate if the alert LED is blinking (indicating a mild alert). */
static bool                               m_is_adv_led_blinking;                            /**< Variable to indicate if the advertising LED is blinking. */

static ble_bas_t                          m_bas;                                            /**< Structure used to identify the battery service. */
static ble_ias_c_t                        m_ias_c;                                          /**< Structure used to identify the client to the Immediate Alert Service at peer. */
static ble_hts_t                          m_hts;                                     /**< Structure used to identify the health thermometer service. */


static ble_gap_sec_params_t               m_sec_params;                                     /**< Security requirements for this application. */
static ble_advertising_mode_t             m_advertising_mode;                               /**< Variable to keep track of when we are advertising. */

static volatile bool                      m_is_high_alert_signalled;                        /**< Variable to indicate whether or not high alert is signalled to the peer. */

static app_timer_id_t                     m_battery_timer_id;                               /**< Battery measurement timer. */
static app_timer_id_t                     m_alert_led_blink_timer_id;                       /**< Timer to realize the blinking of Alert LED. */
static app_timer_id_t                     m_adv_led_blink_timer_id;                         /**< Timer to realize the blinking of Advertisement LED. */
static app_timer_id_t					  m_temp_meas_id;				// Temp Measurement

static struct OneWire owTemp;					// One Wire Controller

static void on_ias_evt(ble_ias_t * p_ias, ble_ias_evt_t * p_evt);
static void on_lls_evt(ble_lls_t * p_lls, ble_lls_evt_t * p_evt);
static void on_ias_c_evt(ble_ias_c_t * p_lls, ble_ias_c_evt_t * p_evt);
static void on_bas_evt(ble_bas_t * p_bas, ble_bas_evt_t * p_evt);
static void advertising_init(uint8_t adv_flags);

static uint8_t ow_addrs[MAX_TEMP_SENSORS][ONEWIRE_ADDR_LEN];	// Address array
static uint8_t sensorCount;										// Count of sensors found
static bool                                  m_hts_meas_ind_conf_pending = false;       /**< Flag to keep track of when an indication confirmation is pending. */
static ble_sensorsim_cfg_t                   m_temp_celcius_sim_cfg;                    /**< Temperature simulator configuration. */
static ble_sensorsim_state_t                 m_temp_celcius_sim_state;                  /**< Temperature simulator state. */


/**
 * Init the UART for communciations to the EZ-VR module
 */
void uart_init(void) {
	simple_uart_config(RTS_PIN_NUMBER, TX_PIN_NUMBER, CTS_PIN_NUMBER, RX_PIN_NUMBER,false);
	// Change the baud rate to 9600
	NRF_UART0->BAUDRATE         = (UART_BAUDRATE_BAUDRATE_Baud9600 << UART_BAUDRATE_BAUDRATE_Pos);
}

/**@brief Function for error handling, which is called when an error has occurred. 
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name. 
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    // This call can be used for debug purposes during development of an application.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover with a reset.
    NVIC_SystemReset();
}


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling the ADC interrupt.
 * @details  This function will fetch the conversion result from the ADC, convert the value into
 *           percentage and send it to peer.
 */
void ADC_IRQHandler(void)
{
    if (NRF_ADC->EVENTS_END != 0)
    {
        uint8_t     adc_result;
        uint16_t    batt_lvl_in_milli_volts;
        uint8_t     percentage_batt_lvl;
        uint32_t    err_code;

        NRF_ADC->EVENTS_END     = 0;
        adc_result              = NRF_ADC->RESULT;
        NRF_ADC->TASKS_STOP     = 1;

        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result) +
                                  DIODE_FWD_VOLT_DROP_MILLIVOLTS;
        percentage_batt_lvl     = battery_level_in_percent(batt_lvl_in_milli_volts);

        err_code = ble_bas_battery_level_update(&m_bas, percentage_batt_lvl);
        if (
            (err_code != NRF_SUCCESS)
            &&
            (err_code != NRF_ERROR_INVALID_STATE)
            &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS)
            &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
        {
            APP_ERROR_HANDLER(err_code);
        }
    }
}


/**@brief Function for making the ADC start a battery level conversion.
 */
static void adc_start(void)
{
    uint32_t err_code;

    // Configure ADC
    NRF_ADC->INTENSET   = ADC_INTENSET_END_Msk;
    NRF_ADC->CONFIG     = (ADC_CONFIG_RES_8bit                        << ADC_CONFIG_RES_Pos)     |
                          (ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos)  |
                          (ADC_CONFIG_REFSEL_VBG                      << ADC_CONFIG_REFSEL_Pos)  |
                          (ADC_CONFIG_PSEL_Disabled                   << ADC_CONFIG_PSEL_Pos)    |
                          (ADC_CONFIG_EXTREFSEL_None                  << ADC_CONFIG_EXTREFSEL_Pos);
    NRF_ADC->EVENTS_END = 0;
    NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled;

    // Enable ADC interrupt
    err_code = sd_nvic_ClearPendingIRQ(ADC_IRQn);
    APP_ERROR_CHECK(err_code);

    err_code = sd_nvic_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
    APP_ERROR_CHECK(err_code);

    err_code = sd_nvic_EnableIRQ(ADC_IRQn);
    APP_ERROR_CHECK(err_code);

    NRF_ADC->EVENTS_END  = 0;    // Stop any running conversions.
    NRF_ADC->TASKS_START = 1;
}



/**@brief Function for starting the blinking of the Advertisement LED.
 */
static void adv_led_blink_start(void)
{
    if (!m_is_adv_led_blinking)
    {
        uint32_t             err_code;
        static volatile bool is_led_on;
        
        is_led_on = true;

        nrf_gpio_pin_set(ADVERTISING_LED_PIN_NO);
        
        is_led_on             = true;
        m_is_adv_led_blinking = true;

        err_code = app_timer_start(m_adv_led_blink_timer_id, ADV_LED_ON_TIME, (void *)&is_led_on);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for stopping the blinking of the Advertisement LED.
 */
static void adv_led_blink_stop(void)
{
    uint32_t err_code;

    err_code = app_timer_stop(m_adv_led_blink_timer_id);
    APP_ERROR_CHECK(err_code);

    m_is_adv_led_blinking = false;

    nrf_gpio_pin_clear(ADVERTISING_LED_PIN_NO);
}


/**@brief Function for starting the blinking of the Alert LED.
 */
static void alert_led_blink_start(void)
{
    if (!m_is_alert_led_blinking)
    {
        uint32_t             err_code;
        static volatile bool is_led_on;

        is_led_on = true;
        
        nrf_gpio_pin_set(ALERT_PIN_NO);
        
        m_is_alert_led_blinking = true;

        err_code = app_timer_start(m_alert_led_blink_timer_id,
                                   MILD_ALERT_LED_ON_TIME,
                                   (void *)&is_led_on);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for stopping the blinking of the Alert LED.
 */
static void alert_led_blink_stop(void)
{
    uint32_t err_code;

    err_code = app_timer_stop(m_alert_led_blink_timer_id);
    APP_ERROR_CHECK(err_code);

    m_is_alert_led_blinking = false;

    nrf_gpio_pin_clear(ALERT_PIN_NO);
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t             err_code;
    ble_gap_adv_params_t adv_params;
    ble_gap_whitelist_t  whitelist;
    
    // Initialize advertising parameters with defaults values
    memset(&adv_params, 0, sizeof(adv_params));
    
    adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    adv_params.p_peer_addr = NULL;
    adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    adv_params.p_whitelist = NULL;

    // Configure advertisement according to current advertising state
    switch (m_advertising_mode)
    {
        case BLE_NO_ADV:
            m_advertising_mode = BLE_FAST_ADV_WHITELIST;
            // fall through

        case BLE_FAST_ADV_WHITELIST:
            err_code = ble_bondmngr_whitelist_get(&whitelist);
            APP_ERROR_CHECK(err_code);

            if ((whitelist.addr_count != 0) || (whitelist.irk_count != 0))
            {
                adv_params.fp          = BLE_GAP_ADV_FP_FILTER_CONNREQ;
                adv_params.p_whitelist = &whitelist;
                
                advertising_init(BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED);
                m_advertising_mode = BLE_FAST_ADV;
            }
            else
            {
                m_advertising_mode = BLE_SLOW_ADV;
            }
            
            adv_params.interval = APP_ADV_INTERVAL_FAST;
            adv_params.timeout  = APP_FAST_ADV_TIMEOUT;
            break;
            
        case BLE_FAST_ADV:
            advertising_init(BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE);
            
            adv_params.interval = APP_ADV_INTERVAL_FAST;
            adv_params.timeout  = APP_FAST_ADV_TIMEOUT;
            m_advertising_mode  = BLE_SLOW_ADV;            
            break;
            
        case BLE_SLOW_ADV:
            advertising_init(BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE);
            
            adv_params.interval = APP_ADV_INTERVAL_SLOW;
            adv_params.timeout  = APP_SLOW_ADV_TIMEOUT;
            m_advertising_mode  = BLE_SLEEP;
            
            break;
            
        default:
            break;
    }

    // Start advertising
    err_code = sd_ble_gap_adv_start(&adv_params);
    APP_ERROR_CHECK(err_code);
    
    adv_led_blink_start();
}


/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *          This function will start the ADC.
 *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    adc_start();
}

static void temp_meas_timeout_handler(void * p_context) {
    uint8_t i, j, data[9], crc;
	UNUSED_PARAMETER(p_context);

    // Read all sensor values
    for(i= 0; i < sensorCount; i++) {
    	OneWire_reset(&owTemp);
    	OneWire_select(&owTemp, ow_addrs[i]);
    	OneWire_write(&owTemp, 0xBE, 0);
    	for(j= 0; j < 9; j++)
    		data[j]= OneWire_read(&owTemp);

		{
			static int16_t tempc, tempr;
			tempc= (data[1] << 8) | data[0];
			tempc= (tempc * 100) / 16;
			crc= OneWire_crc8(data, 8);
		}
    }
    OneWire_reset(&owTemp);
    OneWire_write(&owTemp, 0xCC, 0);
    OneWire_write(&owTemp, 0x44, 0);
}


/**@brief Function for handling the timeout that is responsible for toggling the Alert LED.
 *
 * @details This function will be called each time the timer that was started for the sake of
 *          blinking the Alert LED expires. This function toggle the state of the Alert LED.
 *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
static void mild_alert_timeout_handler(void * p_context)
{
    if (m_is_alert_led_blinking)
    {
        bool *          p_is_led_on;
        uint32_t        next_timer_interval;
        uint32_t        err_code;

        APP_ERROR_CHECK_BOOL(p_context != NULL);

        nrf_gpio_pin_toggle(ALERT_PIN_NO);

        p_is_led_on = (bool *)p_context;

        *p_is_led_on = !(*p_is_led_on);

        if(*p_is_led_on)
        {
            // The toggle operation above would have resulted in an ON state. So start a timer that
            // will expire after MILD_ALERT_LED_OFF_TIME.
            next_timer_interval = MILD_ALERT_LED_ON_TIME;
        }
        else
        {
            // The toggle operation above would have resulted in an OFF state. So start a timer that
            // will expire after MILD_ALERT_LED_OFF_TIME.
            next_timer_interval = MILD_ALERT_LED_OFF_TIME;
        }

        err_code = app_timer_start(m_alert_led_blink_timer_id,
                                   next_timer_interval,
                                   p_is_led_on);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling the timeout that is responsible for toggling the Advertisement LED.
 *
 * @details This function will be called each time the timer that was started for the sake of
 *          blinking the Advertisement LED expires. This function toggle the state of the
 *          Advertisement LED.
 *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
static void adv_led_blink_timeout_handler(void * p_context)
{
    if (m_is_adv_led_blinking)
    {
        bool *          p_is_led_on;
        uint32_t        next_timer_interval;
        uint32_t        err_code;
    
        APP_ERROR_CHECK_BOOL(p_context != NULL);

        nrf_gpio_pin_toggle(ADVERTISING_LED_PIN_NO);
        // Also blink all keyboard lights
        nrf_gpio_pin_toggle(KEYPAD_BLUE_LED);
        nrf_gpio_pin_toggle(KEYPAD_GREEN_LED);
        nrf_gpio_pin_toggle(KEYPAD_YELLOW_LED);
        nrf_gpio_pin_toggle(KEYPAD_RED_LED);


        p_is_led_on = (bool *)(p_context);

        *p_is_led_on = !(*p_is_led_on);

        if(*p_is_led_on )
        {
            // The toggle operation above would have resulted in an ON state. So start a timer that
            // will expire after ADV_LED_ON_TIME.
            next_timer_interval = ADV_LED_ON_TIME;
        }
        else
        {
            // The toggle operation above would have resulted in an OFF state. So start a timer that
            // will expire after ADV_LED_OFF_TIME.
            next_timer_interval = ADV_LED_OFF_TIME;
        }

        err_code = app_timer_start(m_adv_led_blink_timer_id,
                                   next_timer_interval,
                                   p_is_led_on );
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    GPIO_LED_CONFIG(ADVERTISING_LED_PIN_NO);
    GPIO_LED_CONFIG(ALERT_PIN_NO);
    GPIO_LED_CONFIG(KEYPAD_BLUE_LED);
    nrf_gpio_pin_set(KEYPAD_BLUE_LED);
    GPIO_LED_CONFIG(KEYPAD_GREEN_LED);
    nrf_gpio_pin_set(KEYPAD_GREEN_LED);
    GPIO_LED_CONFIG(KEYPAD_YELLOW_LED);
    nrf_gpio_pin_set(KEYPAD_YELLOW_LED);
    GPIO_LED_CONFIG(KEYPAD_RED_LED);
    nrf_gpio_pin_set(KEYPAD_RED_LED);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    uint32_t err_code;

    // Initialize timer module
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);

    // Create battery timer
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_create(&m_alert_led_blink_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                mild_alert_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_adv_led_blink_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                adv_led_blink_timeout_handler);
    APP_ERROR_CHECK(err_code);

    // Create Temp Measurement timer
    err_code = app_timer_create(&m_temp_meas_id,
                                APP_TIMER_MODE_REPEATED,
                                temp_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
    err_code= app_timer_start(m_temp_meas_id, TEMP_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function shall be used to setup all the necessary GAP (Generic Access Profile)
 *          parameters of the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_HID);
    APP_ERROR_CHECK(err_code);
    
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_tx_power_set(TX_POWER_LEVEL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 *
 * @param[in]  adv_flags  Indicates which type of advertisement to use, see @ref BLE_GAP_DISC_MODES.
 * 
 */
static void advertising_init(uint8_t adv_flags)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    int8_t        tx_power_level = TX_POWER_LEVEL;
    
    ble_uuid_t adv_uuids[] = 
    {
        {BLE_UUID_TX_POWER_SERVICE,        BLE_UUID_TYPE_BLE}, 
        {BLE_UUID_IMMEDIATE_ALERT_SERVICE, BLE_UUID_TYPE_BLE}, 
        {BLE_UUID_LINK_LOSS_SERVICE,       BLE_UUID_TYPE_BLE}
    };

    m_advertising_mode = BLE_NO_ADV;

    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));
    
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags.size              = sizeof(adv_flags);
    advdata.flags.p_data            = &adv_flags;
    advdata.p_tx_power_level        = &tx_power_level;
    advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = adv_uuids;
    
    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the TX Power Service.
 */
static void tps_init(void)
{
    uint32_t       err_code;
    ble_tps_init_t tps_init_obj;

    memset(&tps_init_obj, 0, sizeof(tps_init_obj));
    tps_init_obj.initial_tx_power_level = TX_POWER_LEVEL;
    
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&tps_init_obj.tps_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&tps_init_obj.tps_attr_md.write_perm);

    err_code = ble_tps_init(&m_tps, &tps_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Immediate Alert Service.
 */
static void ias_init(void)
{
    uint32_t       err_code;
    ble_ias_init_t ias_init_obj;

    memset(&ias_init_obj, 0, sizeof(ias_init_obj));
    ias_init_obj.evt_handler = on_ias_evt;
    
    err_code = ble_ias_init(&m_ias, &ias_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Link Loss Service.
 */
static void lls_init(void)
{
    uint32_t       err_code;
    ble_lls_init_t lls_init_obj;

    // Initialize Link Loss Service
    memset(&lls_init_obj, 0, sizeof(lls_init_obj));
    
    lls_init_obj.evt_handler         = on_lls_evt;
    lls_init_obj.error_handler       = service_error_handler;
    lls_init_obj.initial_alert_level = INITIAL_LLS_ALERT_LEVEL;
    
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&lls_init_obj.lls_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&lls_init_obj.lls_attr_md.write_perm);

    err_code = ble_lls_init(&m_lls, &lls_init_obj);
    APP_ERROR_CHECK(err_code);
    m_is_link_loss_alerting = false;
}


/**@brief Function for initializing the Battery Service.
 */
static void bas_init(void)
{
    uint32_t       err_code;
    ble_bas_init_t bas_init_obj;
    
    memset(&bas_init_obj, 0, sizeof(bas_init_obj));
    
    bas_init_obj.evt_handler          = on_bas_evt;
    bas_init_obj.support_notification = true;
    bas_init_obj.p_report_ref         = NULL;
    bas_init_obj.initial_batt_level   = 100;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init_obj.battery_level_char_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_report_read_perm);
    
    err_code = ble_bas_init(&m_bas, &bas_init_obj);
    APP_ERROR_CHECK(err_code);
}



/**@brief Function for initializing the immediate alert service client.
 * @details This will initialize the client side functionality of the Find Me profile.
 */
static void ias_client_init(void)
{
    uint32_t            err_code;
    ble_ias_c_init_t    ias_c_init_obj;

    memset(&ias_c_init_obj, 0, sizeof(ias_c_init_obj));

    m_is_high_alert_signalled = false;

    ias_c_init_obj.evt_handler    = on_ias_c_evt;
    ias_c_init_obj.error_handler  = service_error_handler;

    err_code = ble_ias_c_init(&m_ias_c, &ias_c_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**
 * Handle BLE Write to LED characterstic
 * @param p_lbs
 * @param led_state
 */
static void led_write_handler(ble_lbs_t * p_lbs, uint8_t led_state)
{
#define PROC_LED(bit, led) if( led_state & bit ) nrf_gpio_pin_set(led); else nrf_gpio_pin_clear(led);
	PROC_LED(0x01, KEYPAD_BLUE_LED);
	PROC_LED(0x02, KEYPAD_GREEN_LED);
	PROC_LED(0x04, KEYPAD_RED_LED);
	PROC_LED(0x08, KEYPAD_YELLOW_LED);
//    if (led_state)
//    {
//        nrf_gpio_pin_set(ALERT_PIN_NO);
//    }
//    else
//    {
//        nrf_gpio_pin_clear(ALERT_PIN_NO);
//    }
}


/**
 *
 * Init the LED Service
 */
void lbs_init(void)
{
    uint32_t err_code;
    ble_lbs_init_t init;

    init.led_write_handler = led_write_handler;

    err_code = ble_lbs_init(&m_lbs, &init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for populating simulated health thermometer measurement.
 */
static void hts_sim_measurement(ble_hts_meas_t * p_meas)
{
    static ble_date_time_t time_stamp = { 2012, 12, 5, 11, 50, 0 };

    uint32_t celciusX100;

    p_meas->temp_in_fahr_units = false;
    p_meas->time_stamp_present = true;
    p_meas->temp_type_present  = TEMP_TYPE_AS_CHARACTERISTIC ? false : true;

    celciusX100 = ble_sensorsim_measure(&m_temp_celcius_sim_state, &m_temp_celcius_sim_cfg);

    p_meas->temp_in_celcius.exponent = -2;
    p_meas->temp_in_celcius.mantissa = celciusX100;
    p_meas->temp_in_fahr.exponent    = -2;
    p_meas->temp_in_fahr.mantissa    = (32 * 100) + ((celciusX100 * 9) / 5);
    p_meas->time_stamp               = time_stamp;
    p_meas->temp_type                = BLE_HTS_TEMP_TYPE_FINGER;

    // update simulated time stamp
    time_stamp.seconds += 27;
    if (time_stamp.seconds > 59)
    {
        time_stamp.seconds -= 60;
        time_stamp.minutes++;
        if (time_stamp.minutes > 59)
        {
            time_stamp.minutes = 0;
        }
    }
}




/**@brief Function for simulating and sending one Temperature Measurement.
 */
static void temperature_measurement_send(void)
{
    ble_hts_meas_t simulated_meas;
    uint32_t       err_code;

    if (!m_hts_meas_ind_conf_pending)
    {
        hts_sim_measurement(&simulated_meas);

        err_code = ble_hts_measurement_send(&m_hts, &simulated_meas);
        switch (err_code)
        {
            case NRF_SUCCESS:
                // Measurement was successfully sent, wait for confirmation.
                m_hts_meas_ind_conf_pending = true;
                break;

            case NRF_ERROR_INVALID_STATE:
                // Ignore error.
                break;

            default:
                APP_ERROR_HANDLER(err_code);
        }
    }
}


/**@brief Function for handling the Health Thermometer Service events.
 *
 * @details This function will be called for all Health Thermometer Service events which are passed
 *          to the application.
 *
 * @param[in]   p_hts   Health Thermometer Service stucture.
 * @param[in]   p_evt   Event received from the Health Thermometer Service.
 */
static void on_hts_evt(ble_hts_t * p_hts, ble_hts_evt_t *p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_HTS_EVT_INDICATION_ENABLED:
            // Indication has been enabled, send a single temperature measurement
            temperature_measurement_send();
            break;

        case BLE_HTS_EVT_INDICATION_CONFIRMED:
            m_hts_meas_ind_conf_pending = false;
            break;

        default:
            break;
    }
}




static void hts_init(void) {
	uint32_t         err_code;
	ble_hts_init_t   hts_init;

	// Initialize Health Thermometer Service
	memset(&hts_init, 0, sizeof(hts_init));

	hts_init.evt_handler                 = on_hts_evt;
	hts_init.temp_type_as_characteristic = TEMP_TYPE_AS_CHARACTERISTIC;
	hts_init.temp_type                   = BLE_HTS_TEMP_TYPE_EAR;

	// Here the sec level for the Health Thermometer Service can be changed/increased.
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hts_init.hts_meas_attr_md.cccd_write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hts_init.hts_meas_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hts_init.hts_meas_attr_md.write_perm);

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&hts_init.hts_temp_type_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hts_init.hts_temp_type_attr_md.write_perm);

	err_code = ble_hts_init(&m_hts, &hts_init);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the services that will be used by the application.
 */
static void services_init(void)
{
    tps_init();
    ias_init();
    lls_init();
    bas_init();
    lbs_init();
    ias_client_init();
    hts_init();
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
    
    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = true;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;
    
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Signals alert event from Immediate Alert or Link Loss services.
 *
 * @param[in]   alert_level  Requested alert level.
 */
static void alert_signal(uint8_t alert_level)
{
    switch (alert_level)
    {
        case BLE_CHAR_ALERT_LEVEL_NO_ALERT:
            m_is_link_loss_alerting = false;
            alert_led_blink_stop();
            break;

        case BLE_CHAR_ALERT_LEVEL_MILD_ALERT:
            alert_led_blink_start();
            break;

        case BLE_CHAR_ALERT_LEVEL_HIGH_ALERT:
            alert_led_blink_stop();
            nrf_gpio_pin_set(ALERT_PIN_NO);
            break;
            
        default:
            break;
    }
}


/**@brief Function for handling Immediate Alert events.
 *
 * @details This function will be called for all Immediate Alert events which are passed to the
 *          application.
 *
 * @param[in]   p_ias  Immediate Alert structure.
 * @param[in]   p_evt  Event received from the Immediate Alert service.
 */
static void on_ias_evt(ble_ias_t * p_ias, ble_ias_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_IAS_EVT_ALERT_LEVEL_UPDATED:
            alert_signal(p_evt->params.alert_level);
            break;
            
        default:
            break;
    }
}


/**@brief Function for handling Link Loss events.
 *
 * @details This function will be called for all Link Loss events which are passed to the
 *          application.
 *
 * @param[in]   p_lls  Link Loss stucture.
 * @param[in]   p_evt  Event received from the Link Loss service.
 */
static void on_lls_evt(ble_lls_t * p_lls, ble_lls_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_LLS_EVT_LINK_LOSS_ALERT:
            m_is_link_loss_alerting = true;
            alert_signal(p_evt->params.alert_level);
            break;
            
        default:
            break;
    }
}


/**@brief Function for initializing the security parameters.
 */
static void sec_params_init(void)
{
    m_sec_params.timeout      = SEC_PARAM_TIMEOUT;
    m_sec_params.bond         = SEC_PARAM_BOND;
    m_sec_params.mitm         = SEC_PARAM_MITM;
    m_sec_params.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    m_sec_params.oob          = SEC_PARAM_OOB;
    m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
}


/**@brief Function for handling IAS Client events.
 *
 * @details This function will be called for all IAS Client events which are passed to the
 *          application.
 *
 * @param[in]   p_ias_c  IAS Client stucture.
 * @param[in]   p_evt    Event received.
 */
static void on_ias_c_evt(ble_ias_c_t * p_ias_c, ble_ias_c_evt_t * p_evt)
{
    uint32_t err_code = NRF_SUCCESS;

    switch (p_evt->evt_type)
    {
        case BLE_IAS_C_EVT_SRV_DISCOVERED:
            // IAS is found on peer. The Find Me Locator functionality of this app will work.
            break;

        case BLE_IAS_C_EVT_SRV_NOT_FOUND:
            // IAS is not found on peer. The Find Me Locator functionality of this app will NOT work.
            break;

        case BLE_IAS_C_EVT_DISCONN_COMPLETE:
            // Stop detecting button presses when not connected
            err_code = app_button_disable();
            break;

        default:
            break;
    }

    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Battery Service events.
 *
 * @details This function will be called for all Battery Service events which are passed to the
 |          application.
 *
 * @param[in]   p_bas  Battery Service stucture.
 * @param[in]   p_evt  Event received from the Battery Service.
 */
static void on_bas_evt(ble_bas_t * p_bas, ble_bas_evt_t *p_evt)
{
    uint32_t err_code;

    switch (p_evt->evt_type)
    {
        case BLE_BAS_EVT_NOTIFICATION_ENABLED:
            // Start battery timer
            err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_BAS_EVT_NOTIFICATION_DISABLED:
            err_code = app_timer_stop(m_battery_timer_id);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t        err_code      = NRF_SUCCESS;
    static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            adv_led_blink_stop();

            if (m_is_link_loss_alerting)
            {
                alert_led_blink_stop();
            }
            
            m_advertising_mode = BLE_NO_ADV;
            m_conn_handle      = p_ble_evt->evt.gap_evt.conn_handle;
            
            // Start handling button presses
            err_code = app_button_enable();
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            if (!m_is_link_loss_alerting)
            {
                alert_led_blink_stop();
            }

            m_conn_handle = BLE_CONN_HANDLE_INVALID;

            // Since we are not in a connection and have not started advertising, store bonds
            err_code = ble_bondmngr_bonded_masters_store();
            APP_ERROR_CHECK(err_code);

            advertising_start();
            break;
            
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, 
                                                   BLE_GAP_SEC_STATUS_SUCCESS, 
                                                   &m_sec_params);
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT)
            { 
                if (m_advertising_mode == BLE_SLEEP)
                {
                    m_advertising_mode = BLE_NO_ADV;
                    adv_led_blink_stop();
                    alert_led_blink_stop();

                    GPIO_WAKEUP_BUTTON_CONFIG(SIGNAL_ALERT_BUTTON);
                    GPIO_WAKEUP_BUTTON_CONFIG(BONDMNGR_DELETE_BUTTON_PIN_NO);
                                    
                    // Go to system-off mode
                    // (this function will not return; wakeup will cause a reset)
                    err_code = sd_power_system_off();    
                }
                else
                {
                    advertising_start();
                }
            }
            break;

        case BLE_GATTC_EVT_TIMEOUT:
        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server and Client timeout events.
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            break;

        default:
            break;
    }

    APP_ERROR_CHECK(err_code);
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_bondmngr_on_ble_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_ias_on_ble_evt(&m_ias, p_ble_evt);
    ble_lls_on_ble_evt(&m_lls, p_ble_evt);
    ble_bas_on_ble_evt(&m_bas, p_ble_evt);
    ble_ias_c_on_ble_evt(&m_ias_c, p_ble_evt);
    ble_lbs_on_ble_evt(&m_lbs, p_ble_evt);
    on_ble_evt(p_ble_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    BLE_STACK_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM,
                           BLE_L2CAP_MTU_DEF,
                           ble_evt_dispatch,
                           false);
}


/**@brief Function for handling a Bond Manager error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void bond_manager_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for the Bond Manager initialization.
 */
static void bond_manager_init(void)
{
    uint32_t            err_code;
    ble_bondmngr_init_t bond_init_data;
    bool                bonds_delete;

    // Clear all bonded masters if the Bonds Delete button is pushed
    err_code = app_button_is_pushed(BONDMNGR_DELETE_BUTTON_PIN_NO, &bonds_delete);
    APP_ERROR_CHECK(err_code);
    
    // Initialize the Bond Manager
    bond_init_data.flash_page_num_bond     = FLASH_PAGE_BOND;
    bond_init_data.flash_page_num_sys_attr = FLASH_PAGE_SYS_ATTR;
    bond_init_data.evt_handler             = NULL;
    bond_init_data.error_handler           = bond_manager_error_handler;
    bond_init_data.bonds_delete            = bonds_delete;

    err_code = ble_bondmngr_init(&bond_init_data);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Radio Notification event.
 */
static void radio_notification_init(void)
{
    uint32_t err_code;

    err_code = ble_radio_notification_init(NRF_APP_PRIORITY_HIGH,
                                           NRF_RADIO_NOTIFICATION_DISTANCE_4560US,
                                           ble_flash_on_radio_active_evt);
    APP_ERROR_CHECK(err_code);
}


static void keypad_button_event_hanlder(uint8_t button, uint8_t led) {
	// Send the notification
	ble_lbs_on_button_change(&m_lbs, button);
	nrf_gpio_pin_set(led);
	EasyVR_playPhoneTone(button, 5);
	nrf_gpio_pin_clear(led);
}

/**@brief Function for handling button events.
 *
 * @param[in]   pin_no   The pin number of the button pressed.
 */
static void button_event_handler(uint8_t pin_no)
{
    uint32_t err_code;

    switch (pin_no)
    {
    case KEYPAD_BLUE_KEY:
    	keypad_button_event_hanlder(pin_no, KEYPAD_BLUE_LED);
    	break;
    case KEYPAD_RED_KEY:
    	keypad_button_event_hanlder(pin_no, KEYPAD_RED_LED);
    	break;
    case KEYPAD_GREEN_KEY:
    	keypad_button_event_hanlder(pin_no, KEYPAD_GREEN_LED);
    	break;
    case KEYPAD_YELLOW_KEY:
    	keypad_button_event_hanlder(pin_no, KEYPAD_YELLOW_LED);
    	break;

        case SIGNAL_ALERT_BUTTON:
            if (!m_is_high_alert_signalled)
            {
                err_code = ble_ias_c_send_alert_level(&m_ias_c, BLE_CHAR_ALERT_LEVEL_HIGH_ALERT);
            }
            else
            {
                err_code = ble_ias_c_send_alert_level(&m_ias_c, BLE_CHAR_ALERT_LEVEL_NO_ALERT);
            }

            if (err_code == NRF_SUCCESS)
            {
                m_is_high_alert_signalled = !m_is_high_alert_signalled;
            }
            else if (
                (err_code != BLE_ERROR_NO_TX_BUFFERS)
                &&
                (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
                &&
                (err_code != NRF_ERROR_NOT_FOUND)
            )
            {
                APP_ERROR_HANDLER(err_code);
            }
            break;

        case STOP_ALERTING_BUTTON:
            alert_led_blink_stop();
            break;

        default:
            APP_ERROR_HANDLER(pin_no);
    }
}


/**@brief Function for initializing the GPIOTE handler module.
 */
static void gpiote_init(void)
{
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}


/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
    static app_button_cfg_t buttons[] =
    {
        {KEYPAD_BLUE_KEY,  false, NRF_GPIO_PIN_PULLUP, button_event_handler},
        {KEYPAD_GREEN_KEY,  false, NRF_GPIO_PIN_PULLUP, button_event_handler},
        {KEYPAD_RED_KEY,  false, NRF_GPIO_PIN_PULLUP, button_event_handler},
        {KEYPAD_YELLOW_KEY,  false, NRF_GPIO_PIN_PULLUP, button_event_handler},
        {SIGNAL_ALERT_BUTTON,  false, NRF_GPIO_PIN_PULLUP, button_event_handler},
        {STOP_ALERTING_BUTTON, false, NRF_GPIO_PIN_PULLUP, button_event_handler}
    };
    
    APP_BUTTON_INIT(buttons, sizeof(buttons) / sizeof(buttons[0]), BUTTON_DETECTION_DELAY, false);
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code;
    err_code = sd_app_event_wait();
    APP_ERROR_CHECK(err_code);
}


static void locate_temp_sensors(struct OneWire *ow) {
	sensorCount= 0;
	while(OneWire_search(ow, ow_addrs[sensorCount] ))
		sensorCount++;
}


/**@brief Function for application main entry.
 */
int main(void)
{
	static uint8_t addr[8];
	static uint8_t data[9];
	static uint8_t temp;
	static uint8_t crc;
	int i;
    // Initialize
    leds_init();
    timers_init();
    gpiote_init();
    buttons_init();
    // Config Ow
    OneWire_create(&owTemp, 25);
    locate_temp_sensors(&owTemp);

//    for(;;) {
//    	i++;
//    }

	static uint8_t c;
    uart_init();
//    {
//    	EasyVR_setLanguage(ENGLISH);
//    	EasyVR_setKnob(STRICT);
//    	EasyVR_setLevel(NORMAL);
//    	EasyVR_playSound(BEEP,VOL_DOUBLE);
//    	EasyVR_setTimeout(10);
////    	EasyVR_recognizeWord(3);
////    	EasyVR_hasFinished();
//    	c= EasyVR_getWord();
//    	for(int i= 0; i < 16; i++)
//    		EasyVR_playPhoneTone(i,4);
////    	simple_uart_putstring("w@@B");
////    	c= simple_uart_get();
////    	nrf_delay_ms(100);
////    	simple_uart_putstring("w@B_");
////    	c= simple_uart_get();
////    	nrf_delay_ms(100);
////    	simple_uart_putstring("w@C_");
////    	c= simple_uart_get();
//    }


    bond_manager_init();
    ble_stack_init();



    gap_params_init();
    advertising_init(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    services_init();
    conn_params_init();
    sec_params_init();
    radio_notification_init();

    // Start execution
    advertising_start();
    
    // Enter main loop
    for (;;)
    {
        power_manage();
    }
}

/** 
 * @}
 */
