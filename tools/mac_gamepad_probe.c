/* Shows what a game pad sends and what the game makes of it. Opens every HID
   game pad read-only (not seized) and, for seconds (default 10), prints:
     - each pad found (ids, name, element count);
     - its raw input reports: the first few in hex, the rate, and, for a Sony
       pad (DualShock 4, DualSense), the decoded state when it changes;
     - the game's view of the pads (mac_gamepad_poll_all) when it changes.
   Press buttons and move the sticks while it runs; with no input a DualShock 4
   still sends about 125 to 250 reports a second, so "0 reports" means the
   pad is asleep or not linked to its adapter. Writes nothing to the pad.
   It survives a device that detaches and attaches again with other ids (8BitDo's
   adapter does, several times, while it pairs): it is told of each attach and
   detach, logs them with the time, and reads the device that took its place.
   Usage: mac_gamepad_probe [seconds] [vendor:product in hex]
   Build: see tools/mac_gamepad_probe_checks.sh */
#include "mac_gamepad_report.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/IOMessage.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Every attach is a new entry: an adapter detaches and attaches again, with other ids, as it pairs,
   and each of its lives is reported on its own in the summary. */
#define MAX_PROBED 64
struct probed {
    IOHIDDeviceInterface122 **interface;
    io_service_t service;
    io_object_t notifier;
    int active, is_pad;
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
    double first_time, last_time, attached_at, detached_at;
    CFRunLoopSourceRef source;
};
static struct probed probed[MAX_PROBED];
static int probed_count;
static double start_time;
static unsigned want_vendor, want_product;
static int filtered;
static IONotificationPortRef note_port;

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

static double since(void)
{
    return now() - start_time;
}

static void close_probed(struct probed *p)
{
    if (p->source) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), p->source, kCFRunLoopDefaultMode);
        CFRelease(p->source);
        p->source = NULL;
    }
    if (p->interface) {
        (*p->interface)->close(p->interface);
        (*p->interface)->Release(p->interface);
        p->interface = NULL;
    }
    if (p->notifier) { IOObjectRelease(p->notifier); p->notifier = 0; }
    if (p->service) { IOObjectRelease(p->service); p->service = 0; }
    p->active = 0;
}

/* IOKit: the device is terminated (unplugged, or re-enumerating); it is let go of at once */
static void on_message(void *refcon, io_service_t service, natural_t type, void *argument)
{
    struct probed *p = (struct probed *)refcon;
    (void)service; (void)argument;
    if (type != kIOMessageServiceIsTerminated || !p->active) return;
    p->detached_at = since();
    printf("t=%6.2f DETACH %04x:%04x \"%s\"%s, %lu reports\n", p->detached_at, p->vendor, p->product, p->name,
        p->is_pad ? "" : " (not a game pad)", p->reports);
    close_probed(p);
}

static int is_known(io_service_t service)
{
    int i;
    for (i = 0; i < probed_count; ++i)
        if (probed[i].active && IOObjectIsEqualTo(probed[i].service, service)) return 1;
    return 0;
}

static void attach(io_service_t service)
{
    long page = service_long(service, CFSTR(kIOHIDPrimaryUsagePageKey));
    long usage = service_long(service, CFSTR(kIOHIDPrimaryUsageKey));
    unsigned vendor = (unsigned)service_long(service, CFSTR(kIOHIDVendorIDKey));
    unsigned product = (unsigned)service_long(service, CFSTR(kIOHIDProductIDKey));
    int is_pad = page == 1 && (usage == 4 || usage == 5);
    struct probed *p;
    CFTypeRef name;
    if (is_known(service) || probed_count >= MAX_PROBED) return;
    /* (only the pads, and 8BitDo's own devices, which are named to show an adapter that is waiting) */
    if (!is_pad && vendor != 0x2dc8) return;
    if (filtered && (vendor != want_vendor || product != want_product)) return;
    p = &probed[probed_count++];
    memset(p, 0, sizeof(*p));
    p->vendor = vendor; p->product = product; p->is_pad = is_pad;
    p->kind = mac_gamepad_report_kind(vendor, product);
    p->attached_at = since();
    name = IORegistryEntryCreateCFProperty(service, CFSTR(kIOHIDProductKey), kCFAllocatorDefault, 0);
    if (name) {
        if (CFGetTypeID(name) == CFStringGetTypeID())
            CFStringGetCString((CFStringRef)name, p->name, sizeof(p->name), kCFStringEncodingUTF8);
        CFRelease(name);
    }
    printf("t=%6.2f ATTACH %04x:%04x \"%s\" usage page 0x%lx usage %ld: %s%s%s\n", p->attached_at, vendor, product, p->name,
        page, usage, is_pad ? (usage == 5 ? "game pad" : "joystick") : "not a game pad (an adapter waiting for a controller)",
        p->kind ? ", report layout " : is_pad ? ", generic HID layout" : "", mac_gamepad_report_name(p->kind));
    IOObjectRetain(service);
    p->service = service;
    p->active = 1;
    IOServiceAddInterestNotification(note_port, service, kIOGeneralInterest, on_message, p, &p->notifier);
    if (is_pad) {
        IOCFPlugInInterface **plugin = NULL;
        SInt32 score = 0;
        if (IOCreatePlugInInterfaceForService(service, kIOHIDDeviceUserClientTypeID,
            kIOCFPlugInInterfaceID, &plugin, &score) == kIOReturnSuccess && plugin) {
            (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122), (LPVOID *)&p->interface);
            (*plugin)->Release(plugin);
        }
        if (p->interface) {
            IOReturn r = (*p->interface)->open(p->interface, 0); /* not seized */
            printf("  open: %s (0x%x)\n", r == kIOReturnSuccess ? "ok" : "FAILED", (unsigned)r);
            if (r == kIOReturnSuccess &&
                (*p->interface)->createAsyncEventSource(p->interface, &p->source) == kIOReturnSuccess &&
                (*p->interface)->setInterruptReportHandlerCallback(p->interface, p->buffer, sizeof(p->buffer),
                    on_report, p, NULL) == kIOReturnSuccess)
                CFRunLoopAddSource(CFRunLoopGetCurrent(), p->source, kCFRunLoopDefaultMode);
            else
                printf("  report handler: FAILED\n");
        } else
            printf("  device interface: FAILED\n");
    }
    fflush(stdout);
}

/* the devices that match, as they are published (the iterator is drained to be armed again) */
static void on_matched(void *refcon, io_iterator_t iterator)
{
    io_service_t service;
    (void)refcon;
    while ((service = IOIteratorNext(iterator)) != 0) {
        attach(service);
        IOObjectRelease(service);
    }
}

static void watch(CFMutableDictionaryRef matching)
{
    io_iterator_t iterator = 0;
    if (matching && IOServiceAddMatchingNotification(note_port, kIOFirstMatchNotification, matching,
            on_matched, NULL, &iterator) == kIOReturnSuccess)
        on_matched(NULL, iterator);
}

static CFMutableDictionaryRef match_number(CFStringRef key, long value, CFMutableDictionaryRef matching)
{
    CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &value);
    if (!matching) matching = IOServiceMatching(kIOHIDDeviceKey);
    CFDictionarySetValue(matching, key, number);
    CFRelease(number);
    return matching;
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 10.0;
    double last_game = 0;
    struct mac_gamepad_frame game_last[MAC_GAMEPAD_MAXIMUM_DEVICES];
    int slot, seen_game = 0, i;
    if (seconds < 1.0) seconds = 1.0;
    filtered = argc > 2 && sscanf(argv[2], "%x:%x", &want_vendor, &want_product) == 2;
    memset(game_last, 0, sizeof(game_last));
    start_time = now();
    note_port = IONotificationPortCreate(kIOMasterPortDefault);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(note_port), kCFRunLoopDefaultMode);
    /* game pads and joysticks as they are published, and 8BitDo's devices of any kind (the adapter
       waiting for a controller is a vendor-defined page): a device that detaches is let go, and the
       one that attaches in its place is read */
    watch(match_number(CFSTR(kIOHIDPrimaryUsageKey), 5, match_number(CFSTR(kIOHIDPrimaryUsagePageKey), 1, NULL)));
    watch(match_number(CFSTR(kIOHIDPrimaryUsageKey), 4, match_number(CFSTR(kIOHIDPrimaryUsagePageKey), 1, NULL)));
    watch(match_number(CFSTR(kIOHIDVendorIDKey), 0x2dc8, NULL));
    printf("reading for %.0f seconds: plug, pair and unplug as needed, move the sticks and press buttons now\n", seconds);
    while (since() < seconds) {
        struct mac_gamepad_frame frames[MAC_GAMEPAD_MAXIMUM_DEVICES];
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, false);
        if (now() - last_game >= 0.016) {
            last_game = now();
            mac_gamepad_poll_all(frames);
            for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot) {
                if (frames[slot].connected && !game_last[slot].connected)
                    printf("t=%6.2f game: slot %d is \"%s\"%s\n", since(), slot, mac_gamepad_name_at(slot),
                        mac_gamepad_is_playstation_at(slot) ? " (PlayStation)" : "");
                if (!frames[slot].connected && game_last[slot].connected)
                    printf("t=%6.2f game: slot %d is free\n", since(), slot);
                if (frames[slot].connected && (!seen_game || memcmp(&frames[slot], &game_last[slot], sizeof(frames[0])) != 0)) {
                    char text[256];
                    describe(&frames[slot], 0, text);
                    printf("  game slot %d  %s\n", slot, text);
                }
                game_last[slot] = frames[slot];
            }
            seen_game = 1;
            fflush(stdout);
        }
    }
    printf("\nsummary\n");
    for (i = 0; i < probed_count; ++i) {
        struct probed *p = &probed[i];
        double span = p->reports > 1 ? p->last_time - p->first_time : 0;
        printf("%04x:%04x \"%s\"%s: attached at %.2f, %s", p->vendor, p->product, p->name, p->is_pad ? "" : " (not a game pad)",
            p->attached_at, p->active ? "still there" : "detached at ");
        if (!p->active) printf("%.2f", p->detached_at);
        printf(", %lu reports", p->reports);
        if (span > 0) printf(", %.1f per second", (p->reports - 1) / span);
        if (p->undecoded) printf(", %lu not decoded as a %s report", p->undecoded, mac_gamepad_report_name(p->kind));
        printf("\n");
        if (p->reports) {
            unsigned j;
            printf("  first report (%u bytes):", p->first_size);
            for (j = 0; j < p->first_size && j < sizeof(p->first); ++j) printf(" %02x", p->first[j]);
            printf("\n");
        } else if (p->is_pad)
            printf("  no reports arrived: the pad is asleep or not linked to its adapter, or sends only on change; press a button and run again\n");
    }
    for (i = 0; i < probed_count; ++i)
        if (probed[i].active) close_probed(&probed[i]);
    mac_gamepad_stop();
    return 0;
}
