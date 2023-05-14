
#include "usb.h"
#include <stddef.h>

#define VENDOR_ID        0x03eb
#define PRODUCT_ID       0x201d
#define STR_MANUFACTURER L"soramimi@soramimi.jp"
#define STR_PRODUCT      L"keypad"
#define STR_SERIALNUMBER L"1"

#define ENDPOINT0_SIZE		32

#if KEYBOARD_ENABLED
#define KEYBOARD_INTERFACE  0
#define KEYBOARD_ENDPOINT   3
#define KEYBOARD_SIZE       8
#define KEYBOARD_BUFFER     EP_DOUBLE_BUFFER
#endif

#if MOUSE_ENABLED
#define MOUSE_INTERFACE     1
#define MOUSE_ENDPOINT      4
#define MOUSE_SIZE          8
#define MOUSE_BUFFER        EP_DOUBLE_BUFFER
#endif

PROGMEM static const uint8_t endpoint_config_table[] = {
#if KEYBOARD_ENABLED
	KEYBOARD_ENDPOINT, EP_TYPE_INTERRUPT_IN, EP_SIZE(KEYBOARD_SIZE) | KEYBOARD_BUFFER,
#endif
#if MOUSE_ENABLED
	MOUSE_ENDPOINT,    EP_TYPE_INTERRUPT_IN, EP_SIZE(MOUSE_SIZE) | MOUSE_BUFFER,
#endif
	0
};

// device descriptor

PROGMEM static const uint8_t device_desc[] = {
	18,                  // size
	1,                   // descriptor type
	0x10, 0x01,          // bcdUSB
	0,                   // device class
	0,                   // device subclass
	0,                   // device protocol
	ENDPOINT0_SIZE,      // max packet size for EP0
	LSB(VENDOR_ID), MSB(VENDOR_ID),   // vendor ID
	LSB(PRODUCT_ID), MSB(PRODUCT_ID), // product ID
	0x00, 0x01,          // bcdDevice
	1,                   // index of string descriptor for manufacturer
	2,                   // index of string descriptor for product
	3,                   // index of string descriptor for serial number
	1                    // number of configurations
};

// report descriptors

#if KEYBOARD_ENABLED
PROGMEM static const uint8_t keyboard_hid_report_desc[] = {
	0x05, 0x01,          // Usage Page (Generic Desktop),
	0x09, 0x06,          // Usage (Keyboard),
	0xa1, 0x01,          // Collection (Application),

	0x05, 0x07,          //   Usage Page (Key Codes),
	0x19, 0xe0,          //   Usage Minimum (224),
	0x29, 0xe7,          //   Usage Maximum (231),
	0x15, 0x00,          //   Logical Minimum (0),
	0x25, 0x01,          //   Logical Maximum (1),
	0x95, 0x08,          //   Report Count (8),
	0x75, 0x01,          //   Report Size (1),
	0x81, 0x02,          //   Input (Data, Variable, Absolute), ;Modifier byte

	0x95, 0x01,          //   Report Count (1),
	0x75, 0x08,          //   Report Size (8),
	0x81, 0x01,          //   Input (Constant),                 ;Reserved byte

	0x05, 0x07,          //   Usage Page (Key Codes),
	0x19, 0x00,          //   Usage Minimum (0),
	0x29, 0xff,          //   Usage Maximum (255),
	0x15, 0x00,          //   Logical Minimum (0),
	0x26, 0xff, 0x00,    //   Logical Maximum(255),
	0x95, 0x06,          //   Report Count (6),
	0x75, 0x08,          //   Report Size (8),
	0x81, 0x00,          //   Input (Data, Array, Absolute),

	0x05, 0x08,          //   Usage Page (LEDs),
	0x19, 0x01,          //   Usage Minimum (1),
	0x29, 0x05,          //   Usage Maximum (5),
	0x95, 0x05,          //   Report Count (5),
	0x75, 0x01,          //   Report Size (1),
	0x91, 0x02,          //   Output (Data, Variable, Absolute, NonVolatile), ;LED report

	0x95, 0x01,          //   Report Count (1),
	0x75, 0x03,          //   Report Size (3),
	0x91, 0x01,          //   Output (Constant),                 ;LED report padding

	0xc0                 // End Collection
};
#endif

#if MOUSE_ENABLED
PROGMEM static const uint8_t mouse_hid_report_desc[] = {   /* USB report descriptor */
	0x05, 0x01,          // Usage Page (Generic Desktop)
	0x09, 0x02,          // Usage (Mouse)
	0xa1, 0x01,          // Collection (Application)
	0x09, 0x01,          //   Usage (Pointer)
	0xa1, 0x00,          //   Collection (Physical)

	0x05, 0x09,          //     Usage Page (Button)
	0x19, 0x01,          //     Usage Minimum (1)
	0x29, 0x05,          //     Usage Maximum (5)
	0x15, 0x00,          //     Logical Minimum (0)
	0x25, 0x01,          //     Logical Maximum (1)
	0x95, 0x05,          //     Report Count (5)
	0x75, 0x01,          //     Report Size (1)
	0x81, 0x02,          //     Input (Data,Variable,Absolute)

	0x95, 0x01,          //     Report Count (1)
	0x75, 0x03,          //     Report Size (3)
	0x81, 0x01,          //     Input (Const)

	0x05, 0x01,          //     Usage Page (Generic Desktop)
	0x09, 0x30,          //     Usage (X)
	0x09, 0x31,          //     Usage (Y)
	0x09, 0x38,          //     Usage (Wheel)
	0x15, 0x81,          //     Logical Minimum (-127)
	0x25, 0x7f,          //     Logical Maximum (127)
	0x75, 0x08,          //     Report Size (8)
	0x95, 0x03,          //     Report Count (3)
	0x81, 0x06,          //     Input (Data,Variable,Relative)

	0xc0,                //   End Collection
	0xc0,                // End Collection
};
#endif

// configuration descriptor

#if KEYBOARD_ENABLED
#define KEYBOARD_HID_DESC_OFFSET (9 + 9)
#define KEYBOARD_HID_DESC_LENGTH (9 + 9 + 7)
#else
#define KEYBOARD_HID_DESC_LENGTH (0)
#endif

#if MOUSE_ENABLED
#define MOUSE_HID_DESC_OFFSET (9 + KEYBOARD_HID_DESC_LENGTH + 9)
#define MOUSE_HID_DESC_LENGTH (9 + 9 + 7)
#else
#define MOUSE_HID_DESC_LENGTH (0)
#endif

#define USB_CURRENT_mA 100

PROGMEM const uint8_t config_desc[] = {    /* USB configuration descriptor */
	9, // sizeof(usbDescriptorConfiguration): length of descriptor in bytes
	2, // descriptor type
	9 + KEYBOARD_HID_DESC_LENGTH + MOUSE_HID_DESC_LENGTH, 0, // total length of data returned (including inlined descriptors)
	KEYBOARD_ENABLED + MOUSE_ENABLED, // number of interfaces in this configuration
	1, // index of this configuration
	0, // configuration name string index
#if USB_CFG_IS_SELF_POWERED
	(1 << 7) | USBATTR_SELFPOWER, // attributes
#else
	(1 << 7) | (1 << 5), // bus powered | remote wakeup
#endif
	USB_CURRENT_mA / 2, // max USB current in 2mA units

#if KEYBOARD_ENABLED
	//// interface: keyboard

	// interface descriptor follows inline:
	9, // sizeof(usbDescrInterface): length of descriptor in bytes
	4, // descriptor type
	KEYBOARD_INTERFACE, // index of this interface
	0, // alternate setting for this interface
	1, // endpoints excl 0: number of endpoint descriptors to follow
	3, // interface class
	1, // interface subclass
	1, // interface protocol
	0, // string index for interface

	9, // sizeof(usbDescrHID): length of descriptor in bytes
	0x21, // descriptor type: HID
	0x10, 0x01, // BCD representation of HID version
	0x00, // target country code
	0x01, // number of HID Report (or other HID class) Descriptor infos to follow
	0x22, // descriptor type: report
	sizeof(keyboard_hid_report_desc), 0, // total length of report descriptor

	7, // sizeof(usbDescrEndpoint)
	5, // descriptor type = endpoint
	KEYBOARD_ENDPOINT | 0x80, // IN endpoint
	0x03, // attrib: Interrupt endpoint
	8, 0, // maximum packet size
	10, // in ms
#endif

#if MOUSE_ENABLED
	//// interface: mouse

	// interface descriptor follows inline:
	9, // sizeof(usbDescrInterface): length of descriptor in bytes
	4, // descriptor type
	MOUSE_INTERFACE, // index of this interface
	0, // alternate setting for this interface
	1, // endpoints excl 0: number of endpoint descriptors to follow
	3, // interface subclass
	1, // interface subclass
	2, // interface protocol
	0, // string index for interface

	9, // sizeof(usbDescrHID): length of descriptor in bytes
	0x21, // descriptor type: HID
	0x10, 0x01, // BCD representation of HID version
	0x00, // target country code
	0x01, // number of HID Report (or other HID class) Descriptor infos to follow
	0x22, // descriptor type: report
	sizeof(mouse_hid_report_desc), 0, // total length of report descriptor

	7, // sizeof(usbDescrEndpoint)
	5, // descriptor type = endpoint
	MOUSE_ENDPOINT | 0x80, // IN endpoint
	0x03, // attrib: Interrupt endpoint
	8, 0, // maximum packet size
	10, // in ms
#endif
};

// strings

struct usb_string_descriptor_struct {
	uint8_t length;
	uint8_t descriptor_type;
	int16_t string[];
};
PROGMEM static const struct usb_string_descriptor_struct string0 = {
	4,
	3,
{0x0409}
};
PROGMEM static const struct usb_string_descriptor_struct string1 = {
	sizeof(STR_MANUFACTURER),
	3,
	STR_MANUFACTURER
};
PROGMEM static const struct usb_string_descriptor_struct string2 = {
	sizeof(STR_PRODUCT),
	3,
	STR_PRODUCT
};
PROGMEM static const struct usb_string_descriptor_struct string3 = {
	sizeof(STR_SERIALNUMBER),
	3,
	STR_SERIALNUMBER
};

// descriptor list

struct descriptor_list_struct {
	uint16_t value;
	uint16_t index;
	uint8_t const *addr;
	uint8_t length;
};
PROGMEM static const struct descriptor_list_struct descriptor_list[] = {
{0x0100, 0x0000, device_desc, sizeof(device_desc)},
{0x0200, 0x0000, config_desc, sizeof(config_desc)},
#if KEYBOARD_ENABLED
{0x2200, KEYBOARD_INTERFACE, keyboard_hid_report_desc, sizeof(keyboard_hid_report_desc)},
{0x2100, KEYBOARD_INTERFACE, config_desc+KEYBOARD_HID_DESC_OFFSET, 9},
#endif
#if MOUSE_ENABLED
{0x2200, MOUSE_INTERFACE, mouse_hid_report_desc, sizeof(mouse_hid_report_desc)},
{0x2100, MOUSE_INTERFACE, config_desc+MOUSE_HID_DESC_OFFSET, 9},
#endif
{0x0300, 0x0000, (uint8_t const *)&string0, 4},
{0x0301, 0x0409, (uint8_t const *)&string1, sizeof(STR_MANUFACTURER)},
{0x0302, 0x0409, (uint8_t const *)&string2, sizeof(STR_PRODUCT)},
{0x0303, 0x0409, (uint8_t const *)&string3, sizeof(STR_SERIALNUMBER)},
};
#define NUM_DESC_LIST (sizeof(descriptor_list)/sizeof(struct descriptor_list_struct))


// zero when we are not configured, non-zero when enumerated
static volatile uint8_t usb_configuration = 0;

#if KEYBOARD_ENABLED
uint8_t keyboard_data[8];
static uint8_t keyboard_protocol = 1;
static uint8_t keyboard_idle_config = 125;
volatile uint8_t keyboard_leds = 0;
#endif

#if MOUSE_ENABLED
uint8_t mouse_data[4];
static uint8_t mouse_protocol = 1;
static uint8_t mouse_idle_config = 125;
#endif

void usb_init()
{
	HW_CONFIG();
	USB_FREEZE();				// enable USB
	PLL_CONFIG();				// config PLL
	while (!(PLLCSR & (1 << PLOCK))) ;	// wait for PLL lock
	USB_CONFIG();				// start USB clock
	UDCON = 0;				// enable attach resistor
	usb_configuration = 0;
	UDIEN = (1 << EORSTE) | (1 << SOFE);
	sei();
}

uint8_t usb_configured()
{
	return usb_configuration;
}

void usb_remote_wakeup()
{
	if (UDINT & (1 << SUSPI)) {
		UDCON |= 1 << RMWKUP;
	}
}

int8_t usb_send(uint8_t ep, uint8_t const *ptr, int len)
{
	uint8_t i, intr_state, timeout;

	if (!usb_configuration) return -1;
	intr_state = SREG;

	usb_remote_wakeup();

	cli();
	UENUM = ep;
	timeout = UDFNUML + 50;
	while (1) {
		// are we ready to transmit?
		if (UEINTX & (1 << RWAL)) break;
		SREG = intr_state;
		// has the USB gone offline?
		if (!usb_configuration) return -1;
		// have we waited too long?
		if (UDFNUML == timeout) return -1;
		// get ready to try checking again
		intr_state = SREG;
		cli();
		UENUM = ep;
	}
	for (i = 0; i < len; i++) {
		UEDATX = ptr[i];
	}
	UEINTX = 0x3a;
	SREG = intr_state;
	return 0;
}

#if KEYBOARD_ENABLED
int8_t usb_keyboard_send(uint8_t const *data)
{
	return usb_send(KEYBOARD_ENDPOINT, data, 8);
}
#endif

#if MOUSE_ENABLED
int8_t usb_mouse_send()
{
	return usb_send(MOUSE_ENDPOINT, mouse_data, 4);
}
#endif

static inline void usb_wait_in_ready()
{
	while (!(UEINTX & (1 << TXINI))) ;
}

static inline void usb_send_in()
{
	UEINTX = ~(1 << TXINI);
}

static inline void usb_wait_receive_out()
{
	while (!(UEINTX & (1 << RXOUTI))) ;
}

static inline void usb_ack_out()
{
	UEINTX = ~(1 << RXOUTI);
}

ISR(USB_GEN_vect)
{
	uint8_t intbits;

	intbits = UDINT;
	UDINT = 0;
	if (intbits & (1 << EORSTI)) {
		UENUM = 0;
		UECONX = 1;
		UECFG0X = EP_TYPE_CONTROL;
		UECFG1X = EP_SIZE(ENDPOINT0_SIZE) | EP_SINGLE_BUFFER;
		UEIENX = (1 << RXSTPE);
		usb_configuration = 0;
	}
}

ISR(USB_COM_vect)
{
	UENUM = 0;
	uint8_t intbits = UEINTX;
	if (intbits & (1 << RXSTPI)) {
		uint8_t rtype = UEDATX;
		uint8_t request = UEDATX;
		uint16_t value = UEDATX;
		value |= (UEDATX << 8);
		uint16_t index = UEDATX;
		index |= (UEDATX << 8);
		uint16_t length = UEDATX;
		length |= (UEDATX << 8);
		UEINTX = ~((1 << RXSTPI) | (1 << RXOUTI) | (1 << TXINI));
		if (request == GET_DESCRIPTOR) {
			uint8_t const *desc_addr = NULL;
			uint8_t	desc_length = 0;
			uint8_t const *p = (uint8_t const *)descriptor_list;
			for (uint8_t i = 0; i < NUM_DESC_LIST; i++) {
				if (pgm_read_word(p) == value && pgm_read_word(p + 2) == index) {
					desc_addr = (uint8_t const *)pgm_read_word(p + 4);
					desc_length = pgm_read_byte(p + 6);
					break;
				}
				p += sizeof(struct descriptor_list_struct);
			}
			if (!desc_addr) {
				UECONX = (1 << STALLRQ) | (1 << EPEN);  //stall
				return;
			}
			uint8_t len = (length < 255) ? length : 255;
			if (len > desc_length) len = desc_length;
			uint8_t n;
			do {
				// wait for host ready for IN packet
				uint8_t i;
				do {
					i = UEINTX;
				} while (!(i & ((1 << TXINI) | (1 << RXOUTI))));
				if (i & (1 << RXOUTI)) return;	// abort
				// send IN packet
				n = len < ENDPOINT0_SIZE ? len : ENDPOINT0_SIZE;
				for (uint8_t i = 0; i < n; i++) {
					UEDATX = pgm_read_byte(desc_addr++);
				}
				len -= n;
				usb_send_in();
			} while (len || n == ENDPOINT0_SIZE);
			return;
		}
		if (request == SET_ADDRESS) {
			usb_send_in();
			usb_wait_in_ready();
			UDADDR = value | (1 << ADDEN);
			return;
		}
		if (request == SET_CONFIGURATION) {
			if (rtype == 0) {
				usb_configuration = value;
				usb_send_in();
				for (uint8_t i = 1; i <= MAX_ENDPOINT; i++) {
					UENUM = i;
					UECONX = 0;
				}
				uint8_t const *p = endpoint_config_table;
				while (1) {
					uint8_t ep = pgm_read_byte(p);
					if (!ep) break;
					p++;
					UENUM = ep;
					UECONX = 1 << EPEN;
					UECFG0X = pgm_read_byte(p++);
					UECFG1X = pgm_read_byte(p++);
				}
				UERST = 0x1e;
				UERST = 0;
				return;
			}
		}
		if (request == GET_CONFIGURATION) {
			if (rtype == 0x80) {
				usb_wait_in_ready();
				UEDATX = usb_configuration;
				usb_send_in();
				return;
			}
		}
		if (request == GET_STATUS) {
			usb_wait_in_ready();
			uint8_t i = 0;
			if (rtype == 0x82) {
				UENUM = index;
				if (UECONX & (1 << STALLRQ)) i = 1;
				UENUM = 0;
			}
			UEDATX = i;
			UEDATX = 0;
			usb_send_in();
			return;
		}
		if (request == CLEAR_FEATURE || request == SET_FEATURE) {
			if (rtype == 0x02 && value == 0) {
				uint8_t i = index & 0x7f;
				if (i >= 1 && i <= MAX_ENDPOINT) {
					usb_send_in();
					UENUM = i;
					if (request == SET_FEATURE) {
						UECONX = (1 << STALLRQ) | (1 << EPEN);
					} else {
						UECONX = (1 << STALLRQC) | (1 << RSTDT) | (1 << EPEN);
						UERST = (1 << i);
						UERST = 0;
					}
					return;
				}
			}
		}
#if KEYBOARD_ENABLED
		if (index == KEYBOARD_INTERFACE) {
			if (rtype == 0xa1) {
				if (request == HID_GET_IDLE) {
					usb_wait_in_ready();
					UEDATX = keyboard_idle_config;
					usb_send_in();
					return;
				}
				if (request == HID_GET_PROTOCOL) {
					usb_wait_in_ready();
					UEDATX = keyboard_protocol;
					usb_send_in();
					return;
				}
			}
			if (rtype == 0x21) {
				if (request == HID_SET_REPORT) {
					usb_wait_receive_out();
					keyboard_leds = UEDATX;
					usb_ack_out();
					usb_send_in();
					return;
				}
				if (request == HID_SET_IDLE) {
					keyboard_idle_config = (value >> 8);
					usb_send_in();
					return;
				}
				if (request == HID_SET_PROTOCOL) {
					keyboard_protocol = value;
					usb_send_in();
					return;
				}
			}
		}
#endif
#if MOUSE_ENABLED
		if (index == MOUSE_INTERFACE) {
			if (rtype == 0x21) {
				if (request == HID_SET_IDLE) {
					mouse_idle_config = (value >> 8);
					usb_send_in();
					return;
				}
				if (request == HID_SET_PROTOCOL) {
					mouse_protocol = value;
					usb_send_in();
					return;
				}
			}
		}
#endif
	}
	UECONX = (1 << STALLRQ) | (1 << EPEN);	// stall
}

