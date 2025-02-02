//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include <stdio.h>
#include "diag/Trace.h"
#include "stm32f4_discovery.h"

//#include "Timer.h"
#include "BlinkLed.h"
#include "Servo.h"
#include "EasyVR.h"
#include "AX12.h"



// ----------------------------------------------------------------------------
//
// STM32F4 led blink sample (trace via ITM).
//
// In debug configurations, demonstrate how to print a greeting message
// on the trace device. In release configurations the message is
// simply discarded.
//
// To demonstrate POSIX retargetting, reroute the STDOUT and STDERR to the
// trace device and display messages on both of them.
//
// Then demonstrates how to blink a led with 1Hz, using a
// continuous loop and SysTick delays.
//
// On DEBUG, the uptime in seconds is also displayed on the trace device.
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the ITM output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- Timing definitions -------------------------------------------------

// Keep the LED on for 2/3 of a second.
//#define BLINK_ON_TICKS  (TIMER_FREQUENCY_HZ * 2 / 3)
//#define BLINK_OFF_TICKS (TIMER_FREQUENCY_HZ - BLINK_ON_TICKS)

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"



#if 0
void Test_Servo() {

#define SERVO_ON_TICKS  (TIMER_FREQUENCY_HZ / 100)
#define CENTER 1.5E-3
#define RANGE 1.0E-3
#define MIN_SERVO (CENTER-(RANGE/2.0))
#define MAX_SERVO (CENTER+(RANGE/2.0))
	float res = RESOLUTION_SERVO;
	float s_min = MIN_SERVO;
	float s_max = MAX_SERVO;
	float p;

	while (1) {
		for (p = MIN_SERVO; p < MAX_SERVO; p += 4E-6) {
			Servo_Position(0, p);
			Servo_Position(1, p);
			timer_sleep(SERVO_ON_TICKS);
		}
		for (p = MAX_SERVO; p > MIN_SERVO; p += -4E-6) {
			Servo_Position(0, p);
			Servo_Position(1, p);
			timer_sleep(SERVO_ON_TICKS);
		}
	}
}
#endif



extern void initialise_monitor_handles(void);



void dbg_write_str(const char *msg)
{
#ifdef __arm__
 // Manual semi-hosting, because the GCC ARM Embedded's semihosting wasn't working.
 for (; *msg; ++msg)
 {
  // Moves a pointer to msg into r1, sets r0 to 0x03,
  // and then performs a special breakpoint that OpenOCD sees as
  // the semihosting call. r0 tells OpenOCD which semihosting
  // function we're calling. In this case WRITEC, which writes
  // a single char pointed to by r1 to the console.
  __asm__ ("mov r1,%0; mov r0,$3; BKPT 0xAB" :
                                             : "r" (msg)
                                             : "r0", "r1"
  );
 }
#else
 printf ("%s", msg);
#endif
}


int
main(int argc, char* argv[])
{

	  /* STM32F4xx HAL library initialization:
	       - Configure the Flash prefetch, Flash preread and Buffer caches
	       - Systick timer is configured by default as source of time base, but user
	             can eventually implement his proper time base source (a general purpose
	             timer for example or other time source), keeping in mind that Time base
	             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	             handled in milliseconds basis.
	       - Low Level Initialization
	     */
	  static uint32_t i;
	  HAL_Init();
//	  initialise_monitor_handles();
	  setbuf(stdout, NULL);
	  dbg_write_str("Wagstaff");
	  i= HAL_RCC_GetPCLK1Freq();
	  printf("PCLK1 Freq = %ld\n", i);
	  trace_puts("Dave\n");
	  AX12_init();

	  EasyVR_init();

		BSP_LED_Init(LED4);
		while(1) {
			BSP_LED_On(LED4);
			HAL_Delay(1000);
			BSP_LED_Off(LED4);
			HAL_Delay(1000);
//			BSP_LED_Init(LED5);
//			BSP_LED_On(LED5);
		}


	  // Perform floating point test
//	int i, j;
//	for(j= 0; j < 50; j++) {
//		for(i= 0; i < 30000; i++) {
//			float f, f2, f3;
//			f2= 1.243;
//			f3= 8.234;
//			f2+=f2/f3;
//			f2/= f3*1.2;
//		}
//	}

	// Init the I2C channel (Audio and Servo Controller)
	I2Cx_Init();

	Servo_init();
	Servo_Position(0, 1.5E-3);
	Test_Servo();
//	while(1) {
//		Servo_Position(0, 1.0E-3);
//		timer_sleep(TIMER_FREQUENCY_HZ * 2);
//		Servo_Position(0, 2.0E-3);
//		timer_sleep(TIMER_FREQUENCY_HZ * 2);
//	}


  // By customising __initialize_args() it is possible to pass arguments,
  // for example when running tests with semihosting you can pass various
  // options to the test.
  // trace_dump_args(argc, argv);

  // Send a greeting to the trace device (skipped on Release).
  trace_puts("Hello ARM World!");

  // The standard output and the standard error should be forwarded to
  // the trace device. For this to work, a redirection in _write.c is
  // required.
  trace_printf("Standard output message.");
  fprintf(stderr, "Standard error message.\n");

  // At this stage the system clock should have already been configured
  // at high speed.
  trace_printf("System clock: %uHz\n", SystemCoreClock);

  timer_start();

  blink_led_init();
  
  uint32_t seconds = 0;

  // Infinite loop
//  while (1)
//    {
//      blink_led_on();
//      timer_sleep(BLINK_ON_TICKS);
//
//      blink_led_off();
//      timer_sleep(BLINK_OFF_TICKS);
//
//      ++seconds;
//
//      // Count seconds on the trace device.
//      //trace_printf("Second %u\n", seconds);
//    }
//  // Infinite loop, never return.
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
