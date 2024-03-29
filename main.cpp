
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
static signed char _blink = 0;
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
		if (_blink >= 25) {
			_blink = -25;
		} else {
			_blink++;
		}
	}
}

void led(int n, bool f)
{
	uint8_t mask = 0x40 >> n;
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

#if KEYBOARD_ENABLED

#define KEY_MATRIX_ROWS 5
#define KEY_MATRIX_COLS 4

enum {
	MM_CTRL   = 0x01,
	MM_SHIFT  = 0x02,
	MM_ALT    = 0x04,
	MM_EXTRA  = 0x08,
	MM_EXTRA2 = 0x10,
	MM_SINGLESHOT = 0x80
};

static bool key_changed = false;
static int matrix_line = 0;
static uint8_t key_matrix[KEY_MATRIX_ROWS];
static uint8_t modifiers = 0;
static bool extra_modifier_released = false;
static uint8_t layout_set = 0;
static char two_stroke = 0;

// layout set 0

const PROGMEM uint16_t keytable_0a[] = {
	0x29, 0x1e, 0x1f, 0x20,
	0x05, 0x04, 0x07, 0x08,
	0x2b, 0x0a, 0x15, 0x16,
	0x00, 0x1b, 0x1c, 0x1d,
	0x00, 0x00, 0x14, 0x00,
};

const PROGMEM uint16_t keytable_0b[] = {
	0x28, 0x12, 0x17, 0x11,
	0x06, 0x0f, 0x19, 0x0c,
	0x1a, 0x0d, 0x0e, 0x10,
	0x00, 0x0b, 0x13, 0x09,
	0x00, 0x00, 0x18, 0x00,
};

const PROGMEM uint16_t keytable_0t[] = {
	0xff, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x63/*pad.*/, 0x5f | ((MM_SINGLESHOT | MM_SHIFT) << 8)/*shift+pad7*/, 0x5d/*pad5*/, 0x00,
};

// layout set 1

const PROGMEM uint16_t keytable_1a[] = {
	0x29, 0x04, 0x0e, 0x10,
	0x2c, 0x0c, 0x16, 0x1c,
	0x00, 0x18, 0x17, 0x15,
	0x00, 0x08, 0x11, 0x1a,
	0x00, 0x12, 0x0b, 0x00,
};

const PROGMEM uint16_t keytable_1b[] = {
	0x28, 0x4c, 0x0a, 0x14,
	0x2b, 0x2a, 0x1d, 0x0d,
	0x00, 0x09, 0x07, 0x0f,
	0x00, 0x1b, 0x06, 0x19,
	0x00, 0x13, 0x05, 0x00,
};

const PROGMEM uint16_t keytable_1c[] = { // MM_EXTRA2
	0x8b, 0x8a, 0x30, 0x32,
	0xff, 0x4a, 0x52, 0x4b,
	0xff, 0x50, 0x2d, 0x4f,
	0xff, 0x4d, 0x51, 0x4e,
	0xff, 0x36, 0x37, 0x00,
};

const PROGMEM uint16_t keytable_1t[] = { // two stroke 1
	0xff, 0x00, 0x00, 0x00,
	0x2f, 0x24, 0x25, 0x26,
	0xff, 0x21, 0x22, 0x23,
	0xff, 0x1e, 0x1f, 0x20,
	0xff, 0x27, 0x37, 0x36,
};

const PROGMEM uint16_t keytable_1u[] = { // two stroke 2
	0xff, 0x00, 0x00, 0x00,
	0xff, 0x8b, 0x8a, 0xff,
	0xff, 0x8b | ((MM_SINGLESHOT | MM_CTRL) << 8), 0x8a | ((MM_SINGLESHOT | MM_CTRL) << 8), 0x35,
	0xff, 0x09 | ((MM_SINGLESHOT | MM_CTRL | MM_SHIFT) << 8), 0x0d | ((MM_SINGLESHOT | MM_CTRL | MM_SHIFT) << 8), 0x88,
	0xff, 0x36, 0x37, 0x00,
};

const PROGMEM uint16_t keytable_1v[] = { // two stroke 3
	0xff, 0x00, 0x00, 0x00,
	0xff, 0x2e, 0x30, 0x32,
	0xff, 0x33, 0x34, 0x2f,
	0xff, 0x38, 0x87, 0x89,
	0xff, 0x36, 0x37, 0x00,
};

// layout set 2

const PROGMEM uint16_t keytable_2a[] = {
	0x29, 0x00, 0x00, 0x00,
	0x3a, 0x3b, 0x3c, 0x3d,
	0x3e, 0x3f, 0x40, 0x41,
	0x42, 0x43, 0x44, 0x45,
	0x00, 0x00, 0x00, 0x00,
};

const PROGMEM uint16_t keytable_2b[] = {
	0x28, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

//

void set_layout_set(uint8_t mode)
{
	layout_set = mode;
}

bool clear_key(uint8_t key)
{
	bool changed = false;
	if (key > 0) {
		if (key >= 0xe0 && key < 0xe8) {
			uint8_t c = keyboard_data[0];
			uint8_t d = c & ~(1 << (key - 0xe0));
			if (c != d) {
				keyboard_data[0] = d;
				changed = true;
			}
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
			changed = true;
		}
	}
	return changed;
}

bool press_key(uint8_t key, bool pressed)
{
	bool changed = false;
	if (key > 0 && key < 0xff) { // 0x00 is no key, 0xff is dummy
		if (pressed) {
			if (key >= 0xe0 && key < 0xe8) {
				uint8_t c = keyboard_data[0];
				uint8_t d = c | (1 << (key - 0xe0));
				if (c != d) {
					keyboard_data[0] = d;
					changed = true;
				}
			} else if (key > 0) {
				if (keyboard_data[2] != key) {
					clear_key(key);
					memmove(keyboard_data + 3, keyboard_data + 2, 5);
					keyboard_data[2] = key;
					changed = true;
				}
			}
		} else {
			changed |= clear_key(key);
		}
		keyboard_data[1] = 0;
	}
	return changed;
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

bool scan_key_matrix_line(int row)
{
	static uint16_t current_key_code = 0;
	static uint8_t single_shot_state = 0;

	const bool is_last_row = (row == KEY_MATRIX_ROWS - 1);
	const bool ex = modifiers & MM_EXTRA;

	bool changed = false;

	uint16_t const *keytable = nullptr;
	switch (two_stroke) {
	default:
		switch (layout_set) {
		default:
			keytable = !ex ? keytable_0a : keytable_0b;
			break;
		case 1:
			if (!(modifiers & MM_EXTRA2)) {
				keytable = !ex ? keytable_1a : keytable_1b;
			} else {
				keytable = keytable_1c;
			}
			break;
		case 2:
			keytable = !ex ? keytable_2a : keytable_2b;
			break;
		}
		break;
	case 1:
		switch (layout_set) {
		default:
			keytable = keytable_0t;
			break;
		case 1:
			keytable = keytable_1t;
			break;
		case 2:
			break;
		}
		break;
	case 2:
		switch (layout_set) {
		default:
			break;
		case 1:
			keytable = keytable_1u;
			break;
		case 2:
			break;
		}
		break;
	case 3:
		switch (layout_set) {
		default:
			break;
		case 1:
			keytable = keytable_1v;
			break;
		case 2:
			break;
		}
		break;
	}

	if (single_shot_state == 8) { // シングルショット送信済み
#if 0 // シングルショットを無効にする場合
		single_shot_state = !ex ? 0 : 2; // シングルショット処理済み
		two_stroke = false;
#else // シングルショットを継続する場合
		single_shot_state = 2;
#endif
		press_key((uint8_t)current_key_code, false); // シングルショット開放
		current_key_code = 0;
		changed = true;
	} else {
		uint8_t bits = 0;
		for (int r = 0; r < KEY_MATRIX_ROWS; r++) {
			bits |= key_matrix[r];
		}
		if (bits == 0) { // すべてのキーが離された
			single_shot_state = 0;
			current_key_code = 0;
		}
	}

	uint8_t prev_pins = key_matrix[row];
	uint8_t curr_pins = read_key_pins();

	for (int col = 0; col < KEY_MATRIX_COLS; col++) {
		int i = 4 * row + col;
		uint16_t c = 0;
		if (keytable) {
			c = pgm_read_word(keytable + i);
		}
		bool curr = (curr_pins >> col) & 1;
		bool prev = (prev_pins >> col) & 1;
		if (curr) {
			if (!prev) { // 押された
				if (two_stroke != 0) {
					switch (i) {
					case 1:
					case 2:
					case 3:
						set_layout_set(i - 1);
						continue;
					default:
						if (c == 0) {
							two_stroke = 0;
						} else {
							if (layout_set == 0) {
								current_key_code = c;
								single_shot_state = 6; // シングルショット送信待ち（離されたときにキーが送信される）
								continue;
							}
						}
						break;
					}
				}
				if (c & (MM_SINGLESHOT << 8)) {
					current_key_code = c;
					single_shot_state = 6;
					continue;
				}
				bool normal = true;
				switch (i) {
				case 4 * 2 + 0:
					if (ex && two_stroke == 0) {
						if (layout_set == 1) {
							two_stroke = 3;
							continue;
						}
					}
					break;
				case 4 * 3 + 0:
					if (ex && two_stroke == 0) {
						two_stroke = 2;
						continue;
					}
					break;
				case 4 * 4 + 0:
					if (ex && two_stroke == 0) {
						two_stroke = 1;
						continue;
					}
					break;
				case 4 * 4 + 3:
					if (layout_set == 1) {
						if (key_matrix[4] & 0x01) {
							two_stroke = 1;
							continue;
						}
						if (key_matrix[3] & 0x01) {
							two_stroke = 2;
							continue;
						}
						if (key_matrix[2] & 0x01) {
							if (layout_set == 1) {
								two_stroke = 3;
								continue;
							}
						}
					}
					single_shot_state |= 2; // シングルショット押下待ち
					continue;
				}
				if (normal) {
					if (c != 0) { // 通常のキー押下
						press_key((uint8_t)current_key_code, false);
						press_key(c, true);
						current_key_code = c;
						changed = true;
					}
					single_shot_state = 1; // シングルショット無効
				} else if (c != 0) {
					if (single_shot_state == 0 || single_shot_state == 2) {
						if (current_key_code != c) {
							current_key_code = c;
							single_shot_state |= 4; // シングルショット送信待ち
						} else {
							single_shot_state = 1; // シングルショット無効
						}
					}
				}
			}
		} else {
			if (prev) { // 離された
				if (single_shot_state == 6) { // シングルショット送信待ち状態？
					single_shot_state = 7; // シングルショット送信予約
				} else { // 通常のキー開放
					press_key((uint8_t)current_key_code, false);
					changed = true;
				}
			}
		}
	}

	key_matrix[row] = curr_pins;

	if (is_last_row) {
		modifiers = 0;

		if (single_shot_state == 6) { // シングルショット送信予約状態？
			modifiers = current_key_code >> 8;
			single_shot_state = 7; // シングルショット送信予約状態
		} else if (single_shot_state == 7) { // シングルショット送信予約状態？
			modifiers = current_key_code >> 8;
			press_key((uint8_t)current_key_code, true);
			single_shot_state = 8; // シングルショット送信済み
			changed = true;
		} else {
			auto SETMODIFIER = [&](bool f, uint8_t bit){
				if (f && bit) {
					modifiers |= bit;
				} else {
					modifiers &= ~bit;
				}
			};
			SETMODIFIER(key_matrix[4] & 0x01, MM_CTRL);
			if (layout_set == 0) {
				SETMODIFIER(key_matrix[3] & 0x01, MM_SHIFT);
				SETMODIFIER(key_matrix[4] & 0x02, MM_ALT);
			} else if (layout_set == 1) {
				SETMODIFIER(key_matrix[3] & 0x01, MM_SHIFT);
				SETMODIFIER(key_matrix[2] & 0x01, MM_EXTRA2);
			} else if (layout_set == 2) {
				SETMODIFIER(key_matrix[4] & 0x02, MM_SHIFT);
				SETMODIFIER(key_matrix[4] & 0x04, MM_ALT);
			}

			if (key_matrix[4] & 0x08) {
				modifiers |= MM_EXTRA;
			} else {
				if (modifiers & MM_EXTRA) {
					// when the extra modifier released, release other modifier keys
					extra_modifier_released = true;
					changed = true;
				}
				modifiers &= ~MM_EXTRA;
				two_stroke = 0;
				if (single_shot_state != 8) {
					single_shot_state = 0;
				}
			}
		}

		changed |= press_key(0xe0, modifiers & MM_CTRL); // ctrl
		changed |= press_key(0xe1, modifiers & MM_SHIFT); // shift
		changed |= press_key(0xe2, modifiers & MM_ALT); // alt
	}

	return changed;
}
#endif

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

	set_layout_set(0);

	while (1) {
		_delay_us(1);
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
						if (extra_modifier_released) {
							extra_modifier_released = false;
							data[0] = 0; // release modifiers
						}
						usb_keyboard_send(data);
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

		bool on = true;
		if (two_stroke != 0) {
			on = _blink > 0; // 点滅
		}
		led(0, on && layout_set == 0);
		led(1, on && layout_set == 1);
		led(2, on && layout_set == 2);
	}
}

