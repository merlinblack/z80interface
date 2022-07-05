#include "button.h"

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
