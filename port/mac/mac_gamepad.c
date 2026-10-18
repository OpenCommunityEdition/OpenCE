/* Game pads through IOKit's HID device interface (mac_gamepad.h). */
#include "mac_gamepad.h"
#include "mac_gamepad_report.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define PAGE_GENERIC_DESKTOP 0x01
#define PAGE_SIMULATION 0x02
#define PAGE_BUTTON 0x09
#define USAGE_JOYSTICK 0x04
#define USAGE_GAME_PAD 0x05
#define USAGE_X 0x30
#define USAGE_Y 0x31
#define USAGE_Z 0x32
#define USAGE_RX 0x33
#define USAGE_RY 0x34
#define USAGE_RZ 0x35
#define USAGE_HAT 0x39
#define USAGE_ACCELERATOR 0xc4
#define USAGE_BRAKE 0xc5
#define XBOX_VENDOR 0x045e
#define XBOX_360_PRODUCT 0x028e
/* PlayStation pads, and adapters that present themselves as one (8BitDo's
   USB Wireless Adapter 2, as a DualShock 4): sticks X/Y and Z/Rz, the
   analog triggers Rx/Ry, buttons square, cross, circle, triangle, L1, R1,
   L2, R2, share, options, L3, R3, PS, touchpad */
#define SONY_VENDOR 0x054c
/* Nintendo pads (the Switch Pro controller, and adapters in Switch mode):
   buttons B, A, Y, X, L, R, ZL, ZR, minus, plus, L3, R3, home, capture,
   mapped by position (Nintendo's B is the Xbox pad's A); ZL and ZR are
   digital, full trigger pulls */
#define NINTENDO_VENDOR 0x057e
#define RESCAN_INTERVAL 2.0
/* A raw report stream is the pad's state until the next report arrives (an
   adapter may send only when something changes, so a quiet stream is not a
   pad that let go). A stream whose first reports do not decode is given up
   for the elements. */
#define RAW_BAD_LIMIT 8
#define RAW_BUFFER_SIZE 256
#define MAX_REPORTS_PER_POLL 64

/* ---------- mapping */

static short stick_value(long value, long minimum, long maximum, int flip)
{
    double scaled;
    if (maximum <= minimum) return 0;
    if (value < minimum) value = minimum;
    if (value > maximum) value = maximum;
    /* (rounded to the nearest, so that a byte axis gives the exact 257 steps of
       mac_gamepad_report.c's) */
    scaled = (double)(long)(((double)(value - minimum) / (double)(maximum - minimum)) * 65535.0 + 0.5) - 32768.0;
    if (flip) scaled = -scaled - 1.0;
    if (scaled < -32768.0) scaled = -32768.0;
    if (scaled > 32767.0) scaled = 32767.0;
    return (short)scaled;
}

static unsigned char trigger_value(long value, long minimum, long maximum)
{
    if (maximum <= minimum || value <= minimum) return 0;
    if (value >= maximum) return 255;
    return (unsigned char)((value - minimum) * 255 / (maximum - minimum));
}

/* A hat's position, from its first value clockwise from north, as d-pad
   bits; out of its range is the middle. A four-way hat has 4 positions. */
static unsigned hat_buttons(long value, long minimum, long maximum)
{
    static const unsigned eight[8] = {
        MAC_GAMEPAD_DPAD_UP, MAC_GAMEPAD_DPAD_UP | MAC_GAMEPAD_DPAD_RIGHT,
        MAC_GAMEPAD_DPAD_RIGHT, MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_DPAD_RIGHT,
        MAC_GAMEPAD_DPAD_DOWN, MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_DPAD_LEFT,
        MAC_GAMEPAD_DPAD_LEFT, MAC_GAMEPAD_DPAD_UP | MAC_GAMEPAD_DPAD_LEFT
    };
    long position = value - minimum;
    if (value < minimum || value > maximum) return 0;
    if (maximum - minimum == 3) return eight[position * 2];
    return position < 8 ? eight[position] : 0;
}

/* Buttons 1 to 10 as the Xbox One S driver's pad numbers them, which is the
   usual order of a PC pad's too: A, B, X, Y, the shoulders (the Xbox
   pad's white and black), View (back), Menu (start), the sticks. */
static void button_value(unsigned number, int down, struct mac_gamepad_frame *out)
{
    /* (numbers 11 and up: the Xbox and share buttons, unused) */
    static const int analog[6] = {
        MAC_GAMEPAD_A, MAC_GAMEPAD_B, MAC_GAMEPAD_X, MAC_GAMEPAD_Y,
        MAC_GAMEPAD_WHITE, MAC_GAMEPAD_BLACK
    };
    static const unsigned digital[4] = {
        MAC_GAMEPAD_BACK, MAC_GAMEPAD_START, MAC_GAMEPAD_LEFT_THUMB, MAC_GAMEPAD_RIGHT_THUMB
    };
    if (!down || number < 1) return;
    if (number <= 6) out->analog[analog[number - 1]] = 255;
    else if (number <= 10) out->buttons |= digital[number - 7];
}

void mac_gamepad_map(const struct mac_gamepad_element *elements,
    const long *values, unsigned count, unsigned vendor, unsigned product,
    struct mac_gamepad_frame *out)
{
    /* a PlayStation pad's buttons as the Xbox order's numbers, 0 for none
       (L2 and R2 are the analog triggers too) */
    static const unsigned char playstation_buttons[14] = { 3, 1, 2, 4, 5, 6, 0, 0, 7, 8, 9, 10, 0, 0 };
    unsigned i;
    int has_rx = 0, has_ry = 0;
    static const unsigned char nintendo_buttons[14] = { 1, 2, 3, 4, 5, 6, 0, 0, 7, 8, 9, 10, 0, 0 };
    int playstation = vendor == SONY_VENDOR;
    int nintendo = vendor == NINTENDO_VENDOR;
    (void)product;
    memset(out, 0, sizeof(*out));
    out->connected = 1;
    for (i = 0; i < count; ++i)
        if (elements[i].usage_page == PAGE_GENERIC_DESKTOP) {
            if (elements[i].usage == USAGE_RX) has_rx = 1;
            if (elements[i].usage == USAGE_RY) has_ry = 1;
        }
    for (i = 0; i < count; ++i) {
        const struct mac_gamepad_element *e = &elements[i];
        long v = values[i];
        if (e->usage_page == PAGE_BUTTON) {
            if (nintendo && (e->usage == 7 || e->usage == 8)) {
                if (v) out->analog[e->usage == 7 ? MAC_GAMEPAD_LEFT_TRIGGER : MAC_GAMEPAD_RIGHT_TRIGGER] = 255;
            } else if (nintendo)
                button_value(e->usage >= 1 && e->usage <= 14 ? nintendo_buttons[e->usage - 1] : 0,
                    v != 0, out);
            else if (playstation)
                button_value(e->usage >= 1 && e->usage <= 14 ? playstation_buttons[e->usage - 1] : 0,
                    v != 0, out);
            else
                button_value(e->usage, v != 0, out);
        } else if (e->usage_page == PAGE_SIMULATION) {
            if (e->usage == USAGE_ACCELERATOR)
                out->analog[MAC_GAMEPAD_RIGHT_TRIGGER] = trigger_value(v, e->minimum, e->maximum);
            else if (e->usage == USAGE_BRAKE)
                out->analog[MAC_GAMEPAD_LEFT_TRIGGER] = trigger_value(v, e->minimum, e->maximum);
        } else if (e->usage_page == PAGE_GENERIC_DESKTOP) {
            switch (e->usage) {
            case USAGE_X: out->left_x = stick_value(v, e->minimum, e->maximum, 0); break;
            case USAGE_Y: out->left_y = stick_value(v, e->minimum, e->maximum, 1); break;
            case USAGE_RX:
                if (playstation) out->analog[MAC_GAMEPAD_LEFT_TRIGGER] = trigger_value(v, e->minimum, e->maximum);
                else out->right_x = stick_value(v, e->minimum, e->maximum, 0);
                break;
            case USAGE_RY:
                if (playstation) out->analog[MAC_GAMEPAD_RIGHT_TRIGGER] = trigger_value(v, e->minimum, e->maximum);
                else out->right_y = stick_value(v, e->minimum, e->maximum, 1);
                break;
            /* With Rx and Ry the right stick, Z and Rz are the triggers (the
               Xbox pads); without them, Z and Rz are the right stick. */
            case USAGE_Z:
                if (has_rx && has_ry && !playstation)
                    out->analog[MAC_GAMEPAD_LEFT_TRIGGER] = trigger_value(v, e->minimum, e->maximum);
                else out->right_x = stick_value(v, e->minimum, e->maximum, 0);
                break;
            case USAGE_RZ:
                if (has_rx && has_ry && !playstation)
                    out->analog[MAC_GAMEPAD_RIGHT_TRIGGER] = trigger_value(v, e->minimum, e->maximum);
                else out->right_y = stick_value(v, e->minimum, e->maximum, 1);
                break;
            case USAGE_HAT: out->buttons |= hat_buttons(v, e->minimum, e->maximum); break;
            default: break;
            }
        }
    }
}

/* ---------- the devices */

/* Up to MAC_GAMEPAD_MAXIMUM_DEVICES pads, each in a slot that it keeps while
   it stays connected: a pad that goes leaves its slot free, and the next one
   connected takes the lowest free slot (so the controllers of the players
   do not shuffle when a pad drops). */
struct gamepad_device {
    IOHIDDeviceInterface122 **interface;
    io_service_t service;
    int joystick;
    unsigned vendor, product;
    unsigned count;
    struct mac_gamepad_element elements[MAC_GAMEPAD_MAXIMUM_ELEMENTS];
    IOHIDElementCookie cookies[MAC_GAMEPAD_MAXIMUM_ELEMENTS];
    char name[64];
    /* Sony pads (mac_gamepad_report.h): their input reports, which the
       interrupt pipe delivers to raw_report as the run loop runs in
       RAW_MODE, give the state; the HID elements' values are not read
       (an adapter may answer a poll with a canned frame, and a pad's
       elements are only as fresh as the driver's last report). */
    int raw_kind, raw_seen, raw_bad;
    CFRunLoopSourceRef raw_source;
    struct mac_gamepad_frame raw_frame;
    unsigned char raw_buffer[RAW_BUFFER_SIZE];
};

static struct gamepad_device pads[MAC_GAMEPAD_MAXIMUM_DEVICES];
static CFAbsoluteTime last_scan = -1.0e9;
static int reported;
/* the run loop mode the raw reports are delivered in, run only by poll_all */
#define RAW_MODE CFSTR("OpenCEGamepad")

static long dictionary_long(CFDictionaryRef dictionary, CFStringRef key, long fallback)
{
    CFTypeRef value = CFDictionaryGetValue(dictionary, key);
    long result = fallback;
    if (value && CFGetTypeID(value) == CFNumberGetTypeID())
        CFNumberGetValue((CFNumberRef)value, kCFNumberLongType, &result);
    return result;
}

static long service_long(io_service_t service, CFStringRef key, long fallback)
{
    CFTypeRef value = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    long result = fallback;
    if (value) {
        if (CFGetTypeID(value) == CFNumberGetTypeID())
            CFNumberGetValue((CFNumberRef)value, kCFNumberLongType, &result);
        CFRelease(value);
    }
    return result;
}

static void device_close(struct gamepad_device *pad)
{
    if (pad->raw_source) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), pad->raw_source, RAW_MODE);
        CFRelease(pad->raw_source);
    }
    if (pad->interface) {
        (*pad->interface)->close(pad->interface);
        (*pad->interface)->Release(pad->interface);
    }
    if (pad->service)
        IOObjectRelease(pad->service);
    memset(pad, 0, sizeof(*pad));
}

/* The device's input elements: buttons, axes and hats. */
static int device_elements(struct gamepad_device *pad)
{
    CFArrayRef elements = NULL;
    CFIndex i, n;
    if ((*pad->interface)->copyMatchingElements(pad->interface, NULL, &elements) != kIOReturnSuccess || !elements)
        return 0;
    n = CFArrayGetCount(elements);
    for (i = 0; i < n && pad->count < MAC_GAMEPAD_MAXIMUM_ELEMENTS; ++i) {
        CFDictionaryRef element = (CFDictionaryRef)CFArrayGetValueAtIndex(elements, i);
        long type, page, usage;
        if (!element || CFGetTypeID(element) != CFDictionaryGetTypeID()) continue;
        type = dictionary_long(element, CFSTR(kIOHIDElementTypeKey), 0);
        if (type != kIOHIDElementTypeInput_Misc && type != kIOHIDElementTypeInput_Button &&
            type != kIOHIDElementTypeInput_Axis) continue;
        page = dictionary_long(element, CFSTR(kIOHIDElementUsagePageKey), 0);
        usage = dictionary_long(element, CFSTR(kIOHIDElementUsageKey), 0);
        if (page != PAGE_GENERIC_DESKTOP && page != PAGE_SIMULATION && page != PAGE_BUTTON) continue;
        pad->elements[pad->count].usage_page = (unsigned)page;
        pad->elements[pad->count].usage = (unsigned)usage;
        pad->elements[pad->count].minimum = dictionary_long(element, CFSTR(kIOHIDElementMinKey), 0);
        pad->elements[pad->count].maximum = dictionary_long(element, CFSTR(kIOHIDElementMaxKey), 0);
        pad->cookies[pad->count] = (IOHIDElementCookie)(uintptr_t)
            dictionary_long(element, CFSTR(kIOHIDElementCookieKey), 0);
        ++pad->count;
    }
    CFRelease(elements);
    return pad->count != 0;
}

/* one input report from the pad's interrupt pipe */
static void raw_report(void *target, IOReturn result, void *refcon, void *sender, UInt32 size)
{
    struct gamepad_device *pad = (struct gamepad_device *)target;
    struct mac_gamepad_frame frame;
    (void)refcon; (void)sender;
    if (result != kIOReturnSuccess) return;
    if (size > RAW_BUFFER_SIZE) size = RAW_BUFFER_SIZE;
    if (mac_gamepad_decode_report(pad->raw_kind, pad->raw_buffer, size, &frame, NULL)) {
        pad->raw_frame = frame;
        pad->raw_seen = 1;
    } else if (pad->raw_bad < RAW_BAD_LIMIT && !pad->raw_seen)
        ++pad->raw_bad;
}

/* starts a Sony pad's reports; left off (the elements are read) when the
   interface will not deliver them */
static void device_raw_start(struct gamepad_device *pad)
{
    pad->raw_kind = mac_gamepad_report_kind(pad->vendor, pad->product);
    if (!pad->raw_kind) return;
    if ((*pad->interface)->createAsyncEventSource(pad->interface, &pad->raw_source) != kIOReturnSuccess ||
        !pad->raw_source ||
        (*pad->interface)->setInterruptReportHandlerCallback(pad->interface, pad->raw_buffer,
            sizeof(pad->raw_buffer), raw_report, pad, NULL) != kIOReturnSuccess) {
        if (pad->raw_source) { CFRelease(pad->raw_source); pad->raw_source = NULL; }
        pad->raw_kind = MAC_PAD_REPORT_NONE;
        return;
    }
    CFRunLoopAddSource(CFRunLoopGetCurrent(), pad->raw_source, RAW_MODE);
}

static int device_open(struct gamepad_device *pad, io_service_t service)
{
    IOCFPlugInInterface **plugin = NULL;
    SInt32 score = 0;
    CFTypeRef name;
    memset(pad, 0, sizeof(*pad));
    if (IOCreatePlugInInterfaceForService(service, kIOHIDDeviceUserClientTypeID,
        kIOCFPlugInInterfaceID, &plugin, &score) != kIOReturnSuccess || !plugin)
        return 0;
    (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122),
        (LPVOID *)&pad->interface);
    (*plugin)->Release(plugin);
    if (!pad->interface) return 0;
    /* (not seized: a driver that seizes the pad, as the Xbox One S
       driver seizes the controller itself, makes this fail, and the next
       pad is tried) */
    if ((*pad->interface)->open(pad->interface, 0) != kIOReturnSuccess) {
        (*pad->interface)->Release(pad->interface);
        pad->interface = NULL;
        return 0;
    }
    IOObjectRetain(service);
    pad->service = service;
    pad->vendor = (unsigned)service_long(service, CFSTR(kIOHIDVendorIDKey), 0);
    pad->product = (unsigned)service_long(service, CFSTR(kIOHIDProductIDKey), 0);
    name = IORegistryEntryCreateCFProperty(service, CFSTR(kIOHIDProductKey), kCFAllocatorDefault, 0);
    if (name) {
        if (CFGetTypeID(name) == CFStringGetTypeID())
            CFStringGetCString((CFStringRef)name, pad->name, sizeof(pad->name), kCFStringEncodingUTF8);
        CFRelease(name);
    }
    if (!device_elements(pad)) { device_close(pad); return 0; }
    device_raw_start(pad);
    return 1;
}

static int device_connected_count(void)
{
    int slot, count = 0;
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
        if (pads[slot].interface) ++count;
    return count;
}

/* whether the service is a pad already open (the same registry entry) */
static int service_is_open(io_service_t service)
{
    int slot;
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
        if (pads[slot].interface && IOObjectIsEqualTo(pads[slot].service, service))
            return 1;
    return 0;
}

/* opens the matching services of a usage that are not open yet, into the
   free slots, preferring the Xbox 360 pad's ids when xbox_only */
static void open_more(long usage, int xbox_only)
{
    CFMutableDictionaryRef matching = IOServiceMatching(kIOHIDDeviceKey);
    io_iterator_t iterator = 0;
    io_service_t service;
    long page = PAGE_GENERIC_DESKTOP;
    CFNumberRef number;
    if (!matching) return;
    number = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &page);
    CFDictionarySetValue(matching, CFSTR(kIOHIDPrimaryUsagePageKey), number);
    CFRelease(number);
    number = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &usage);
    CFDictionarySetValue(matching, CFSTR(kIOHIDPrimaryUsageKey), number);
    CFRelease(number);
    /* (the dictionary is consumed) */
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator) != kIOReturnSuccess)
        return;
    while ((service = IOIteratorNext(iterator)) != 0) {
        int slot = 0;
        while (slot < MAC_GAMEPAD_MAXIMUM_DEVICES && pads[slot].interface) ++slot;
        if (slot < MAC_GAMEPAD_MAXIMUM_DEVICES && !service_is_open(service) &&
            (!xbox_only ||
             (service_long(service, CFSTR(kIOHIDVendorIDKey), 0) == XBOX_VENDOR &&
              service_long(service, CFSTR(kIOHIDProductIDKey), 0) == XBOX_360_PRODUCT)) &&
            device_open(&pads[slot], service)) {
            pads[slot].joystick = usage == USAGE_JOYSTICK;
            if (reported < 16) {
                ++reported;
                fprintf(stderr, "game pad %d: %s (%04x:%04x), %u elements\n", slot + 1,
                    pads[slot].name[0] ? pads[slot].name : "unnamed",
                    pads[slot].vendor, pads[slot].product, pads[slot].count);
            }
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);
}

/* Looks for pads while a slot is free. Game pads (the Xbox 360's ids first)
   are taken before joysticks, and joysticks only while no pad is a game pad. */
static void scan(void)
{
    int slot, game_pads = 0;
    open_more(USAGE_GAME_PAD, 1);
    open_more(USAGE_GAME_PAD, 0);
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
        if (pads[slot].interface && !pads[slot].joystick) game_pads = 1;
    if (!game_pads)
        open_more(USAGE_JOYSTICK, 0);
}

/* the state of a slot's pad; 0 when the pad is gone (closed) */
static int device_poll(struct gamepad_device *pad, struct mac_gamepad_frame *out)
{
    long values[MAC_GAMEPAD_MAXIMUM_ELEMENTS];
    unsigned i;
    if (pad->raw_kind && pad->raw_bad >= RAW_BAD_LIMIT) {
        /* (the reports are not the layout expected: the elements are read) */
        pad->raw_kind = MAC_PAD_REPORT_NONE;
    }
    if (pad->raw_kind) {
        IOHIDEventStruct event;
        memset(&event, 0, sizeof(event));
        /* (the pad's removal shows as an element that cannot be read) */
        if ((*pad->interface)->getElementValue(pad->interface, pad->cookies[0], &event) != kIOReturnSuccess) {
            if (reported < 16) { ++reported; fprintf(stderr, "game pad: %s went away\n", pad->name); }
            device_close(pad);
            return 0;
        }
        if (pad->raw_seen)
            *out = pad->raw_frame;
        else {
            memset(out, 0, sizeof(*out));
            out->connected = 1;
        }
        return 1;
    }
    for (i = 0; i < pad->count; ++i) {
        IOHIDEventStruct event;
        memset(&event, 0, sizeof(event));
        if ((*pad->interface)->getElementValue(pad->interface, pad->cookies[i], &event) != kIOReturnSuccess) {
            if (reported < 16) { ++reported; fprintf(stderr, "game pad: %s went away\n", pad->name); }
            device_close(pad);
            return 0;
        }
        values[i] = (long)event.value;
    }
    mac_gamepad_map(pad->elements, values, pad->count, pad->vendor, pad->product, out);
    return 1;
}

int mac_gamepad_poll_all(struct mac_gamepad_frame frames[MAC_GAMEPAD_MAXIMUM_DEVICES])
{
    int slot, count = 0;
    memset(frames, 0, sizeof(frames[0]) * MAC_GAMEPAD_MAXIMUM_DEVICES);
    /* the raw reports waiting (the run loop mode holds only the pads' sources) */
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
        if (pads[slot].raw_source) {
            int n = 0;
            while (n++ < MAX_REPORTS_PER_POLL &&
                CFRunLoopRunInMode(RAW_MODE, 0.0, true) == kCFRunLoopRunHandledSource) {}
            break;
        }
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
        if (pads[slot].interface && device_poll(&pads[slot], &frames[slot]))
            ++count;
    if (count < MAC_GAMEPAD_MAXIMUM_DEVICES) {
        CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
        if (now - last_scan >= RESCAN_INTERVAL) {
            last_scan = now;
            scan();
            if (device_connected_count() != count) {
                /* (a pad connected: its first state now) */
                for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
                    if (pads[slot].interface && !frames[slot].connected)
                        device_poll(&pads[slot], &frames[slot]);
                count = device_connected_count();
            }
        }
    }
    return count;
}

void mac_gamepad_poll(struct mac_gamepad_frame *out)
{
    struct mac_gamepad_frame frames[MAC_GAMEPAD_MAXIMUM_DEVICES];
    int slot;
    mac_gamepad_poll_all(frames);
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
        if (frames[slot].connected) {
            *out = frames[slot];
            return;
        }
    memset(out, 0, sizeof(*out));
}

void mac_gamepad_stop(void)
{
    int slot;
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
        device_close(&pads[slot]);
    last_scan = -1.0e9;
}

const char *mac_gamepad_name_at(int slot)
{
    return slot >= 0 && slot < MAC_GAMEPAD_MAXIMUM_DEVICES && pads[slot].interface ? pads[slot].name : "";
}

int mac_gamepad_is_playstation_at(int slot)
{
    return slot >= 0 && slot < MAC_GAMEPAD_MAXIMUM_DEVICES && pads[slot].interface &&
        pads[slot].vendor == SONY_VENDOR;
}

const char *mac_gamepad_name(void)
{
    int slot;
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot)
        if (pads[slot].interface) return pads[slot].name;
    return "";
}
