
#include <string.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
extern "C" {
#include "usb.h"
}

#define SCALE 125
static unsigned short _scale = 0;
static unsigned long _time_s;
static unsigned short _time_ms = 0;
ISR(TIMER0_OVF_vect, ISR_NOBLOCK)
{
	_scale += 16;
	if (_scale >= SCALE) {
		_scale -= SCALE;
		_time_ms++;
		if (_time_ms >= 1000) {
			_time_ms = 0;
			_time_s++;
		}
	}
}

#if KEYBOARD_ENABLED

#define KEY_MATRIX_ROWS 5
static bool key_changed = false;
static int matrix_line = 0;
static uint8_t key_matrix[KEY_MATRIX_ROWS];

const PROGMEM char keytable_default[] = {
	0x29, 0x1e, 0x1f, 0x20,
	0x06, 0x07, 0x04, 0x0f,
	0x2b, 0x0a, 0x15, 0x16,
	0x00, 0x1b, 0x1c, 0x1d,
	0x00, 0x00, 0x05, 0x00,
};

const PROGMEM char keytable_1[] = {
	0x28, 0x00, 0x00, 0x00,
	0x1a, 0x19, 0x08, 0x0c,
	0x14, 0x0d, 0x0e, 0x10,
	0x12, 0x0b, 0x13, 0x09,
	0x63, 0x11, 0x18, 0x00,
};

void clear_key(uint8_t key)
{
	if (key > 0) {
		if (key >= 0xe0 && key < 0xe8) {
			keyboard_data[0] &= ~(1 << (key - 0xe0));
		}
		uint8_t i = 8;
		while (i > 2) {
			i--;
			if (keyboard_data[i] == key) {
				if (i < 7) {
					memmove(keyboard_data + i, keyboard_data + i + 1, 7 - i);
				}
				keyboard_data[7] = 0;
			}
		}
	}
}

void release_all_keys()
{
	memset(keyboard_data + 2, 0, 6);
}

void release_key(uint8_t key)
{
	clear_key(key);
	keyboard_data[1] = 0;
}

void press_key(uint8_t key)
{
	if (key >= 0xe0 && key < 0xe8) {
		keyboard_data[0] |= 1 << (key - 0xe0);
	} else if (key > 0) {
		clear_key(key);
		memmove(keyboard_data + 3, keyboard_data + 2, 5);
		keyboard_data[2] = key;
	}
	keyboard_data[1] = 0;
}

void select_key_matrix_line(int i)
{
	PORTD = ~(0x01 << i);
}

uint8_t read_key_pins()
{
	uint8_t pins = PINC; // read pins
	// shift and invert bits
	pins |= 0x0f;
	pins ^= (pins >> 7) & 0x01;
	pins ^= (pins >> 5) & 0x02;
	pins ^= (pins >> 3) & 0x04;
	pins ^= (pins >> 1) & 0x08;
	return pins & 0x0f;
}

bool is_extra_key_pressed()
{
	return key_matrix[4] & 0x08;
}

bool scan_key_matrix_line(int row)
{
	char const *keytable = keytable_default;

	if (key_matrix[4] & 0x08) {
		keytable = keytable_1;
	}

	bool changed = false;
	uint8_t pins = read_key_pins();
	for (int col = 0; col < 4; col++) {
		int c = pgm_read_byte(keytable + (4 * row + col));
		bool f = (pins >> col) & 1;
		bool g = (key_matrix[row] >> col) & 1;
		if (f) {
			if (!g) {
				press_key(c);
				changed = true;
			}
		} else {
			if (g) {
				release_key(c);
				changed = true;
			}
		}
	}

	if (key_matrix[row] ^ pins) {
		if (row == 3) {
			if (pins & 0x01) { // shift
				press_key(0xe1);
			} else {
				release_key(0xe1);
			}
		} else if (row == 4) {
			if (pins & 0x01) { // ctrl
				if (!(key_matrix[4] & 0x08)) {
					press_key(0xe0);
				}
			} else {
				release_key(0xe0);
				release_key(0x63); // numpad .
			}
			if (pins & 0x02) { // alt
				press_key(0xe2);
			} else {
				release_key(0xe2);
			}
		}
	}

#if 0
	if (row == 4 && (key_matrix[row] & 0x08) && !(pins & 0x08)) {
		// when the extra key released, release all keys
		release_all_keys();
	}
#endif

	key_matrix[row] = pins;

	return changed;
}
#endif

void led(int n, bool f)
{
	uint8_t mask = 0x10 << n;
	if (f) {
		PORTB |= mask;
	} else {
		PORTB &= ~mask;
	}
}

extern "C" void debug(int n)
{
	led(0, n & 1);
	led(1, n & 2);
	led(2, n & 4);
}

int main()
{
	static unsigned short time_ms = 0;

	memset(key_matrix, 0, KEY_MATRIX_ROWS);

	// 16 MHz clock
	CLKPR = 0x80;
	CLKPR = 0;
	// Disable JTAG
	MCUCR |= 0x80;
	MCUCR |= 0x80;

	_delay_ms(10);

	PORTB = 0x00;
	DDRB = 0x70; // LEDs

	PORTC = 0xff; // pull-up
	DDRC = 0x00;

	PORTD = 0xff;
	DDRD = 0x1f;

	TCCR0B = 0x02; // 1/8 prescaling
	TIMSK0 |= 1 << TOIE0;

	usb_init();
	while (!usb_configured()) {
		_delay_ms(10);
	}

	sei();  // enable interrupts

#if KEYBOARD_ENABLED
	select_key_matrix_line(matrix_line);
#endif

	while (1) {
		if (0) {//if (_time_ms < 500) {
			led(0, true);
		} else {
			led(0, false);
		}

		bool send_ready = false;
		if (time_ms != _time_ms) {
			time_ms = _time_ms;
			if (time_ms & 1) { // every 2ms
				send_ready = true;
#if KEYBOARD_ENABLED
				if (matrix_line < KEY_MATRIX_ROWS) {
					if (scan_key_matrix_line(matrix_line)) { // scan
						key_changed = true;
					}
					matrix_line++;
				}
				if (matrix_line == KEY_MATRIX_ROWS) {
					if (key_changed && send_ready) {
						send_ready = false;
						key_changed = false;
						const int N = sizeof(keyboard_data);
						uint8_t data[N];
						memcpy(data, keyboard_data, N);
						if (is_extra_key_pressed()) {
							data[0] = 0; // release modifiers
						}
						usb_keyboard_send(data);
						debug(0);
					}
					matrix_line = 0;
				}
				select_key_matrix_line(matrix_line); // select line to next scan
#endif
			}
		}

		if (send_ready) {
			send_ready = false;

#if MOUSE_ENABLED
			mouse_data[0] = 0;
			mouse_data[1] = 1;
			mouse_data[2] = 1;
			mouse_data[3] = 0;
			usb_mouse_send();
#endif
		}
	}
}

