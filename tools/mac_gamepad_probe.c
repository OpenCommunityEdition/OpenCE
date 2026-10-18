/* Shows what a game pad sends and what the game makes of it. Opens every HID
   game pad read-only (not seized) and, for seconds (default 10), prints:
     - each pad found (ids, name, element count);
     - its raw input reports: the first few in hex, the rate, and, for a Sony
       pad (DualShock 4, DualSense), the decoded state when it changes;
     - the game's view of the pads (mac_gamepad_poll_all) when it changes.
   Press buttons and move the sticks while it runs; with no input a DualShock 4
   still sends about 125 to 250 reports a second, so "0 reports" means the
   pad is asleep or not linked to its adapter. Writes nothing to the pad.
   Usage: mac_gamepad_probe [seconds] [vendor:product in hex]
   Build: see tools/mac_gamepad_probe_checks.sh */
#include "mac_gamepad_report.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROBED 8
struct probed {
    IOHIDDeviceInterface122 **interface;
    unsigned vendor, product;
    char name[64];
    int kind;
    unsigned long reports, undecoded;
    unsigned char buffer[256];
    unsigned char first[24];
    unsigned first_size;
    struct mac_gamepad_frame last;
    unsigned last_extra;
    int have_last;
    double first_time, last_time;
};
static struct probed probed[MAX_PROBED];
static int probed_count;

static long service_long(io_service_t service, CFStringRef key)
{
    CFTypeRef value = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    long result = 0;
    if (value) {
        if (CFGetTypeID(value) == CFNumberGetTypeID()) CFNumberGetValue((CFNumberRef)value, kCFNumberLongType, &result);
        CFRelease(value);
    }
    return result;
}

static void describe(const struct mac_gamepad_frame *f, unsigned extra, char *text)
{
    static const char *const analog[8] = { "A", "B", "X", "Y", "BLACK", "WHITE", "LT", "RT" };
    int i;
    text[0] = 0;
    sprintf(text, "L(%6d,%6d) R(%6d,%6d)", f->left_x, f->left_y, f->right_x, f->right_y);
    for (i = 0; i < 8; ++i)
        if (f->analog[i]) sprintf(text + strlen(text), " %s=%d", analog[i], f->analog[i]);
    if (f->buttons & MAC_GAMEPAD_DPAD_UP) strcat(text, " UP");
    if (f->buttons & MAC_GAMEPAD_DPAD_DOWN) strcat(text, " DOWN");
    if (f->buttons & MAC_GAMEPAD_DPAD_LEFT) strcat(text, " LEFT");
    if (f->buttons & MAC_GAMEPAD_DPAD_RIGHT) strcat(text, " RIGHT");
    if (f->buttons & MAC_GAMEPAD_START) strcat(text, " START");
    if (f->buttons & MAC_GAMEPAD_BACK) strcat(text, " BACK");
    if (f->buttons & MAC_GAMEPAD_LEFT_THUMB) strcat(text, " L3");
    if (f->buttons & MAC_GAMEPAD_RIGHT_THUMB) strcat(text, " R3");
    if (extra & MAC_PAD_EXTRA_PS) strcat(text, " PS");
    if (extra & MAC_PAD_EXTRA_TOUCHPAD) strcat(text, " TOUCHPAD");
    if (extra & MAC_PAD_EXTRA_MUTE) strcat(text, " MUTE");
}

static double now(void)
{
    return CFAbsoluteTimeGetCurrent();
}

static void on_report(void *target, IOReturn result, void *refcon, void *sender, UInt32 size)
{
    struct probed *p = (struct probed *)target;
    struct mac_gamepad_frame frame;
    unsigned extra = 0;
    (void)refcon; (void)sender;
    if (result != kIOReturnSuccess) return;
    if (size > sizeof(p->buffer)) size = sizeof(p->buffer);
    if (p->reports == 0) {
        p->first_size = size;
        memcpy(p->first, p->buffer, size < sizeof(p->first) ? size : sizeof(p->first));
        p->first_time = now();
    }
    ++p->reports;
    p->last_time = now();
    if (p->kind && mac_gamepad_decode_report(p->kind, p->buffer, size, &frame, &extra)) {
        if (!p->have_last || memcmp(&frame, &p->last, sizeof(frame)) != 0 || extra != p->last_extra) {
            char text[256];
            describe(&frame, extra, text);
            printf("  %s raw  %s\n", p->name, text);
            p->last = frame; p->last_extra = extra; p->have_last = 1;
        }
    } else if (p->kind)
        ++p->undecoded;
}

static void open_pads(unsigned want_vendor, unsigned want_product, int filtered)
{
    CFMutableDictionaryRef matching = IOServiceMatching(kIOHIDDeviceKey);
    io_iterator_t iterator = 0;
    io_service_t service;
    long page = 1;
    CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &page);
    CFDictionarySetValue(matching, CFSTR(kIOHIDPrimaryUsagePageKey), number);
    CFRelease(number);
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator) != kIOReturnSuccess) return;
    while ((service = IOIteratorNext(iterator)) != 0) {
        long usage = service_long(service, CFSTR(kIOHIDPrimaryUsageKey));
        unsigned vendor = (unsigned)service_long(service, CFSTR(kIOHIDVendorIDKey));
        unsigned product = (unsigned)service_long(service, CFSTR(kIOHIDProductIDKey));
        if ((usage == 4 || usage == 5) && probed_count < MAX_PROBED &&
            (!filtered || (vendor == want_vendor && product == want_product))) {
            struct probed *p = &probed[probed_count];
            IOCFPlugInInterface **plugin = NULL;
            SInt32 score = 0;
            CFTypeRef name;
            memset(p, 0, sizeof(*p));
            p->vendor = vendor; p->product = product;
            p->kind = mac_gamepad_report_kind(vendor, product);
            name = IORegistryEntryCreateCFProperty(service, CFSTR(kIOHIDProductKey), kCFAllocatorDefault, 0);
            if (name) {
                if (CFGetTypeID(name) == CFStringGetTypeID())
                    CFStringGetCString((CFStringRef)name, p->name, sizeof(p->name), kCFStringEncodingUTF8);
                CFRelease(name);
            }
            printf("found %04x:%04x \"%s\" usage %s%s%s\n", vendor, product, p->name, usage == 5 ? "game pad" : "joystick",
                p->kind ? ", report layout " : ", generic HID layout", mac_gamepad_report_name(p->kind));
            if (IOCreatePlugInInterfaceForService(service, kIOHIDDeviceUserClientTypeID,
                kIOCFPlugInInterfaceID, &plugin, &score) == kIOReturnSuccess && plugin) {
                (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122), (LPVOID *)&p->interface);
                (*plugin)->Release(plugin);
            }
            if (p->interface) {
                IOReturn r = (*p->interface)->open(p->interface, 0); /* not seized */
                CFRunLoopSourceRef source = NULL;
                printf("  open: %s (0x%x)\n", r == kIOReturnSuccess ? "ok" : "FAILED", (unsigned)r);
                if (r == kIOReturnSuccess &&
                    (*p->interface)->createAsyncEventSource(p->interface, &source) == kIOReturnSuccess &&
                    (*p->interface)->setInterruptReportHandlerCallback(p->interface, p->buffer, sizeof(p->buffer),
                        on_report, p, NULL) == kIOReturnSuccess) {
                    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);
                    ++probed_count;
                } else
                    printf("  report handler: FAILED\n");
            } else
                printf("  device interface: FAILED\n");
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 10.0;
    unsigned vendor = 0, product = 0;
    int filtered = argc > 2 && sscanf(argv[2], "%x:%x", &vendor, &product) == 2;
    double start, last_game = 0;
    struct mac_gamepad_frame game_last[MAC_GAMEPAD_MAXIMUM_DEVICES];
    int slot, seen_game = 0, i;
    if (seconds < 1.0) seconds = 1.0;
    memset(game_last, 0, sizeof(game_last));
    open_pads(vendor, product, filtered);
    if (!probed_count) { printf("no game pad could be opened\n"); return 1; }
    printf("reading for %.0f seconds: move the sticks and press buttons now\n", seconds);
    start = now();
    while (now() - start < seconds) {
        struct mac_gamepad_frame frames[MAC_GAMEPAD_MAXIMUM_DEVICES];
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, false);
        if (now() - last_game >= 0.016) {
            last_game = now();
            mac_gamepad_poll_all(frames);
            for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot) {
                if (frames[slot].connected && !game_last[slot].connected)
                    printf("  game: slot %d is \"%s\"%s\n", slot, mac_gamepad_name_at(slot),
                        mac_gamepad_is_playstation_at(slot) ? " (PlayStation)" : "");
                if (frames[slot].connected && (!seen_game || memcmp(&frames[slot], &game_last[slot], sizeof(frames[0])) != 0)) {
                    char text[256];
                    describe(&frames[slot], 0, text);
                    printf("  game slot %d  %s\n", slot, text);
                }
                game_last[slot] = frames[slot];
            }
            seen_game = 1;
        }
    }
    printf("\nsummary\n");
    for (i = 0; i < probed_count; ++i) {
        struct probed *p = &probed[i];
        double span = p->reports > 1 ? p->last_time - p->first_time : 0;
        printf("%04x:%04x \"%s\": %lu reports", p->vendor, p->product, p->name, p->reports);
        if (span > 0) printf(", %.1f per second", (p->reports - 1) / span);
        if (p->undecoded) printf(", %lu not decoded as a %s report", p->undecoded, mac_gamepad_report_name(p->kind));
        printf("\n");
        if (p->reports) {
            unsigned j;
            printf("  first report (%u bytes):", p->first_size);
            for (j = 0; j < p->first_size && j < sizeof(p->first); ++j) printf(" %02x", p->first[j]);
            printf("\n");
        } else
            printf("  no reports arrived: the pad is asleep or not linked to its adapter, or sends only on change; press a button and run again\n");
    }
    mac_gamepad_stop();
    return 0;
}
