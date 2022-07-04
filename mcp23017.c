#include "mcp23017.h"
#include "twi.h"

#include <stdbool.h>

#define _IODIR 0x00
#define _IODIRA 0x00
#define _IODIRB 0x01
#define _IPOL 0x02
#define _IPOLA 0x02
#define _IPOLB 0x03
#define _GPINTEN 0x04
#define _GPINTENA 0x04
#define _GPINTENB 0x05
#define _DEFVAL 0x06
#define _DEFVALA 0x06
#define _DEFVALB 0x07
#define _INTCON 0x08
#define _INTCONA 0x08
#define _INTCONB 0x09
#define _IOCON 0x0A
#define _GPPU 0x0C
#define _GPPUA 0x0C
#define _GPPUB 0x0D
#define _INTF 0x0E
#define _INTFA 0x0E
#define _INTFB 0x0F
#define _INTCAP 0x10
#define _INTCAPA 0x10
#define _INTCAPB 0x11
#define _GPIO 0x12
#define _GPIOA 0x12
#define _GPIOB 0x13
#define _OLAT 0x14
#define _OLATA 0x14
#define _OLATB 0x15

bool mcp23017_write_byte(uint8_t address, uint8_t reg, uint8_t data)
{
	uint8_t out[2] = { reg, data };
	return TWI_MasterWrite( address, out, 2, true ) == 0;
}

bool mcp23017_write_word(uint8_t address, uint8_t reg, uint16_t data)
{
	uint8_t out[3] = { reg, data & 0xff, data>>8 };
	return TWI_MasterWrite( address, out, 3, true ) == 0;
}

bool mcp23017_init(uint8_t address)
{
	if (!mcp23017_write_byte( address, _IOCON, 0x00 ))
		return false;
}

bool mcp23017_set_direction(uint8_t address, uint16_t mask)
{
	return mcp23017_write_word( address, _IODIR, mask );
}

bool mcp23017_set_pullup(uint8_t address, uint16_t mask)
{
	return mcp23017_write_word( address, _GPPU, mask );
}

uint16_t mcp23017_read_both(uint8_t address)
{
	uint8_t out = _GPIO;
	uint8_t buffer[2];

	if (TWI_MasterWrite( address, &out, 1, false ))
	{
		printf("Failed to send register.\r\n");
		return 0;
	}

	if (TWI_MasterRead( address, buffer, 2, true) != 2)
	{
		printf("Failed to read bytes.\r\n");
		return 0;
	}

	printf( "mcp buffer: %X - %X\r\n", buffer[0], buffer[1] );

	return buffer[0] + (buffer[1] << 8);
}

bool mcp23017_write_both(uint8_t address, uint16_t data)
{
	return mcp23017_write_word( address, _GPIO, data );
}

uint8_t mcp23017_read(uint8_t address, uint8_t which)
{
	uint8_t out = _GPIO + which;
	uint8_t buffer;

	if (TWI_MasterWrite( address, &out, 1, false ))
		return 0;

	if (TWI_MasterRead( address, &buffer, 1, true) != 1)
		return 0;

	return buffer;
}

bool mcp23017_write(uint8_t address, uint8_t which, uint8_t data)
{
	return mcp23017_write_byte( address, _GPIO + which, data );
}
