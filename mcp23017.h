#ifndef __MCP23017_H
#define __MCP23017_H

#include <stdint.h>
#include <stdbool.h>

#define MCP23017_GPIOA 0
#define MCP23017_GPIOB 1
#define MCP23017_BASE_ADDRESS 0x20

bool mcp23017_init(uint8_t address);
bool mcp23017_set_direction(uint8_t address, uint16_t mask);
bool mcp23017_set_pullup(uint8_t address, uint16_t mask);
uint16_t mcp23017_read_both(uint8_t address);
bool mcp23017_write_both(uint8_t address, uint16_t data);
uint8_t mcp23017_read(uint8_t address, uint8_t which);
bool mcp23017_write(uint8_t address, uint8_t which, uint8_t data);

#endif // __MCP23017_H
