#include "timer.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define TIME_TRACKING_TIMER_COUNT (F_CPU / 1000) // Should correspond to exactly 1 ms, i.e. millis()
volatile unsigned long timer_millis = 0;

ISR(TCB3_INT_vect)
{
	timer_millis++;

	/** clear flag **/
	TCB3.INTFLAGS = TCB_CAPT_bm;
}

void init_timer()
{
	_PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0x00);
	TCB3.CTRLB		 = TCB_CNTMODE_INT_gc;
	TCB3.CCMP			= TIME_TRACKING_TIMER_COUNT - 1;
	TCB3.INTCTRL	|= TCB_CAPT_bm;
	TCB3.CTRLA		 = TCB_CLKSEL_CLKDIV1_gc;
	TCB3.CTRLA		|= TCB_ENABLE_bm;
}

unsigned long millis()
{
	unsigned long m;
	uint8_t oldSREG = SREG;

	// copy millis into 'm' atomically by disabling interrupts
	cli();
	m = timer_millis;
	SREG = oldSREG;

	return m;
}

