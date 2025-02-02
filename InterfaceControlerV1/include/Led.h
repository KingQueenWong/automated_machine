//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

#ifndef Led_H_
#define Led_H_

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

// ----- LED definitions ------------------------------------------------------

// Adjust these definitions for your own board.

#if defined(BOARD_OLIMEX_STM32_E407)

// STM32-E407 definitions (the GREEN led, C13, active low)

// Port numbers: 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G, ...
#define BLINK_PORT_NUMBER               (2)
#define BLINK_PIN_NUMBER                (13)
#define BLINK_ACTIVE_LOW                (1)

#else

// STM32F4DISCOVERY definitions (the GREEN led, D12, active high)
// (SEGGER J-Link device name: STM32F407VG).

//#define BLINK_PORT_NUMBER               (3)
//#define BLINK_PIN_NUMBER                (12)
//#define BLINK_ACTIVE_LOW                (0)

#endif

#define BLINK_GPIOx(_N)                 ((GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE-GPIOA_BASE)*(_N)))
#define BLINK_PIN_MASK(_N)              (1 << (_N))
#define BLINK_RCC_MASKx(_N)             (RCC_AHB1ENR_GPIOAEN << (_N))

// ----------------------------------------------------------------------------

class Led
{
private:
//	GPIO_TypeDef *_pPort;
	uint8_t _port;
	uint8_t _pin;

public:
	enum Ports  {A=0, B=1, C=2, D=3, E=4, F=5, G=6, H=7 };
  Led(uint8_t port, uint8_t pin);

  void
  powerUp();

  inline void
  __attribute__((always_inline))
  turnOn()
  {
    HAL_GPIO_WritePin(BLINK_GPIOx(_port),
        BLINK_PIN_MASK(_pin), GPIO_PIN_SET);
  }

  inline void
  __attribute__((always_inline))
  turnOff()
  {
    HAL_GPIO_WritePin(BLINK_GPIOx(_port),
        BLINK_PIN_MASK(_pin), GPIO_PIN_RESET);
  }
};

// ----------------------------------------------------------------------------

#endif // Led_H_
