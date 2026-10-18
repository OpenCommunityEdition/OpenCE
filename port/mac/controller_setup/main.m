/* OpenCE Controller Setup: installs and calibrates an Xbox One S controller.
   Cocoa, IOKit HID Manager and nothing else; Mac OS X 10.5 or later, no
   garbage collection, no nibs. */
#import <Cocoa/Cocoa.h>
#include <IOKit/hid/IOHIDLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "controller_setup_config.h"
#include "controller_setup_hid.h"

static NSString *gConfigPath = nil;
static NSString *gPackagePath = nil;

static BOOL FileExists(NSString *path)
{
	struct stat st;
	return stat([path fileSystemRepresentation], &st) == 0;
}

static int DeviceNumber(IOHIDDeviceRef device, CFStringRef key)
{
	CFTypeRef value = IOHIDDeviceGetProperty(device, key);
	int n = 0;
	if (value && CFGetTypeID(value) == CFNumberGetTypeID())
		CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &n);
	return n;
}

static BOOL ProcessRunning(const char *name)
{
	FILE *p = popen("/bin/ps -axco command", "r");
	char line[256];
	BOOL found = NO;
	if (!p)
		return NO;
	while (fgets(line, sizeof line, p)) {
		size_t n = strlen(line);
		while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == ' '))
			line[--n] = 0;
		if (strcmp(line, name) == 0)
			found = YES;
	}
	pclose(p);
	return found;
}

/* ---- the live view ---- */

@interface PadView : NSView {
@public
	cs_pad_state pad;
	BOOL hasPad;
	BOOL isXbox;
	int leftDeadZone;
	int rightDeadZone;
	NSString *padName;
	NSString *sonyModel; /* "DualShock 4", "DualSense" or nil */
}
@end

@implementation PadView

- (void)dealloc
{
	[padName release];
	[sonyModel release];
	[super dealloc];
}

- (void)drawStickAt:(NSPoint)c radius:(float)r x:(int)x y:(int)y deadZone:(int)dz label:(NSString *)label
{
	NSBezierPath *path;
	float dr = r * (float)dz / 32768.0f;
	NSRect box = NSMakeRect(c.x - r, c.y - r, r * 2, r * 2);
	NSRect dzBox = NSMakeRect(c.x - dr, c.y - dr, dr * 2, dr * 2);
	NSPoint dot = NSMakePoint(c.x + r * (float)x / 32768.0f, c.y - r * (float)y / 32768.0f);
	NSDictionary *attrs = [NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:10] forKey:NSFontAttributeName];

	[[NSColor colorWithCalibratedWhite:0.93 alpha:1] set];
	[[NSBezierPath bezierPathWithOvalInRect:box] fill];
	[[NSColor grayColor] set];
	[[NSBezierPath bezierPathWithOvalInRect:box] stroke];
	[[NSColor colorWithCalibratedRed:1 green:0.6 blue:0.5 alpha:0.5] set];
	[[NSBezierPath bezierPathWithOvalInRect:dzBox] fill];
	[[NSColor redColor] set];
	[[NSBezierPath bezierPathWithOvalInRect:dzBox] stroke];
	path = [NSBezierPath bezierPath];
	[path moveToPoint:NSMakePoint(c.x - r, c.y)];
	[path lineToPoint:NSMakePoint(c.x + r, c.y)];
	[path moveToPoint:NSMakePoint(c.x, c.y - r)];
	[path lineToPoint:NSMakePoint(c.x, c.y + r)];
	[[NSColor lightGrayColor] set];
	[path stroke];
	if (hasPad && cs_hid_in_dead_zone(x, y, dz))
		[[NSColor grayColor] set];
	else
		[[NSColor colorWithCalibratedRed:0.1 green:0.5 blue:0.1 alpha:1] set];
	[[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(dot.x - 5, dot.y - 5, 10, 10)] fill];
	[label drawAtPoint:NSMakePoint(c.x - 25, c.y - r - 14) withAttributes:attrs];
}

- (void)drawBarAt:(NSRect)rect value:(int)v max:(int)m label:(NSString *)label
{
	NSRect fill = rect;
	NSDictionary *attrs = [NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:10] forKey:NSFontAttributeName];
	fill.size.height = rect.size.height * (float)v / (float)m;
	[[NSColor colorWithCalibratedWhite:0.93 alpha:1] set];
	NSRectFill(rect);
	[[NSColor colorWithCalibratedRed:0.1 green:0.5 blue:0.1 alpha:1] set];
	NSRectFill(fill);
	[[NSColor grayColor] set];
	NSFrameRect(rect);
	[label drawAtPoint:NSMakePoint(rect.origin.x + 1, rect.origin.y - 14) withAttributes:attrs];
}

- (void)drawRect:(NSRect)dirty
{
	static const char *const names[12] = { "A", "B", "X", "Y", "LB", "RB", "Vw", "Mn", "LS", "RS", "Xb", "Sh" };
	static const int dx[9] = { 0, 0, 1, 1, 1, 0, -1, -1, -1 };
	static const int dy[9] = { 0, 1, 1, 0, -1, -1, -1, 0, 1 };
	NSDictionary *attrs = [NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:10] forKey:NSFontAttributeName];
	NSDictionary *small = [NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:9] forKey:NSFontAttributeName];
	int i;

	[[NSColor colorWithCalibratedWhite:1 alpha:1] set];
	NSRectFill([self bounds]);
	[[NSColor grayColor] set];
	NSFrameRect([self bounds]);
	if (!hasPad) {
		[@"No game pad found." drawAtPoint:NSMakePoint(190, 80) withAttributes:attrs];
		return;
	}
	[self drawStickAt:NSMakePoint(65, 100) radius:42 x:pad.lx y:pad.ly deadZone:leftDeadZone label:@"Left stick"];
	[self drawStickAt:NSMakePoint(300, 100) radius:42 x:pad.rx y:pad.ry deadZone:rightDeadZone label:@"Right stick"];

	/* d-pad */
	for (i = 1; i <= 8; i += 2) {
		NSRect r = NSMakeRect(165 + dx[i] * 17 - 7, 100 + dy[i] * 17 - 7, 14, 14);
		if (pad.hat == i || (pad.hat > 0 && (pad.hat % 8 + 1 == i || pad.hat - 1 == i)))
			[[NSColor colorWithCalibratedRed:0.1 green:0.5 blue:0.1 alpha:1] set];
		else
			[[NSColor colorWithCalibratedWhite:0.85 alpha:1] set];
		NSRectFill(r);
		[[NSColor grayColor] set];
		NSFrameRect(r);
	}
	[@"D-pad" drawAtPoint:NSMakePoint(150, 44) withAttributes:attrs];

	if (isXbox != CS_LAYOUT_GENERIC) {
		[self drawBarAt:NSMakeRect(205, 60, 16, 80) value:pad.lt max:1023 label:@"LT"];
		[self drawBarAt:NSMakeRect(228, 60, 16, 80) value:pad.rt max:1023 label:@"RT"];
	}
	[padName ? padName : @"" drawAtPoint:NSMakePoint(370, 120) withAttributes:attrs];
	[(isXbox == CS_LAYOUT_XBOX ? @"Xbox layout" : isXbox == CS_LAYOUT_PLAYSTATION ? (sonyModel && [sonyModel length] ? [NSString stringWithFormat:@"PlayStation layout (%@)", sonyModel] : @"PlayStation layout") : isXbox == CS_LAYOUT_NINTENDO ? @"Nintendo layout" : @"Generic pad") drawAtPoint:NSMakePoint(370, 104) withAttributes:attrs];

	for (i = 0; i < 12; i++) {
		NSRect r = NSMakeRect(10 + i * 40, 4, 36, 18);
		if (pad.buttons & (1UL << i))
			[[NSColor colorWithCalibratedRed:0.1 green:0.5 blue:0.1 alpha:1] set];
		else
			[[NSColor colorWithCalibratedWhite:0.88 alpha:1] set];
		NSRectFill(r);
		[[NSColor grayColor] set];
		NSFrameRect(r);
		[[NSString stringWithUTF8String:names[i]] drawAtPoint:NSMakePoint(r.origin.x + 4, r.origin.y + 3) withAttributes:small];
	}
}
@end

/* ---- the controller ---- */

@interface SetupController : NSObject {
	NSWindow *window;
	PadView *padView;
	NSTextField *statusLines[4];
	NSTextField *hint;
	NSTextField *leftLabel, *rightLabel;
	NSSlider *leftSlider, *rightSlider;
	NSButton *invertBox, *installButton, *saveButton;
	IOHIDManagerRef manager;
	NSMutableArray *devices; /* NSValue of IOHIDDeviceRef */
	IOHIDDeviceRef chosen;
	int rawCount;
	BOOL installOpened;
}
- (void)start;
- (void)sliderChanged:(id)sender;
- (void)refresh:(id)sender;
- (void)readInitialValues;
- (void)handleValue:(IOHIDValueRef)value;
- (void)deviceAdded:(IOHIDDeviceRef)device;
- (void)deviceRemoved:(IOHIDDeviceRef)device;
@end

static void DeviceMatched(void *ctx, IOReturn r, void *sender, IOHIDDeviceRef device)
{
	[(SetupController *)ctx deviceAdded:device];
}

static void DeviceRemoved(void *ctx, IOReturn r, void *sender, IOHIDDeviceRef device)
{
	[(SetupController *)ctx deviceRemoved:device];
}

static void InputValue(void *ctx, IOReturn r, void *sender, IOHIDValueRef value)
{
	[(SetupController *)ctx handleValue:value];
}

/* an element usage page, for the input values asked for */
static CFMutableDictionaryRef MatchElementPage(int page)
{
	CFMutableDictionaryRef d = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
		&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFNumberRef a = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page);
	CFDictionarySetValue(d, CFSTR(kIOHIDElementUsagePageKey), a);
	CFRelease(a);
	return d;
}

static CFMutableDictionaryRef MatchPage(int page, int usage)
{
	CFMutableDictionaryRef d = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
		&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFNumberRef a = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page);
	CFNumberRef b = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
	CFDictionarySetValue(d, CFSTR(kIOHIDDeviceUsagePageKey), a);
	CFDictionarySetValue(d, CFSTR(kIOHIDDeviceUsageKey), b);
	CFRelease(a);
	CFRelease(b);
	return d;
}

static CFMutableDictionaryRef MatchVendorProduct(int vendor, int product)
{
	CFMutableDictionaryRef d = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
		&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFNumberRef a = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendor);
	CFNumberRef b = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &product);
	CFDictionarySetValue(d, CFSTR(kIOHIDVendorIDKey), a);
	CFDictionarySetValue(d, CFSTR(kIOHIDProductIDKey), b);
	CFRelease(a);
	CFRelease(b);
	return d;
}

@implementation SetupController

- (NSTextField *)labelAt:(NSRect)frame text:(NSString *)text bold:(BOOL)bold
{
	NSTextField *t = [[[NSTextField alloc] initWithFrame:frame] autorelease];
	[t setEditable:NO];
	[t setBordered:NO];
	[t setDrawsBackground:NO];
	[t setSelectable:NO];
	[t setStringValue:text];
	[t setFont:bold ? [NSFont boldSystemFontOfSize:13] : [NSFont systemFontOfSize:12]];
	[[window contentView] addSubview:t];
	return t;
}

- (NSButton *)buttonAt:(NSRect)frame title:(NSString *)title action:(SEL)action
{
	NSButton *b = [[[NSButton alloc] initWithFrame:frame] autorelease];
	[b setTitle:title];
	[b setBezelStyle:NSRoundedBezelStyle];
	[b setTarget:self];
	[b setAction:action];
	[[window contentView] addSubview:b];
	return b;
}

- (NSSlider *)sliderAt:(NSRect)frame
{
	NSSlider *s = [[[NSSlider alloc] initWithFrame:frame] autorelease];
	[s setMinValue:0];
	[s setMaxValue:16000];
	[s setIntValue:9000];
	[s setContinuous:YES];
	[s setTarget:self];
	[s setAction:@selector(sliderChanged:)];
	[[window contentView] addSubview:s];
	return s;
}

- (void)start
{
	int i;
	NSRect frame = NSMakeRect(0, 0, 520, 420);
	CFMutableArrayRef matches;
	CFMutableDictionaryRef m;

	devices = [[NSMutableArray alloc] init];
	window = [[NSWindow alloc] initWithContentRect:frame
		styleMask:NSTitledWindowMask | NSClosableWindowMask
		backing:NSBackingStoreBuffered defer:NO];
	[window setTitle:@"PowerSpark Controller Setup"];
	[window setReleasedWhenClosed:NO];
	[window setDelegate:(id)self];

	[self labelAt:NSMakeRect(10, 392, 400, 18) text:@"Xbox One S controller setup" bold:YES];
	for (i = 0; i < 4; i++)
		statusLines[i] = [self labelAt:NSMakeRect(10, 368 - i * 20, 360, 18) text:@"" bold:NO];
	installButton = [self buttonAt:NSMakeRect(380, 346, 130, 32) title:@"Install Driver..." action:@selector(install:)];
	hint = [self labelAt:NSMakeRect(10, 284, 500, 32) text:@"" bold:NO];
	[[hint cell] setWraps:YES];

	padView = [[[PadView alloc] initWithFrame:NSMakeRect(10, 122, 500, 160)] autorelease];
	padView->leftDeadZone = padView->rightDeadZone = 9000;
	[[window contentView] addSubview:padView];

	[self labelAt:NSMakeRect(10, 94, 150, 18) text:@"Left stick dead zone" bold:NO];
	leftSlider = [self sliderAt:NSMakeRect(160, 94, 280, 20)];
	leftLabel = [self labelAt:NSMakeRect(450, 94, 60, 18) text:@"9000" bold:NO];
	[self labelAt:NSMakeRect(10, 70, 150, 18) text:@"Right stick dead zone" bold:NO];
	rightSlider = [self sliderAt:NSMakeRect(160, 70, 280, 20)];
	rightLabel = [self labelAt:NSMakeRect(450, 70, 60, 18) text:@"9000" bold:NO];
	invertBox = [[[NSButton alloc] initWithFrame:NSMakeRect(10, 44, 300, 18)] autorelease];
	[invertBox setButtonType:NSSwitchButton];
	[invertBox setTitle:@"Invert look (right stick Y)"];
	[[window contentView] addSubview:invertBox];

	[self buttonAt:NSMakeRect(10, 6, 90, 32) title:@"Skip" action:@selector(skip:)];
	saveButton = [self buttonAt:NSMakeRect(410, 6, 100, 32) title:@"Save" action:@selector(save:)];
	[saveButton setKeyEquivalent:@"\r"];

	/* preload current values from the config */
	if (gConfigPath) {
		char v[32];
		const char *p = [gConfigPath fileSystemRepresentation];
		if (cs_config_get(p, "input.gamepad_left_dead_zone", v, sizeof v) == 1)
			[leftSlider setIntValue:cs_config_clamp_dead_zone(atoi(v))];
		if (cs_config_get(p, "input.gamepad_right_dead_zone", v, sizeof v) == 1)
			[rightSlider setIntValue:cs_config_clamp_dead_zone(atoi(v))];
		if (cs_config_get(p, "input.gamepad_invert_look", v, sizeof v) == 1)
			[invertBox setState:strcmp(v, "true") == 0 ? NSOnState : NSOffState];
	}
	[self sliderChanged:nil];

	manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	matches = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	m = MatchPage(CS_PAGE_GENERIC_DESKTOP, 5); /* game pad */
	CFArrayAppendValue(matches, m);
	CFRelease(m);
	m = MatchPage(CS_PAGE_GENERIC_DESKTOP, 4); /* joystick */
	CFArrayAppendValue(matches, m);
	CFRelease(m);
	m = MatchVendorProduct(CS_VENDOR_MICROSOFT, CS_PRODUCT_XBOX_ONE_S_RAW);
	CFArrayAppendValue(matches, m);
	CFRelease(m);
	IOHIDManagerSetDeviceMatchingMultiple(manager, matches);
	CFRelease(matches);
	IOHIDManagerRegisterDeviceMatchingCallback(manager, DeviceMatched, self);
	IOHIDManagerRegisterDeviceRemovalCallback(manager, DeviceRemoved, self);
	/* only the sticks, triggers, hat and buttons: a pad may have hundreds
	of vendor elements (8BitDo's adapter, as a DualShock 4, has over a
	thousand, and changes some 250 times a second) */
	matches = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	m = MatchElementPage(CS_PAGE_GENERIC_DESKTOP);
	CFArrayAppendValue(matches, m);
	CFRelease(m);
	m = MatchElementPage(0x02); /* simulation controls */
	CFArrayAppendValue(matches, m);
	CFRelease(m);
	m = MatchElementPage(CS_PAGE_BUTTON);
	CFArrayAppendValue(matches, m);
	CFRelease(m);
	IOHIDManagerSetInputValueMatchingMultiple(manager, matches);
	CFRelease(matches);
	IOHIDManagerRegisterInputValueCallback(manager, InputValue, self);
	NSLog(@"controller setup: HID manager open");
	IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);

	[self refresh:nil];
	[NSTimer scheduledTimerWithTimeInterval:2.0 target:self selector:@selector(refresh:) userInfo:nil repeats:YES];
	[window center];
	[window makeKeyAndOrderFront:nil];
}

- (BOOL)isRaw:(IOHIDDeviceRef)d
{
	return DeviceNumber(d, CFSTR(kIOHIDVendorIDKey)) == CS_VENDOR_MICROSOFT
		&& DeviceNumber(d, CFSTR(kIOHIDProductIDKey)) == CS_PRODUCT_XBOX_ONE_S_RAW;
}

- (BOOL)isXboxPad:(IOHIDDeviceRef)d
{
	return DeviceNumber(d, CFSTR(kIOHIDVendorIDKey)) == CS_VENDOR_MICROSOFT
		&& DeviceNumber(d, CFSTR(kIOHIDProductIDKey)) == CS_PRODUCT_XBOX_VIRTUAL;
}

- (BOOL)isPad:(IOHIDDeviceRef)d
{
	return IOHIDDeviceConformsTo(d, CS_PAGE_GENERIC_DESKTOP, 5)
		|| IOHIDDeviceConformsTo(d, CS_PAGE_GENERIC_DESKTOP, 4);
}

- (int)layoutOf:(IOHIDDeviceRef)d
{
	if ([self isXboxPad:d])
		return CS_LAYOUT_XBOX;
	if (DeviceNumber(d, CFSTR(kIOHIDVendorIDKey)) == CS_VENDOR_SONY)
		return CS_LAYOUT_PLAYSTATION;
	if (DeviceNumber(d, CFSTR(kIOHIDVendorIDKey)) == CS_VENDOR_NINTENDO)
		return CS_LAYOUT_NINTENDO;
	return CS_LAYOUT_GENERIC;
}

/* Chooses the Xbox virtual pad if there is one, else the first other pad. */
- (void)choose
{
	IOHIDDeviceRef best = NULL;
	unsigned i;
	BOOL bestXbox = NO;

	rawCount = 0;
	for (i = 0; i < [devices count]; i++) {
		IOHIDDeviceRef d = (IOHIDDeviceRef)[[devices objectAtIndex:i] pointerValue];
		if ([self isRaw:d]) {
			rawCount++;
			continue;
		}
		if (![self isPad:d])
			continue;
		if (!best || (!bestXbox && [self isXboxPad:d])) {
			best = d;
			bestXbox = [self isXboxPad:d];
		}
	}
	if (best != chosen) {
		chosen = best;
		cs_pad_clear(&padView->pad, best ? [self layoutOf:best] : CS_LAYOUT_GENERIC);
		padView->isXbox = best ? [self layoutOf:best] : CS_LAYOUT_GENERIC;
		padView->hasPad = best != NULL;
		[padView->padName release];
		padView->padName = nil;
		[padView->sonyModel release];
		padView->sonyModel = nil;
		if (best) {
			padView->sonyModel = [[NSString stringWithUTF8String:cs_sony_model(DeviceNumber(best, CFSTR(kIOHIDVendorIDKey)),
				DeviceNumber(best, CFSTR(kIOHIDProductIDKey)))] retain];
			CFTypeRef n = IOHIDDeviceGetProperty(best, CFSTR(kIOHIDProductKey));
			if (n && CFGetTypeID(n) == CFStringGetTypeID())
				padView->padName = [[NSString stringWithString:(NSString *)n] retain];
			NSLog(@"controller setup: chose %@ (layout %d); reading its values", padView->padName, (int)padView->isXbox);
			[self readInitialValues];
			NSLog(@"controller setup: values read");
		}
	}
	[padView setNeedsDisplay:YES];
}

- (void)readInitialValues
{
	CFArrayRef elements = IOHIDDeviceCopyMatchingElements(chosen, NULL, kIOHIDOptionsTypeNone);
	CFIndex i;
	if (!elements)
		return;
	for (i = 0; i < CFArrayGetCount(elements); i++) {
		IOHIDElementRef e = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
		IOHIDValueRef v = NULL;
		IOHIDElementType type = IOHIDElementGetType(e);
		uint32_t page = IOHIDElementGetUsagePage(e);
		/* the sticks, triggers, hat and buttons only: a feature or output
		element's value is a report request to the device, and a pad may
		have a thousand vendor elements (8BitDo's adapter, as a DualShock 4) */
		if (type != kIOHIDElementTypeInput_Misc && type != kIOHIDElementTypeInput_Button &&
			type != kIOHIDElementTypeInput_Axis)
			continue;
		if (page != CS_PAGE_GENERIC_DESKTOP && page != 0x02 && page != CS_PAGE_BUTTON)
			continue;
		if (IOHIDDeviceGetValue(chosen, e, &v) == kIOReturnSuccess && v)
			[self handleValue:v];
	}
	CFRelease(elements);
}

- (void)deviceAdded:(IOHIDDeviceRef)device
{
	NSLog(@"controller setup: device %04x:%04x added", DeviceNumber(device, CFSTR(kIOHIDVendorIDKey)),
		DeviceNumber(device, CFSTR(kIOHIDProductIDKey)));
	[devices addObject:[NSValue valueWithPointer:device]];
	[self choose];
	[self refresh:nil];
}

- (void)deviceRemoved:(IOHIDDeviceRef)device
{
	[devices removeObject:[NSValue valueWithPointer:device]];
	if (chosen == device)
		chosen = NULL;
	[self choose];
	[self refresh:nil];
}

- (void)handleValue:(IOHIDValueRef)value
{
	IOHIDElementRef e = IOHIDValueGetElement(value);
	IOHIDDeviceRef d = IOHIDElementGetDevice(e);
	if (!chosen || d != chosen)
		return;
	if (cs_hid_apply(&padView->pad, IOHIDElementGetUsagePage(e), IOHIDElementGetUsage(e),
			(int)IOHIDValueGetIntegerValue(value), (int)IOHIDElementGetLogicalMin(e),
			(int)IOHIDElementGetLogicalMax(e)))
		[padView setNeedsDisplay:YES];
}

- (void)setStatus:(int)i ok:(BOOL)ok text:(NSString *)text
{
	[statusLines[i] setStringValue:[NSString stringWithFormat:@"%@ %@", ok ? @"[x]" : @"[ ]", text]];
}

- (void)refresh:(id)sender
{
	BOOL installed = FileExists(@"/usr/local/bin/xboxd")
		&& FileExists(@"/usr/local/share/XboxOnePad/XboxOnePad.kext")
		&& FileExists(@"/Library/LaunchDaemons/net.xboxcontroller.xboxd.plist");
	BOOL running = ProcessRunning("xboxd");
	BOOL virt = NO;
	unsigned i;
	NSString *h;

	for (i = 0; i < [devices count]; i++)
		if ([self isXboxPad:(IOHIDDeviceRef)[[devices objectAtIndex:i] pointerValue]])
			virt = YES;
	[self setStatus:0 ok:installed text:@"Driver installed"];
	[self setStatus:1 ok:running text:@"Driver running (xboxd)"];
	[self setStatus:2 ok:virt text:@"Virtual game pad visible"];
	[self setStatus:3 ok:(rawCount > 0 || virt) text:@"Controller connected over Bluetooth"];
	[installButton setTitle:installed ? @"Reinstall Driver..." : @"Install Driver..."];

	if (virt)
		h = @"Move the sticks and press buttons to check them, then set the dead zones.";
	else if (chosen)
		h = @"No Xbox driver pad, but another game pad works. Move it to check, then save.";
	else if (!installed)
		h = @"Install the driver first. Never update the controller's firmware.";
	else if (!running)
		h = @"The driver is installed but not running. Restart the Mac, or run the installer again.";
	else if (rawCount == 0)
		h = @"Pair the controller in System Preferences > Bluetooth (hold the pairing button until the Xbox light flashes), then press the Xbox button.";
	else
		h = @"The controller is connected but the pad is not visible yet. Wait a moment.";
	[hint setStringValue:h];
	[leftSlider setEnabled:chosen != NULL];
	[rightSlider setEnabled:chosen != NULL];
	[invertBox setEnabled:chosen != NULL];
}

- (void)sliderChanged:(id)sender
{
	padView->leftDeadZone = [leftSlider intValue];
	padView->rightDeadZone = [rightSlider intValue];
	[leftLabel setStringValue:[NSString stringWithFormat:@"%d", [leftSlider intValue]]];
	[rightLabel setStringValue:[NSString stringWithFormat:@"%d", [rightSlider intValue]]];
	[padView setNeedsDisplay:YES];
}

- (NSString *)findPackage
{
	NSFileManager *fm = [NSFileManager defaultManager];
	NSString *dir;
	NSArray *names;
	unsigned i;

	if (gPackagePath && FileExists(gPackagePath))
		return gPackagePath;
	dir = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
	names = [fm directoryContentsAtPath:dir];
	for (i = 0; i < [names count]; i++) {
		NSString *n = [names objectAtIndex:i];
		if ([n hasPrefix:@"XboxOneController"] && [n hasSuffix:@".pkg"])
			return [dir stringByAppendingPathComponent:n];
	}
	return nil;
}

- (void)install:(id)sender
{
	NSString *pkg = [self findPackage];
	if (!pkg) {
		NSRunAlertPanel(@"Driver package not found",
			@"Put XboxOneController-1.0.pkg next to this app, or start the app with --package PATH.",
			@"OK", nil, nil);
		return;
	}
	/* The system Installer asks for the administrator password itself. */
	[NSTask launchedTaskWithLaunchPath:@"/usr/bin/open" arguments:[NSArray arrayWithObject:pkg]];
	installOpened = YES;
	[hint setStringValue:@"The Installer is open. Finish it, then come back here: this window checks again every 2 seconds."];
}

- (void)finish:(BOOL)skipOnly
{
	int rc;
	if (!gConfigPath) {
		NSRunAlertPanel(@"No config file", @"The app was started without --config PATH, so nothing can be saved.", @"OK", nil, nil);
		return;
	}
	if (skipOnly)
		rc = cs_config_save_skip([gConfigPath fileSystemRepresentation]);
	else
		rc = cs_config_save_calibration([gConfigPath fileSystemRepresentation],
			[leftSlider intValue], [rightSlider intValue], [invertBox state] == NSOnState);
	if (rc != 0) {
		NSRunAlertPanel(@"Could not write the config file", @"%@", @"OK", nil, nil, gConfigPath);
		return;
	}
	exit(0);
}

- (void)save:(id)sender { [self finish:NO]; }
- (void)skip:(id)sender { [self finish:YES]; }

- (void)windowWillClose:(NSNotification *)n
{
	exit(1); /* closed without saving: the launcher asks again next time */
}

@end

int main(int argc, char **argv)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SetupController *controller;
	int i;

	for (i = 1; i + 1 < argc; i++) {
		if (strcmp(argv[i], "--config") == 0)
			gConfigPath = [[NSString stringWithUTF8String:argv[++i]] retain];
		else if (strcmp(argv[i], "--package") == 0)
			gPackagePath = [[NSString stringWithUTF8String:argv[++i]] retain];
	}
	[NSApplication sharedApplication];
	controller = [[SetupController alloc] init];
	[controller start];
	[NSApp activateIgnoringOtherApps:YES];
	[NSApp run];
	[pool release];
	return 0;
}
