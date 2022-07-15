#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "twi.h"
#include "mcp23017.h"
#include "timer.h"
#include "usart.h"
#include "button.h"

#define MAX_COMMAND_LEN 32

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

void delay(uint16_t delay)
{
	unsigned long end = millis() + delay;
	while (millis() < end);
}

void waitForBus()
{
	BUSREQ_PORT.OUTCLR = BUSREQ_PIN_bm;
	while( BUSACK_PORT.IN & BUSACK_PIN_bm)
		CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;
}

void takeBus()
{
	waitForBus();

	/* init bus */
	mcp23017_set_direction(MCP23017_ADDR, 0x0000);
	MREQ_PORT.DIRSET = MREQ_PIN_bm;
	IORQ_PORT.DIRSET = IORQ_PIN_bm;
	WR_PORT.DIRSET = WR_PIN_bm;
	RD_PORT.DIRSET = RD_PIN_bm;
	/* active low - make them high */
	MREQ_PORT.OUTSET = MREQ_PIN_bm;
	IORQ_PORT.OUTSET = IORQ_PIN_bm;
	WR_PORT.OUTSET = WR_PIN_bm;
	RD_PORT.OUTSET = RD_PIN_bm;
}

void releaseBus()
{
	WR_PORT.DIRCLR = WR_PIN_bm;
	RD_PORT.DIRCLR = RD_PIN_bm;
	MREQ_PORT.DIRCLR = MREQ_PIN_bm;
	IORQ_PORT.DIRCLR = IORQ_PIN_bm;
	DATA_PORT.DIRCLR = 0xff;
	mcp23017_set_direction(MCP23017_ADDR, 0xFFFF);

	BUSREQ_PORT.OUTSET = BUSREQ_PIN_bm;
}

uint8_t readMemory(uint16_t address)
{
	DATA_PORT.DIRCLR = 0xFF;
	mcp23017_write_both(MCP23017_ADDR, address);
	MREQ_PORT.OUTCLR = MREQ_PIN_bm;
	RD_PORT.OUTCLR = RD_PIN_bm;

	_delay_us(1);

	uint8_t data = DATA_VPORT.IN;

	RD_PORT.OUTSET = RD_PIN_bm;
	MREQ_PORT.OUTSET = MREQ_PIN_bm;

	return data;
}

void writeMemory(uint16_t address, uint8_t data)
{
	DATA_PORT.DIRSET = 0xFF;
	mcp23017_write_both(MCP23017_ADDR, address);
	MREQ_PORT.OUTCLR = MREQ_PIN_bm;

	DATA_VPORT.OUT = data;

	WR_PORT.OUTCLR = WR_PIN_bm;
	_delay_us(1);
	WR_PORT.OUTSET = WR_PIN_bm;

	MREQ_PORT.OUTSET = MREQ_PIN_bm;
}

char command[MAX_COMMAND_LEN];
uint8_t commandPos = 0;
bool stepMode = true;
bool run = false;
bool display = false;

uint8_t hexToInt2(const char *ptr)
{
  char buffer[] = {*ptr++, *ptr, 0};
  return (uint8_t)strtol(buffer, NULL, 16);
}

uint16_t hexToInt4(const char *ptr)
{
  char buffer[] = {*ptr++, *ptr++, *ptr++, *ptr, 0};
  return (uint16_t)strtol(buffer, NULL, 16);
}

void dumpMemory(char *parameters)
{
	static uint16_t address = 0x8000;
	uint16_t len = 512;
	uint16_t pos = 0;
	uint8_t buffer[32];

	while(!isalnum(*parameters) && *parameters)
		parameters++;

	if (strlen(parameters) == 4)
		address = hexToInt4(parameters);

	takeBus();

	printf( "     | " );
	for (uint8_t byte = 0; byte < 32; byte++)
	{
		printf( "%02X ", byte );
	}
	printf( "\r\n" );

	while (pos < len)
	{
		printf( "%04X | ", address + pos);
		for (uint8_t byte = 0; byte < 32; byte++)
		{
			buffer[byte] = readMemory(address + pos + byte);
			printf( "%02X ", buffer[byte]);
		}
		printf( " | " );
		for (uint8_t byte = 0; byte < 32; byte++)
		{
			printf( "%c", isgraph(buffer[byte]) ? buffer[byte] : '.' );
		}
		printf("\r\n");
		pos += 32;
	}

	releaseBus();

	address += pos;
}

void writeBuffer( uint16_t address, uint8_t *buffer, uint8_t len )
{
	for (uint8_t pos = 0; pos < len; pos++, address++, buffer++)
	{
		writeMemory(address, *buffer);
	}
}

char getch(uint16_t timeLimit)
{
	unsigned long limit = millis() + timeLimit;
	char in = 0;

	while (!in && millis() < limit)
		in = usart_recieve_char();

	return in;
}

void loadMemory(char *parameters)
{
	uint16_t base_address = 0x8000;
	uint8_t byteBuffer[64];
	bool finished = false;
	char in;
	char inputBuffer[8];

	while(!isalnum(*parameters) && *parameters)
		parameters++;

	if (strlen(parameters) == 4)
		base_address = hexToInt4(parameters);

	printf("Loading hex file starting at address: %04X\r\n", base_address);

	takeBus();

	while(!finished)
	{
		// Process one line.
		in = getch(1000);

		if (!in)
		{
			printf("ERROR: Took too long to send hex file.\r\n");
			finished = true;
			continue;
		}

		if (in != ':')
		{
			printf("ERROR: bad format for hex file. Got %X instead of 3A.\r\n", in);
			finished = true;
			continue;
		}

		for (uint8_t i = 0; i < 8; i++)
		{
			in = getch(10);

			if (!in)
			{
				printf("ERROR: hex file taking too long.\r\n");
				finished = true;
				continue;
			}
			inputBuffer[i] = in;
		}

		uint8_t len = hexToInt2(&inputBuffer[0]);
		uint16_t offset = hexToInt4(&inputBuffer[2]);
		uint8_t type = hexToInt2(&inputBuffer[6]);

		if (type == 0)
		{
			printf("Data record - len: %02X offset: %04X\r\n", len, offset );
			uint16_t check = len + (offset & 0xff) + (offset>>8);
			for (uint8_t i = 0; i < len; i++)
			{
				inputBuffer[0] = getch(10);
				inputBuffer[1] = getch(10);
				byteBuffer[i] = hexToInt2(inputBuffer);
				printf("%02X ", byteBuffer[i]);
				check += byteBuffer[i];
			}
			printf("\r\n");
			inputBuffer[0] = getch(10);
			inputBuffer[1] = getch(10);
			check += hexToInt2(inputBuffer);
			if ((check & 0xff) != 0)
			{
				printf("ERROR: checksum not correct.\r\n");
				finished = true;
				continue;
			}

			printf("Checksum ok - writing buffer.\r\n");
			writeBuffer( base_address + offset, byteBuffer, len);
		}

		if (type == 1)
		{
			// Ignore checksum for this type.
			getch(10);
			getch(10);
			finished = true;
			printf("DONE: Hex file loaded successfully.");
		}

		// Skip \n
		getch(10);
	}

	releaseBus();
}

void modeChange()
{
	if (stepMode) {
		CLOCK_PORT.OUTCLR = CLOCK_PIN_bm;
		run = false;
	}
	else {
		printf("\r\nRunning...\r\n");
		run = true;
	}
}

void runTo(char *parameters)
{
	uint16_t address = 0;

	while(!isalnum(*parameters) && *parameters)
		parameters++;

	if (strlen(parameters) == 4)
		address = hexToInt4(parameters);

	if (!address)
	{
		printf("Missing or could not understand address.\r\n");
		return;
	}

	while(usart_recieve_char())
		;

	printf( "Running until address %04X is read.\r\n", address);

	bool done = false;

	while (!done)
	{
		CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;

		if (haveRecievedIO) {
			printf("%c", recievedIO);
			haveRecievedIO = false;
		}

		if (usart_recieve_char())
		{
			printf("Got char - stopping\r\n");
			done = true;
		}

		if (mcp23017_read_both(MCP23017_ADDR) == address)
		{
			if ( !(MREQ_PORT.IN & MREQ_PIN_bm) && !(RD_PORT.IN & RD_PIN_bm))
			{
				printf("Got address - stopping\r\n");
				done = true;
			}
		}
	}

	stepMode = true;
	modeChange();

	return;
}

void executeCommand(char *command)
{
	for (char *c = command; *c != 0; c++)
		*c = toupper(*c);

	printf("\r\nExecuting: %s\r\n", command);

	if (strncmp("STEP", command, 4) == 0 || command[0] == 0)
	{
		display = true;
		CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;
		delay(100);
		CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;
		return;
	}

	if (strncmp("RUNTO", command, 5) == 0)
	{
		runTo(&command[5]);
		return;
	}

	if (strncmp("RUN", command, 3) == 0)
	{
		stepMode = false;
		modeChange();
		return;
	}

	if (strncmp("DUMP", command, 4) == 0)
	{
		dumpMemory(&command[4]);
		return;
	}

	if (strncmp("DISP", command, 4) == 0)
	{
		display = true;
		return;
	}

	if (strncmp("NODISP", command, 6) == 0)
	{
		display = false;
		return;
	}

	if (strncmp("D", command, 1) == 0)
	{
		dumpMemory(&command[1]);
		return;
	}

	if (strncmp("LOAD", command, 4) == 0)
	{
		loadMemory(&command[4]);
		return;
	}
}

void processTerminalInput()
{

	char in = usart_recieve_char();

	if (!in)
		return;

	if (in == '\r')
	{
		executeCommand(command);
		commandPos = 0;
		command[0] = 0;
	}

	if (in == 8)
	{
		if (commandPos)
			commandPos--;

		command[commandPos] = 0;
	}

	if (in < ' ')
		return; // Chuck away 0 - 32

	command[commandPos++] = in;
	command[commandPos] = 0;

	if (commandPos == 31)
 	{
		printf("ERROR CMD overflow. Resetting.\r\n");
		commandPos = 0;
		while (in)
			in = getch(10);
	}
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

	printf("\r\n\n\nBooting Z80 interface\r\nCompiled: %s %s\r\n", __DATE__, __TIME__);

	unsigned long nextTime = 0;
	char message[32] = {0};
	uint8_t message_pos = 0;

	for (;;) {
		unsigned long currentTime = millis();

		processTerminalInput();

		if (run && currentTime > nextTime) {
			CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;
			//nextTime = currentTime + 1; // Comment this line for go "fast"! (About 43Khz) ;-)
		}

		if (run == false && stepMode == true && button_released(&stepButton, currentTime)) {
			CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;
			delay(100);
			CLOCK_PORT.OUTTGL = CLOCK_PIN_bm;
		}

		if (button_released(&modeButton, currentTime))
		{
			stepMode = !stepMode;
			modeChange();
		}

		if (haveRecievedIO) {
			if (stepMode)
			{
				message[message_pos++] = recievedIO;
				message[message_pos] = 0;
				if (message_pos > 31)
					message_pos = 0;
			}
			else
			{
				printf("%c", recievedIO);
			}
			haveRecievedIO = false;
		}

		if (stepMode && display) {
			uint8_t data = DATA_VPORT.IN;
			uint16_t address = mcp23017_read_both(MCP23017_ADDR);
			bool readBit = RD_PORT.IN & RD_PIN_bm;
			bool writeBit = WR_PORT.IN & WR_PIN_bm;
			bool mreqBit = MREQ_PORT.IN & MREQ_PIN_bm;
			bool iorqBit = IORQ_PORT.IN & IORQ_PIN_bm;
			bool busReqBit = BUSREQ_PORT.IN & BUSREQ_PIN_bm;
			bool busAckBit = BUSACK_PORT.IN & BUSACK_PIN_bm;

			printf("%10lu - %4s %7s %04x %02x [%c%c%c%c%c%c] '%c' [%-32s] > %-32s\r",
					currentTime, 
					(stepMode) ? "Step" : "Run",
					(run) ? "Running" : "",
					address,
					data,
					readBit    ? ' ' : 'R',
					writeBit   ? ' ' : 'W',
					mreqBit    ? ' ' : 'M',
					iorqBit    ? ' ' : 'I',
					busReqBit  ? ' ' : 'Q',
					busAckBit  ? ' ' : 'A',
					(data > 32 && data < 128) ? data : ' ',
					message,
					command
					);
			//delay(1000);
		}
	}

	return 0;
}
