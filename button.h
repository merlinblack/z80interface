#ifndef __BUTTON_H
#define __BUTTON_H

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

typedef struct {
	// Name for debugging
	char name[5];
	unsigned long time_pressed;
	bool pressed;
	uint8_t mask;
	PORT_t *port;
} button;

#define DEBOUNCE_TIME 5

bool button_released(button *btn, unsigned long currentTime);

#endif // __BUTTON_H
