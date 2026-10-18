/* The HCI USB transport (bt_usb.h). The dongle's class is Wireless Controller
   (0xE0/1/1): interface 0 has the interrupt IN endpoint (events), bulk IN and bulk
   OUT (ACL data). HCI commands are class control requests on the device
   (bmRequestType 0x20, bRequest 0, wValue 0, wIndex 0).

   On Mac OS X 10.6 Apple's driver matches the device by class, tries it, and
   lets it go when the ROM-only chip fails its set-up; its interface nubs are
   then unregistered until the configuration is set again, which is why
   SetConfiguration is issued once more through the open device. */
#include "bt_usb.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#define QUEUE 160
#define EVENT_MAX 272
#define ACL_MAX 1100

struct usb_packet {
    int kind;
    unsigned size;
    unsigned char data[BT_PACKET_MAX];
};

struct usb_ctx {
    IOUSBDeviceInterface182 **dev;
    IOUSBInterfaceInterface183 **intf;
    UInt8 pipe_int, pipe_in, pipe_out;
    unsigned char event_buf[EVENT_MAX];
    unsigned char acl_buf[ACL_MAX];
    struct usb_packet *queue;
    unsigned head, tail;
    int error;
    CFRunLoopSourceRef source;
};

static void start_event_read(struct usb_ctx *u);
static void start_acl_read(struct usb_ctx *u);

static void push(struct usb_ctx *u, int kind, const unsigned char *data, unsigned size)
{
    unsigned next = (u->head + 1) % QUEUE;
    if (next == u->tail || size > BT_PACKET_MAX) return;   /* full: the packet is lost */
    u->queue[u->head].kind = kind;
    u->queue[u->head].size = size;
    memcpy(u->queue[u->head].data, data, size);
    u->head = next;
}

static void event_done(void *refcon, IOReturn result, void *arg0)
{
    struct usb_ctx *u = (struct usb_ctx *)refcon;
    unsigned size = (unsigned)(unsigned long)arg0;
    if (result == kIOReturnSuccess) {
        if (size > 0 && size <= EVENT_MAX) push(u, BT_PACKET_EVENT, u->event_buf, size);
        start_event_read(u);
    } else
        u->error = 1;
}

static void acl_done(void *refcon, IOReturn result, void *arg0)
{
    struct usb_ctx *u = (struct usb_ctx *)refcon;
    unsigned size = (unsigned)(unsigned long)arg0;
    if (result == kIOReturnSuccess) {
        if (size > 0 && size <= ACL_MAX) push(u, BT_PACKET_ACL, u->acl_buf, size);
        start_acl_read(u);
    } else
        u->error = 1;
}

static void start_event_read(struct usb_ctx *u)
{
    if ((*u->intf)->ReadPipeAsync(u->intf, u->pipe_int, u->event_buf, sizeof(u->event_buf), event_done, u) != kIOReturnSuccess)
        u->error = 1;
}

static void start_acl_read(struct usb_ctx *u)
{
    if ((*u->intf)->ReadPipeAsync(u->intf, u->pipe_in, u->acl_buf, sizeof(u->acl_buf), acl_done, u) != kIOReturnSuccess)
        u->error = 1;
}

static int usb_send_command(void *ctx, const unsigned char *packet, unsigned size)
{
    struct usb_ctx *u = (struct usb_ctx *)ctx;
    IOUSBDevRequestTO req;
    memset(&req, 0, sizeof(req));
    req.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBDevice);
    req.bRequest = 0;
    req.wValue = 0;
    req.wIndex = 0;
    req.wLength = (UInt16)size;
    req.pData = (void *)packet;
    req.noDataTimeout = 1000;
    req.completionTimeout = 2000;
    return (*u->dev)->DeviceRequestTO(u->dev, &req) == kIOReturnSuccess ? 0 : -1;
}

static int usb_send_acl(void *ctx, const unsigned char *packet, unsigned size)
{
    struct usb_ctx *u = (struct usb_ctx *)ctx;
    return (*u->intf)->WritePipeTO(u->intf, u->pipe_out, (void *)packet, size, 1000, 2000) == kIOReturnSuccess ? 0 : -1;
}

static int usb_receive(void *ctx, unsigned char *buf, unsigned cap, unsigned *size)
{
    struct usb_ctx *u = (struct usb_ctx *)ctx;
    int kind;
    if (u->tail == u->head) return BT_PACKET_NONE;
    kind = u->queue[u->tail].kind;
    *size = u->queue[u->tail].size;
    if (*size > cap) *size = cap;
    memcpy(buf, u->queue[u->tail].data, *size);
    u->tail = (u->tail + 1) % QUEUE;
    return kind;
}

static void usb_wait(void *ctx, double seconds)
{
    (void)ctx;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, seconds, true);
}

static int usb_failed(void *ctx)
{
    return ((struct usb_ctx *)ctx)->error;
}

static void usb_close(void *ctx)
{
    struct usb_ctx *u = (struct usb_ctx *)ctx;
    if (u->source) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), u->source, kCFRunLoopDefaultMode);
        CFRelease(u->source);
    }
    if (u->intf) { (*u->intf)->USBInterfaceClose(u->intf); (*u->intf)->Release(u->intf); }
    if (u->dev) { (*u->dev)->USBDeviceClose(u->dev); (*u->dev)->Release(u->dev); }
    free(u->queue);
    free(u);
}

static io_service_t find_dongle(void)
{
    CFMutableDictionaryRef match = IOServiceMatching(kIOUSBDeviceClassName);
    SInt32 vid = BT_USB_VENDOR, pid = BT_USB_PRODUCT;
    CFNumberRef n;
    n = CFNumberCreate(NULL, kCFNumberSInt32Type, &vid);
    CFDictionarySetValue(match, CFSTR(kUSBVendorID), n);
    CFRelease(n);
    n = CFNumberCreate(NULL, kCFNumberSInt32Type, &pid);
    CFDictionarySetValue(match, CFSTR(kUSBProductID), n);
    CFRelease(n);
    return IOServiceGetMatchingService(kIOMasterPortDefault, match);   /* consumes match */
}

/* whether a Bluetooth HCI controller object of Apple's is below the dongle */
static int apple_has(io_service_t service)
{
    io_iterator_t it = 0;
    io_object_t child;
    int found = 0;
    if (IORegistryEntryCreateIterator(service, kIOServicePlane, kIORegistryIterateRecursively, &it) != KERN_SUCCESS) return 0;
    while ((child = IOIteratorNext(it)) != 0) {
        io_name_t cls;
        if (IOObjectGetClass(child, cls) == KERN_SUCCESS && strstr(cls, "Bluetooth")) found = 1;
        IOObjectRelease(child);
    }
    IOObjectRelease(it);
    return found;
}

void bt_usb_probe(struct bt_usb_probe *out)
{
    io_service_t service;
    io_registry_entry_t options;
    io_service_t hci;
    memset(out, 0, sizeof(*out));
    service = find_dongle();
    if (service) {
        out->dongle = apple_has(service) ? BT_DONGLE_APPLE : BT_DONGLE_FOUND;
        IOObjectRelease(service);
    } else
        out->dongle = BT_DONGLE_NONE;
    options = IORegistryEntryFromPath(kIOMasterPortDefault, "IODeviceTree:/options");
    if (options) {
        CFTypeRef v = IORegistryEntryCreateCFProperty(options, CFSTR("bluetoothHostControllerSwitchBehavior"), NULL, 0);
        if (v) {
            if (CFGetTypeID(v) == CFStringGetTypeID())
                out->nvram_never = CFStringCompare((CFStringRef)v, CFSTR("never"), 0) == kCFCompareEqualTo;
            else if (CFGetTypeID(v) == CFDataGetTypeID())
                out->nvram_never = CFDataGetLength((CFDataRef)v) >= 5 &&
                    memcmp(CFDataGetBytePtr((CFDataRef)v), "never", 5) == 0;
            CFRelease(v);
        }
        IOObjectRelease(options);
    }
    hci = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOBluetoothHCIController"));
    if (hci) { out->apple_controller = 1; IOObjectRelease(hci); }
}

int bt_usb_open(struct bt_transport *tr, int *dongle, char *reason, unsigned reason_size)
{
    struct usb_ctx *u;
    io_service_t service;
    IOCFPlugInInterface **plugin = NULL;
    SInt32 score;
    IOReturn r;
    io_iterator_t it;
    io_service_t is;
    IOUSBFindInterfaceRequest req;
    int attempt;
    UInt8 count = 0, i;

    *dongle = BT_DONGLE_NONE;
    reason[0] = 0;
    service = find_dongle();
    if (!service) { strncpy(reason, "The dongle is not plugged in.", reason_size - 1); return -1; }
    *dongle = apple_has(service) ? BT_DONGLE_APPLE : BT_DONGLE_FOUND;
    u = (struct usb_ctx *)calloc(1, sizeof(*u));
    if (!u) { IOObjectRelease(service); return -1; }
    u->queue = (struct usb_packet *)calloc(QUEUE, sizeof(struct usb_packet));
    if (!u->queue) { free(u); IOObjectRelease(service); return -1; }
    r = IOCreatePlugInInterfaceForService(service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score);
    IOObjectRelease(service);
    if (r != kIOReturnSuccess || !plugin) {
        sprintf(reason, "Could not reach the dongle (0x%08x).", (unsigned)r);
        free(u->queue); free(u);
        return -1;
    }
    (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID182), (LPVOID *)&u->dev);
    (*plugin)->Release(plugin);
    if (!u->dev) { strcpy(reason, "The dongle has no USB device interface."); free(u->queue); free(u); return -1; }
    r = (*u->dev)->USBDeviceOpen(u->dev);
    if (r != kIOReturnSuccess) {
        sprintf(reason, "The dongle is in use (USBDeviceOpen 0x%08x).", (unsigned)r);
        *dongle = (*dongle == BT_DONGLE_APPLE || r == kIOReturnExclusiveAccess) ? BT_DONGLE_APPLE : BT_DONGLE_BUSY;
        (*u->dev)->Release(u->dev);
        free(u->queue); free(u);
        return -1;
    }
    req.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    req.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    req.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    req.bAlternateSetting = kIOUSBFindInterfaceDontCare;
    for (attempt = 0; attempt < 2 && !u->intf; attempt++) {
        if (attempt == 1) {
            IOUSBConfigurationDescriptorPtr cd = NULL;
            (*u->dev)->GetConfigurationDescriptorPtr(u->dev, 0, &cd);
            (*u->dev)->SetConfiguration(u->dev, cd ? cd->bConfigurationValue : 1);
            usleep(500000);
        }
        if ((*u->dev)->CreateInterfaceIterator(u->dev, &req, &it) != kIOReturnSuccess) break;
        while ((is = IOIteratorNext(it)) != 0) {
            IOCFPlugInInterface **ip = NULL;
            IOUSBInterfaceInterface183 **ii = NULL;
            UInt8 number = 255;
            r = IOCreatePlugInInterfaceForService(is, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &ip, &score);
            IOObjectRelease(is);
            if (r != kIOReturnSuccess || !ip) continue;
            (*ip)->QueryInterface(ip, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID183), (LPVOID *)&ii);
            (*ip)->Release(ip);
            if (!ii) continue;
            (*ii)->GetInterfaceNumber(ii, &number);
            if (number != 0 || (*ii)->USBInterfaceOpen(ii) != kIOReturnSuccess) { (*ii)->Release(ii); continue; }
            u->intf = ii;
        }
        IOObjectRelease(it);
    }
    if (!u->intf) {
        strcpy(reason, "Could not open the dongle's Bluetooth interface.");
        *dongle = BT_DONGLE_APPLE;
        usb_close(u);
        return -1;
    }
    (*u->intf)->GetNumEndpoints(u->intf, &count);
    for (i = 1; i <= count; i++) {
        UInt8 dir, num, type, interval;
        UInt16 maxpkt;
        (*u->intf)->GetPipeProperties(u->intf, i, &dir, &num, &type, &maxpkt, &interval);
        if (type == kUSBInterrupt && dir == kUSBIn) u->pipe_int = i;
        if (type == kUSBBulk && dir == kUSBIn) u->pipe_in = i;
        if (type == kUSBBulk && dir == kUSBOut) u->pipe_out = i;
    }
    if (!u->pipe_int || !u->pipe_in || !u->pipe_out) {
        strcpy(reason, "The dongle's endpoints are not the ones expected.");
        usb_close(u);
        return -1;
    }
    if ((*u->intf)->CreateInterfaceAsyncEventSource(u->intf, &u->source) != kIOReturnSuccess) {
        strcpy(reason, "No run-loop source for the dongle.");
        usb_close(u);
        return -1;
    }
    CFRunLoopAddSource(CFRunLoopGetCurrent(), u->source, kCFRunLoopDefaultMode);
    start_event_read(u);
    start_acl_read(u);
    memset(tr, 0, sizeof(*tr));
    tr->ctx = u;
    tr->send_command = usb_send_command;
    tr->send_acl = usb_send_acl;
    tr->receive = usb_receive;
    tr->wait = usb_wait;
    tr->failed = usb_failed;
    tr->close = usb_close;
    *dongle = BT_DONGLE_OPEN;
    return 0;
}
