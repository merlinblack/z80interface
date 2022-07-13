#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>

#define RECIEVE_BUFFER_SIZE 128 
#define XOFF_LIMIT (RECIEVE_BUFFER_SIZE - 32)
#define XON_LIMIT 0
#define XON 0x11
#define XOFF 0x13

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

volatile uint8_t recieve_end;
volatile uint8_t recieve_start;
volatile uint8_t recieve_count;
volatile bool xflow_halted;
char recieve_buffer[RECIEVE_BUFFER_SIZE];

ISR(USART0_RXC_vect)
{
	if (recieve_end == recieve_start-1)
	{
		printf("\r\nError: USART recieve buffer full. (%d)", recieve_count);
		// Throw away recieved byte
		(void)USART0.RXDATAL;
		return;
	}

	recieve_buffer[recieve_end++] = USART0.RXDATAL;
	recieve_count++;

	if (recieve_end > RECIEVE_BUFFER_SIZE-1)
		recieve_end = 0;

	if (!xflow_halted && recieve_count > XOFF_LIMIT)
	{
		xflow_halted = true;
		usart_send_char(XOFF);
		//printf("\r\nSent XOFF\n\r");
	}
}

char usart_recieve_char()
{
	//printf("\r%d %d %d %c", recieve_start, recieve_end, recieve_count, recieve_buffer[recieve_start]);
	if (recieve_start == recieve_end)
		return 0;

	uint8_t oldSREG = SREG;
	cli();

	char ret = recieve_buffer[recieve_start++];

	if (recieve_start > RECIEVE_BUFFER_SIZE-1)
		recieve_start = 0;

	recieve_count--;
	if (xflow_halted)
	{
		if (recieve_count == XON_LIMIT)
		{
			xflow_halted = false;
			usart_send_char(XON);
		  //printf("\r\nSent XON\n\r");
		}
	}

	SREG = oldSREG;

	return ret;
}

#define USART_BAUD_RATE(BAUD_RATE) ((uint16_t)((float)F_CPU * 64 / (16 * (float)BAUD_RATE) + 0.5))

void init_usart()
{
	recieve_end = 0;
	recieve_start = 0;
	recieve_count = 0;
	xflow_halted = false;
	/* Set baud rate */
	USART0.BAUD = USART_BAUD_RATE(115200);
	/* Enable TX and RX for USART0 */
	USART0.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
	/* Set TX pin to output, RX pin to input */
	PORTA.DIRSET = PIN0_bm;
	PORTA.DIRCLR = PIN1_bm;
	/* Enable RX interrupt */
	USART0.CTRLA |= USART_RXCIE_bm;
	/* Redirect stdout */
	stdout = &usart_stream;
}
