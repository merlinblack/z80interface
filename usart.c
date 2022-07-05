#include <avr/io.h>
#include <stdio.h>

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
