#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>

#include "twi.h"
#include "mcp23017.h"

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

void usart_send_char(char c)
{
	/* Wait for TX register to ready for another byte */
	while (!(USART0.STATUS & USART_DREIF_bm))
	{
		;
	}
	/* Send byte */
	USART0.TXDATAL = c;
}

int usart_print_char(char c, FILE *stream)
{
	usart_send_char(c);
	return 0;
}

FILE usart_stream = FDEV_SETUP_STREAM(usart_print_char, NULL, _FDEV_SETUP_WRITE);

#define USART_BAUD_RATE(BAUD_RATE) ((uint16_t)((float)F_CPU * 64 / (16 * (float)BAUD_RATE) + 0.5))

void init_usart()
{
	/* Set baud rate */
	USART0.BAUD = USART_BAUD_RATE(115200);
	/* Enable TX for USART0 */
	USART0.CTRLB |= USART_TXEN_bm;
	/* Set TX pin to output */
	PORTA.DIR |= PIN0_bm;
	/* Redirect stdout */
	stdout = &usart_stream;
}

/************************************************************************************************/

typedef struct {
	// Name for debugging
	char name[5];
	unsigned long time_pressed;
	bool pressed;
	uint8_t mask;
	PORT_t *port;
} button;

#define DEBOUNCE_TIME 5

bool button_released(button *btn, unsigned long currentTime)
{
	uint8_t value = btn->port->IN & btn->mask;

	if (btn->time_pressed == 0) {
		if (!value) {
			btn->time_pressed = currentTime;
		}
	}
	else {
		if (!btn->pressed && currentTime > btn->time_pressed + DEBOUNCE_TIME && !value) {
			btn->pressed = true;
			//printf( "%lu Button %s pressed.\r\n", currentTime, btn->name );
		}
		if (value) {
			btn->time_pressed = 0;
			if (btn->pressed) {
				btn->pressed = false;
				//printf( "%lu Button %s released.\r\n", currentTime, btn->name );
				return true; // Button was pressed, now released.
			}
		}
	}
	return false;
}

#define CLOCK_PIN_bm PIN0_bm
#define CLOCK_PORT PORTF

#define STEP_PIN_bm PIN1_bm
#define STEP_PORT PORTF

#define MODE_PIN_bm PIN2_bm
#define MODE_PORT PORTF

#define DATA_PORT PORTD
#define DATA_VPORT VPORTD

#define IORQ_PORT PORTF
#define IORQ_PIN_bm PIN3_bm

#define MAX_MESSAGE 64

#define MCP23017_ADDR (MCP23017_BASE_ADDRESS + 0x0)

bool haveRecievedIO = false;
uint8_t recievedIO = 0;

ISR(PORTF_PORT_vect) {
	uint8_t intFlags = VPORTF.INTFLAGS;
	
	if (intFlags & IORQ_PIN_bm)
	{
		haveRecievedIO = true;
		recievedIO = VPORTD.IN;
	}

	VPORTF.INTFLAGS = IORQ_PIN_bm;
}

int main(void)
{
	init_timer();
	init_usart();
	TWI_MasterInit(100000);
	sei();

	if (!mcp23017_init(MCP23017_ADDR))
		printf("Could not init 23017,\r\n");
	else
	{
		mcp23017_set_direction(MCP23017_ADDR, 0xFF00);
		mcp23017_set_pullup(MCP23017_ADDR, 0xFF00);
	}

	CLOCK_PORT.DIRSET = CLOCK_PIN_bm;
	CLOCK_PORT.OUTSET = CLOCK_PIN_bm;
	STEP_PORT.DIRCLR = STEP_PIN_bm;
	STEP_PORT.PIN1CTRL |= PORT_PULLUPEN_bm;
	MODE_PORT.DIRCLR = MODE_PIN_bm;
	MODE_PORT.PIN2CTRL |= PORT_PULLUPEN_bm;
	DATA_PORT.DIRCLR = 0xFF;
	IORQ_PORT.DIRCLR = IORQ_PIN_bm;
	IORQ_PORT.PIN3CTRL |= PORT_ISC_FALLING_gc;

	button stepButton = { "Step", 0, false, STEP_PIN_bm, &STEP_PORT };
	button modeButton = { "Mode", 0, false, MODE_PIN_bm, &MODE_PORT };

	char message[MAX_MESSAGE] = {0};
	uint8_t messagePos = 0;

	printf("\r\n\n\nBooting Z80 test clock\r\nCompiled: %s %s\r\n", __DATE__, __TIME__);

	unsigned long lastTime = 0;
	bool stepMode = true;
	bool run = false;
	uint8_t pattern = 1;

	for (;;) {
		unsigned long currentTime = millis();

		if (run && currentTime > lastTime) {
			CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;
			lastTime = currentTime + 100;
			if (stepMode)
				run = false;

			printf("Settting pattern %X ... ", pattern);
			bool ret = mcp23017_write(MCP23017_ADDR, MCP23017_GPIOA, pattern++);
			printf("got %X\r\n", ret);
			printf("Reading GPIO B ... ");
			uint8_t value = mcp23017_read(MCP23017_ADDR), MCP23017_GPIOB);
			printf("got %X\r\n", value);
		}

		if (run == false && stepMode == true && button_released(&stepButton, currentTime)) {
			lastTime = currentTime;
			run = true;
			CLOCK_PORT.OUTSET = CLOCK_PIN_bm;
		}

		if (button_released(&modeButton, currentTime)) {
			stepMode = !stepMode;
			if (stepMode) {
				CLOCK_PORT.OUTCLR = CLOCK_PIN_bm;
			}
			else {
				run = true;
			}
		}

		if (haveRecievedIO) {
			message[messagePos++] = recievedIO;
			message[messagePos] = 0;
			
			if (messagePos > MAX_MESSAGE)
				messagePos = 0;

			haveRecievedIO = false;
		}

		(void)message;
		/*
		uint8_t data = DATA_VPORT.IN;

		printf("%10lu - %4s %7s %02x %c %-100s\r",
				currentTime, 
				(stepMode) ? "Step" : "Run",
				(run) ? "Running" : "",
				data,
				(data > 32 && data < 128) ? data : ' ',
				message
				);
		*/
	}

	return 0;
}
