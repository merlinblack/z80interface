#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <stdbool.h>

#include "twi.h"
#include "mcp23017.h"
#include "timer.h"
#include "usart.h"
#include "button.h"

#define CLOCK_PIN_bm PIN0_bm
#define CLOCK_PORT PORTF

#define STEP_PIN_bm PIN1_bm
#define STEP_PORT PORTF

#define MODE_PIN_bm PIN2_bm
#define MODE_PORT PORTF

#define DATA_PORT PORTD
#define DATA_VPORT VPORTD

#define RD_PORT PORTC
#define RD_PIN_bm PIN0_bm

#define WR_PORT PORTC
#define WR_PIN_bm PIN1_bm

#define MREQ_PORT PORTC
#define MREQ_PIN_bm PIN2_bm

#define IORQ_PORT PORTC
#define IORQ_PIN_bm PIN3_bm

#define BUSREQ_PORT PORTC
#define BUSREQ_PIN_bm PIN4_bm

#define BUSACK_PORT PORTC
#define BUSACK_PIN_bm PIN5_bm

#define MAX_MESSAGE 64

#define MCP23017_ADDR (MCP23017_BASE_ADDRESS + 0x0)

bool haveRecievedIO = false;
uint8_t recievedIO = 0;

ISR(PORTC_PORT_vect) {
	uint8_t intFlags = VPORTC.INTFLAGS;
	
	if (intFlags & IORQ_PIN_bm)
	{
		haveRecievedIO = true;
		recievedIO = VPORTD.IN;
	}

	VPORTC.INTFLAGS = IORQ_PIN_bm;
}

void init_pins()
{
	CLOCK_PORT.DIRSET = CLOCK_PIN_bm;
	CLOCK_PORT.OUTSET = CLOCK_PIN_bm;
	STEP_PORT.DIRCLR = STEP_PIN_bm;
	STEP_PORT.PIN1CTRL |= PORT_PULLUPEN_bm;
	MODE_PORT.DIRCLR = MODE_PIN_bm;
	MODE_PORT.PIN2CTRL |= PORT_PULLUPEN_bm;
	DATA_PORT.DIRCLR = 0xFF;
	RD_PORT.DIRCLR = RD_PIN_bm;
	WR_PORT.DIRCLR = WR_PIN_bm;
	MREQ_PORT.DIRCLR = MREQ_PIN_bm;
	IORQ_PORT.DIRCLR = IORQ_PIN_bm;
	IORQ_PORT.PIN3CTRL |= PORT_ISC_FALLING_gc;
	BUSREQ_PORT.DIRSET = BUSREQ_PIN_bm;
	BUSREQ_PORT.OUTSET = BUSREQ_PIN_bm;		// Active low - i.e. turn it off.
	BUSACK_PORT.DIRCLR = BUSACK_PIN_bm;
}

int main(void)
{
	init_timer();
	init_usart();
	init_pins();
	TWI_MasterInit(100000);
	sei();

	if (!mcp23017_init(MCP23017_ADDR))
		printf("Could not init 23017.\r\n");
	else
	{
		mcp23017_set_direction(MCP23017_ADDR, 0xFFFF);
	}

	button stepButton = { "Step", 0, false, STEP_PIN_bm, &STEP_PORT };
	button modeButton = { "Mode", 0, false, MODE_PIN_bm, &MODE_PORT };

	char message[MAX_MESSAGE] = {0};
	uint8_t messagePos = 0;

	printf("\r\n\n\nBooting Z80 interface\r\nCompiled: %s %s\r\n", __DATE__, __TIME__);

	unsigned long lastTime = 0;
	bool stepMode = true;
	bool run = false;
	uint8_t outputCountdown = 0;

	for (;;) {
		unsigned long currentTime = millis();

		if (run && currentTime > lastTime) {
			CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;
			//lastTime = currentTime + 10;
			if (stepMode)
				run = false;
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

		// Every 256th time.
		if (!outputCountdown++) {
			uint8_t data = DATA_VPORT.IN;
			uint16_t address = mcp23017_read_both(MCP23017_ADDR);
			bool readBit = RD_PORT.IN & RD_PIN_bm;
			bool writeBit = WR_PORT.IN & WR_PIN_bm;
			bool mreqBit = MREQ_PORT.IN & MREQ_PIN_bm;
			bool iorqBit = IORQ_PORT.IN & IORQ_PIN_bm;

			char in = usart_recieve_char();
			if (in == 'A')
				printf("Got A\r\n" );

			printf("%10lu - %4s %7s %04x %02x [%c%c%c%c] '%c' %-100s\r",
					currentTime, 
					(stepMode) ? "Step" : "Run",
					(run) ? "Running" : "",
					address,
					data,
					readBit  ? ' ' : 'R',
					writeBit ? ' ' : 'W',
					mreqBit  ? ' ' : 'M',
					iorqBit  ? ' ' : 'I',
					(data > 32 && data < 128) ? data : ' ',
					message
					);
			//unsigned long delay = millis()+1000; while(millis()<delay);
		}
	}

	return 0;
}
