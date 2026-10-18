/* The HCI USB transport of the Bluetooth stack (bt_usb_iokit.c): IOKit's
   user-space USB interfaces (Mac OS X 10.4 and later). Control requests go
   out on the device, events come in on the interrupt pipe and ACL data on the
   bulk pipes, through an IOKit run-loop source on the thread that opens it. */
#ifndef BT_USB_H
#define BT_USB_H

#include "bt_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_USB_VENDOR 0x2357
#define BT_USB_PRODUCT 0x0604

struct bt_usb_probe {
    int dongle;               /* BT_DONGLE_NONE, _FOUND or _APPLE */
    int nvram_never;          /* bluetoothHostControllerSwitchBehavior is "never" */
    int apple_controller;     /* Apple's Bluetooth stack has a controller (the built-in one) */
};

/* Looks without opening: is the dongle there, and does Apple's driver have it. */
void bt_usb_probe(struct bt_usb_probe *out);

/* Opens the dongle (on the calling thread's run loop) and fills in the
   transport. Returns 0, or -1 with *dongle saying why (BT_DONGLE_NONE,
   _APPLE or _BUSY) and reason a sentence for the log. */
int bt_usb_open(struct bt_transport *tr, int *dongle, char *reason, unsigned reason_size);

#ifdef __cplusplus
}
#endif
#endif
