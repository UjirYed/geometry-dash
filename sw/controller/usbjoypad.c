#include "usbjoypad.h"
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

/* References on libusb 1.0 and the USB HID/joypad protocol
 *
 * http://libusb.org
 * https://web.archive.org/web/20210302095553/https://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/
 *
 * https://www.usb.org/sites/default/files/documents/hid1_11.pdf
 *
 * https://usb.org/sites/default/files/hut1_5.pdf
 */

/*
 * Find and return a USB joypad device or NULL if not found
 * The argument con
 * 
 */
struct libusb_device_handle *openjoypad(uint8_t *endpoint_address) {
  libusb_device **devs;
  struct libusb_device_handle *joypad = NULL;
  struct libusb_device_descriptor desc;
  ssize_t num_devs, d;
  uint8_t i, k;
  
  /* Start the library */
  if ( libusb_init(NULL) < 0 ) {
    fprintf(stderr, "Error: libusb_init failed\n");
    exit(1);
  }

  /* Enumerate all the attached USB devices */
  if ( (num_devs = libusb_get_device_list(NULL, &devs)) < 0 ) {
    fprintf(stderr, "Error: libusb_get_device_list failed\n");
    exit(1);
  }

  /* Look at each device, remembering the first HID device that speaks
     the joypad protocol */

  for (d = 0 ; d < num_devs ; d++) {
    libusb_device *dev = devs[d];
    if ( libusb_get_device_descriptor(dev, &desc) < 0 ) {
      fprintf(stderr, "Error: libusb_get_device_descriptor failed\n");
      exit(1);
    }

    if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE) {
      struct libusb_config_descriptor *config;
      libusb_get_config_descriptor(dev, 0, &config);
      for (i = 0 ; i < config->bNumInterfaces ; i++)	       
	for ( k = 0 ; k < config->interface[i].num_altsetting ; k++ ) {
	  const struct libusb_interface_descriptor *inter =
	    config->interface[i].altsetting + k ;
	  if ( inter->bInterfaceClass == LIBUSB_CLASS_HID &&
	       inter->bInterfaceProtocol == USB_HID_joypad_PROTOCOL) {
	    int r;
	    if ((r = libusb_open(dev, &joypad)) != 0) {
	      fprintf(stderr, "Error: libusb_open failed: %d\n", r);
	      exit(1);
	    }
	    if (libusb_kernel_driver_active(joypad,i))
	      libusb_detach_kernel_driver(joypad, i);
	    libusb_set_auto_detach_kernel_driver(joypad, i);
	    if ((r = libusb_claim_interface(joypad, i)) != 0) {
	      fprintf(stderr, "Error: libusb_claim_interface failed: %d\n", r);
	      exit(1);
	    }
	    *endpoint_address = inter->endpoint[0].bEndpointAddress;
	    goto found;
	  }
	}
    }
  }

 found:
  libusb_free_device_list(devs, 1);

  return joypad;
}


struct libusb_device_handle *joypad;
uint8_t endpoint_address;
volatile ControllerState state = {false, false, false, false};
pthread_mutex_t stateMutex;


void* controller_update_thread(void* arg) {
    struct usb_joypad_packet packet;
    int transferred;
    while (1) {
        int result = libusb_interrupt_transfer(joypad, endpoint_address, (unsigned char*)&packet, sizeof(packet), &transferred, 0);
        if (result == 0 && transferred == sizeof(packet)) {
            pthread_mutex_lock(&stateMutex);
            state.leftArrowPressed = (packet.keycode[1] == 0x00);
            state.rightArrowPressed = (packet.keycode[1] == 0xFF);
            state.buttonAPressed = (packet.keycode[3] == 0x2F);
            state.startPressed = (packet.keycode[4] == 0x20);
            pthread_mutex_unlock(&stateMutex);
        } else {
            continue;
        }
	usleep(100000);
    }
    return NULL; 
}

void controller_init() {
    pthread_t threadId;
    pthread_mutex_init(&stateMutex, NULL);

    if ( (joypad = openjoypad(&endpoint_address)) == NULL ) {
        fprintf(stderr, "Did not find a joypad\n");
        exit(1);
    }

    pthread_create(&threadId, NULL, controller_update_thread, NULL);
}

ControllerState controller_get_state() {
    ControllerState currentState;
    pthread_mutex_lock(&stateMutex);
    currentState = state;
    pthread_mutex_unlock(&stateMutex);
    return currentState;
}


