#ifndef _USBjoypad_H
#define _USBjoypad_H

#include <libusb-1.0/libusb.h>
#include <stdbool.h>
#define USB_HID_joypad_PROTOCOL 0

struct usb_joypad_packet {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keycode[6];
};

typedef struct {
    bool leftArrowPressed;
    bool rightArrowPressed;
    bool buttonAPressed;
    bool startPressed;
} ControllerState;

void controller_init();
void controller_update();
ControllerState controller_get_state();


/* Find and open a USB joypad device.  Argument should point to
   space to store an endpoint address.  Returns NULL if no joypad
   device was found. */
extern struct libusb_device_handle *openjoypad(uint8_t *);

#endif
