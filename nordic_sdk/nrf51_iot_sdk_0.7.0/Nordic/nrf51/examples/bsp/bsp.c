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

#include "bsp.h"
#include <stddef.h>
#include <stdio.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_error.h"

#ifndef BSP_SIMPLE
#include "app_timer.h"
#include "app_gpiote.h"
#include "app_button.h"
#endif // BSP_SIMPLE

#define ADVERTISING_LED_ON_INTERVAL            200
#define ADVERTISING_LED_OFF_INTERVAL           1800

#define ADVERTISING_DIRECTED_LED_ON_INTERVAL   200
#define ADVERTISING_DIRECTED_LED_OFF_INTERVAL  200

#define ADVERTISING_WHITELIST_LED_ON_INTERVAL  200
#define ADVERTISING_WHITELIST_LED_OFF_INTERVAL 800

#define ADVERTISING_SLOW_LED_ON_INTERVAL       400
#define ADVERTISING_SLOW_LED_OFF_INTERVAL      4000

#define BONDING_INTERVAL                       100

#define SENT_OK_INTERVAL                       100
#define SEND_ERROR_INTERVAL                    500

#define RCV_OK_INTERVAL                        100
#define RCV_ERROR_INTERVAL                     500

#define ALERT_INTERVAL                         200

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
static bsp_indication_t m_stable_state        = BSP_INDICATE_IDLE;
static uint32_t         m_app_ticks_per_100ms = 0;
static uint32_t         m_indication_type     = 0;
static app_timer_id_t   m_leds_timer_id;
static app_timer_id_t   m_alert_timer_id;
#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

#if BUTTONS_NUMBER > 0
#ifndef BSP_SIMPLE
static bsp_event_callback_t m_registered_callback         = NULL;
static bsp_event_t          m_events_list[BUTTONS_NUMBER] = {BSP_EVENT_NOTHING};

static void bsp_button_event_handler(uint8_t pin_no, uint8_t button_action);
#endif // BSP_SIMPLE

static const uint32_t m_buttons_list[BUTTONS_NUMBER] = BUTTONS_LIST;

#ifndef BSP_SIMPLE
static const app_button_cfg_t app_buttons[BUTTONS_NUMBER] =
{
    #ifdef BSP_BUTTON_0
    {BSP_BUTTON_0, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BSP_BUTTON_0

    #ifdef BSP_BUTTON_1
    {BSP_BUTTON_1, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BSP_BUTTON_1

    #ifdef BSP_BUTTON_2
    {BSP_BUTTON_2, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BSP_BUTTON_2

    #ifdef BSP_BUTTON_3
    {BSP_BUTTON_3, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BSP_BUTTON_3

    #ifdef BSP_BUTTON_4
    {BSP_BUTTON_4, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BUTTON_4

    #ifdef BSP_BUTTON_5
    {BSP_BUTTON_5, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BUTTON_5

    #ifdef BSP_BUTTON_6
    {BSP_BUTTON_6, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BUTTON_6

    #ifdef BSP_BUTTON_7
    {BSP_BUTTON_7, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BUTTON_7
};
#endif // BSP_SIMPLE
#endif // BUTTONS_NUMBER > 0

#define BSP_MS_TO_TICK(MS) (m_app_ticks_per_100ms * (MS / 100))

#ifdef BSP_LED_2_MASK
#define ALERT_LED_MASK BSP_LED_2_MASK
#else
#define ALERT_LED_MASK BSP_LED_1_MASK
#endif // BSP_LED_2_MASK

uint32_t bsp_buttons_state_get(uint32_t * p_buttons_state)
{
    uint32_t result = NRF_SUCCESS;

    *p_buttons_state = 0;
#if BUTTONS_NUMBER > 0
    uint32_t buttons = ~NRF_GPIO->IN;
    uint32_t cnt;

    for (cnt = 0; cnt < BUTTONS_NUMBER; cnt++)
    {
        if (buttons & (1 << m_buttons_list[cnt]))
        {
            *p_buttons_state |= 1 << cnt;
        }
    }
#endif // BUTTONS_NUMBER > 0

    return result;
}


uint32_t bsp_button_is_pressed(uint32_t button, bool * p_state){

    uint32_t result;

#if BUTTONS_NUMBER > 0
    if(button < BUTTONS_NUMBER)
        {
            uint32_t buttons = ~NRF_GPIO->IN;
            result = NRF_SUCCESS;
            *p_state = (buttons & (1 << m_buttons_list[button])) ? true : false;
        }
        else
        {
            result = NRF_ERROR_INVALID_PARAM;
        }

#else
        result = NRF_ERROR_INVALID_PARAM;
#endif // BUTTONS_NUMBER > 0
    return result;
}


#if (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)
/**@brief Function for handling button events.
 *
 * @param[in]   pin_no          The pin number of the button pressed.
 * @param[in]   button_action   Action button.
 */
static void bsp_button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    bsp_event_t event  = BSP_EVENT_NOTHING;
    uint32_t    button = 0;

    if ((button_action == APP_BUTTON_PUSH) && (m_registered_callback != NULL))
    {
        while ((button < BUTTONS_NUMBER) && (m_buttons_list[button] != pin_no))
        {
            button++;
        }

        if (button < BUTTONS_NUMBER)
        {
            event = m_events_list[button];
        }

        if (event != BSP_EVENT_NOTHING)
        {
            m_registered_callback(event);
        }
    }
}


#endif // (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
/**@brief       Configure leds to indicate required state.
 * @param[in]   indicate   State to be indicated.
 */
static uint32_t bsp_led_indication(bsp_indication_t indicate)
{
    uint32_t err_code   = NRF_SUCCESS;
    uint32_t next_delay = 0;

    switch (indicate)
    {
        case BSP_INDICATE_IDLE:
            LEDS_OFF(LEDS_MASK & ~ALERT_LED_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_SCANNING:
        case BSP_INDICATE_ADVERTISING:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_0_MASK & ~ALERT_LED_MASK);

            // in advertising blink LED_0
            if (LED_IS_ON(BSP_LED_0_MASK))
            {
                LEDS_OFF(BSP_LED_0_MASK);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING ? ADVERTISING_LED_OFF_INTERVAL :
                             ADVERTISING_SLOW_LED_OFF_INTERVAL;
            }
            else
            {
                LEDS_ON(BSP_LED_0_MASK);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING ? ADVERTISING_LED_ON_INTERVAL :
                             ADVERTISING_SLOW_LED_ON_INTERVAL;
            }

            m_stable_state = indicate;
            err_code       = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(next_delay), NULL);
            break;

        case BSP_INDICATE_ADVERTISING_WHITELIST:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_0_MASK & ~ALERT_LED_MASK);

            // in advertising quickly blink LED_0
            if (LED_IS_ON(BSP_LED_0_MASK))
            {
                LEDS_OFF(BSP_LED_0_MASK);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_WHITELIST ?
                             ADVERTISING_WHITELIST_LED_OFF_INTERVAL :
                             ADVERTISING_SLOW_LED_OFF_INTERVAL;
            }
            else
            {
                LEDS_ON(BSP_LED_0_MASK);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_WHITELIST ?
                             ADVERTISING_WHITELIST_LED_ON_INTERVAL :
                             ADVERTISING_SLOW_LED_ON_INTERVAL;
            }
            m_stable_state = indicate;
            err_code       = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(next_delay), NULL);
            break;

        case BSP_INDICATE_ADVERTISING_SLOW:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_0_MASK & ~ALERT_LED_MASK);

            // in advertising slowly blink LED_0
            if (LED_IS_ON(BSP_LED_0_MASK))
            {
                LEDS_OFF(BSP_LED_0_MASK);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_SLOW ? ADVERTISING_SLOW_LED_OFF_INTERVAL :
                             ADVERTISING_SLOW_LED_OFF_INTERVAL;
            }
            else
            {
                LEDS_ON(BSP_LED_0_MASK);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_SLOW ? ADVERTISING_SLOW_LED_ON_INTERVAL :
                             ADVERTISING_SLOW_LED_ON_INTERVAL;
            }
            m_stable_state = indicate;
            err_code       = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(next_delay), NULL);
            break;

        case BSP_INDICATE_ADVERTISING_DIRECTED:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_0_MASK & ~ALERT_LED_MASK);

            // in advertising very quickly blink LED_0
            if (LED_IS_ON(BSP_LED_0_MASK))
            {
                LEDS_OFF(BSP_LED_0_MASK);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_DIRECTED ?
                             ADVERTISING_DIRECTED_LED_OFF_INTERVAL :
                             ADVERTISING_SLOW_LED_OFF_INTERVAL;
            }
            else
            {
                LEDS_ON(BSP_LED_0_MASK);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_DIRECTED ?
                             ADVERTISING_DIRECTED_LED_ON_INTERVAL :
                             ADVERTISING_SLOW_LED_ON_INTERVAL;
            }
            m_stable_state = indicate;
            err_code       = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(next_delay), NULL);
            break;

        case BSP_INDICATE_BONDING:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_0_MASK & ~ALERT_LED_MASK);

            // in bonding fast blink LED_0
            if (LED_IS_ON(BSP_LED_0_MASK))
            {
                LEDS_OFF(BSP_LED_0_MASK);
            }
            else
            {
                LEDS_ON(BSP_LED_0_MASK);
            }

            m_stable_state = indicate;
            err_code       =
                app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(BONDING_INTERVAL), NULL);
            break;

        case BSP_INDICATE_CONNECTED:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_0_MASK & ~ALERT_LED_MASK);
            LEDS_ON(BSP_LED_0_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_SENT_OK:
            // when sending shortly invert LED_1
            LEDS_INVERT(BSP_LED_1_MASK);
            err_code = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(SENT_OK_INTERVAL), NULL);
            break;

        case BSP_INDICATE_SEND_ERROR:
            // on receving error invert LED_1 for long time
            LEDS_INVERT(BSP_LED_1_MASK);
            err_code = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(SEND_ERROR_INTERVAL), NULL);
            break;

        case BSP_INDICATE_RCV_OK:
            // when receving shortly invert LED_1
            LEDS_INVERT(BSP_LED_1_MASK);
            err_code = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(RCV_OK_INTERVAL), NULL);
            break;

        case BSP_INDICATE_RCV_ERROR:
            // on receving error invert LED_1 for long time
            LEDS_INVERT(BSP_LED_1_MASK);
            err_code = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(RCV_ERROR_INTERVAL), NULL);
            break;

        case BSP_INDICATE_FATAL_ERROR:
            // on fatal error turn on all leds
            LEDS_ON(LEDS_MASK);
            break;

        case BSP_INDICATE_ALERT_0:
        case BSP_INDICATE_ALERT_1:
        case BSP_INDICATE_ALERT_2:
        case BSP_INDICATE_ALERT_3:
        case BSP_INDICATE_ALERT_OFF:
            err_code   = app_timer_stop(m_alert_timer_id);
            next_delay = (uint32_t)BSP_INDICATE_ALERT_OFF - (uint32_t)indicate;

            // a little trick to find out that if it did not fall through ALERT_OFF
            if (next_delay && (err_code == NRF_SUCCESS))
            {
                if (next_delay > 1)
                {
                    err_code = app_timer_start(m_alert_timer_id, BSP_MS_TO_TICK(
                                                   (next_delay * ALERT_INTERVAL)), NULL);
                }
                LEDS_ON(ALERT_LED_MASK);
            }
            else
            {
                LEDS_OFF(ALERT_LED_MASK);
            }
            break;

        case BSP_INDICATE_USER_STATE_OFF:
            LEDS_OFF(LEDS_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_USER_STATE_0:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_0_MASK);
            LEDS_ON(BSP_LED_0_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_USER_STATE_1:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_1_MASK);
            LEDS_ON(BSP_LED_1_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_USER_STATE_2:
            LEDS_OFF(LEDS_MASK & ~(BSP_LED_0_MASK | BSP_LED_1_MASK));
            LEDS_ON(BSP_LED_0_MASK | BSP_LED_1_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_USER_STATE_3:

        case BSP_INDICATE_USER_STATE_ON:
            LEDS_ON(LEDS_MASK);
            m_stable_state = indicate;
            break;

        default:
            break;
    }

    return err_code;
}


/**@brief Handle events from leds timer.
 *
 * @note Timer handler does not support returning an error code.
 * Errors from bsp_led_indication() are not propagated.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
static void leds_timer_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    if (m_indication_type & BSP_INIT_LED)
    {
        UNUSED_VARIABLE(bsp_led_indication(m_stable_state));
    }
}


/**@brief Handle events from alert timer.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
static void alert_timer_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    LEDS_INVERT(ALERT_LED_MASK);
}
#endif // #if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)


/**@brief Configure indicators to required state.
 */
uint32_t bsp_indication_set(bsp_indication_t indicate)
{
    uint32_t err_code = NRF_SUCCESS;

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

    if (m_indication_type & BSP_INIT_LED)
    {
        err_code = bsp_led_indication(indicate);
    }

#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
    return err_code;
}


uint32_t bsp_indication_text_set(bsp_indication_t indicate, char const * p_text)
{
    uint32_t err_code = bsp_indication_set(indicate);

#ifdef BSP_UART_SUPPORT
    printf("%s", p_text);
#endif // BSP_UART_SUPPORT

    return err_code;
}


uint32_t bsp_init(uint32_t type, uint32_t ticks_per_100ms, bsp_event_callback_t callback)
{
    uint32_t err_code = NRF_SUCCESS;

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
    m_app_ticks_per_100ms = ticks_per_100ms;
    m_indication_type     = type;
#else
    UNUSED_VARIABLE(ticks_per_100ms);
#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

#if (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)
    m_registered_callback = callback;

    // BSP will support buttons and generate events
    if (type & BSP_INIT_BUTTONS)
    {
        uint32_t cnt;

        for (cnt = 0; ((cnt < BUTTONS_NUMBER) && (err_code == NRF_SUCCESS)); cnt++)
        {
            err_code = bsp_event_to_button_assign(cnt, (bsp_event_t)(BSP_EVENT_KEY_0 + cnt) );
        }
        APP_BUTTON_INIT((app_button_cfg_t *)app_buttons, BUTTONS_NUMBER, ticks_per_100ms / 2,
                        false);

        if (err_code == NRF_SUCCESS)
        {
            err_code = app_button_enable();
        }
    }
#elif (BUTTONS_NUMBER > 0) && (defined BSP_SIMPLE)

    if (type & BSP_INIT_BUTTONS)
    {
        uint32_t cnt;
        uint32_t buttons[] = BUTTONS_LIST;

        for (cnt = 0; cnt < BUTTONS_NUMBER; cnt++)
        {
            nrf_gpio_cfg_input(buttons[cnt], BUTTON_PULL);
        }
    }
#endif // (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

    if (type & BSP_INIT_LED)
    {
        NRF_GPIO->DIRCLR = BUTTONS_MASK;
        LEDS_OFF(LEDS_MASK);
        NRF_GPIO->DIRSET = LEDS_MASK;
    }

    // timers module must be already initialized!
    if (err_code == NRF_SUCCESS)
    {
        err_code =
            app_timer_create(&m_leds_timer_id, APP_TIMER_MODE_SINGLE_SHOT, leds_timer_handler);
    }

    if (err_code == NRF_SUCCESS)
    {
        err_code =
            app_timer_create(&m_alert_timer_id, APP_TIMER_MODE_REPEATED, alert_timer_handler);
    }
#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

    return err_code;
}


#ifndef BSP_SIMPLE
/**@brief Assign specific event to button.
 */
uint32_t bsp_event_to_button_assign(uint32_t button, bsp_event_t event)
{
    uint32_t err_code = NRF_SUCCESS;

#if BUTTONS_NUMBER > 0

    if (button < BUTTONS_NUMBER)
    {
        m_events_list[button] = event;
    }
    else
    {
        err_code = NRF_ERROR_INVALID_PARAM;
    }
#else
    err_code = NRF_ERROR_INVALID_PARAM;
#endif // BUTTONS_NUMBER > 0

    return err_code;
}
#endif // BSP_SIMPLE


/**@brief Enable specified buttons (others are disabled).
 */
uint32_t bsp_buttons_enable(uint32_t buttons)
{
    UNUSED_PARAMETER(buttons);

#if BUTTONS_NUMBER > 0
    uint32_t button_no;
    uint32_t pin_no;

    for (button_no = 0; button_no < BUTTONS_NUMBER; button_no++)
    {
        pin_no = m_buttons_list[button_no];

        if (buttons & (1 << button_no))
        {
            NRF_GPIO->PIN_CNF[pin_no] &= ~GPIO_PIN_CNF_SENSE_Msk;
            NRF_GPIO->PIN_CNF[pin_no] |= GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos;
        }
        else
        {
            NRF_GPIO->PIN_CNF[pin_no] &= ~GPIO_PIN_CNF_SENSE_Msk;
            NRF_GPIO->PIN_CNF[pin_no] |= GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos;
        }
    }
#endif // BUTTONS_NUMBER > 0

    return NRF_SUCCESS;
}


