/* OpenCE Controller Setup: shows how to connect a modern game controller
   (with the 8BitDo Wireless USB Adapter 2), checks that its input arrives,
   and calibrates its dead zones. Cocoa, IOKit HID Manager and Carbon's text
   measuring; Mac OS X 10.5 or later, no garbage collection, no nibs. */
#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controller_setup_config.h"
#include "controller_setup_hid.h"
#include "../mac_gamepad_report.h"

static NSString *gConfigPath = nil;

/* the instructions: short, in the order a person does them */
static NSString *const kHeading = @"Connect a modern controller with the 8BitDo Wireless USB Adapter 2";
static NSString *const kInstructions =
	@"Works with PS5 DualSense, PS4 DualShock 4, Xbox One/Series, Switch Pro and 8BitDo pads.\n"
	@"1. Plug the adapter into a USB port. A USB data cable is the most reliable way; a charge-only cable is not seen at all.\n"
	@"2. Press the pair button on the adapter. Its light blinks fast.\n"
	@"3. Put the controller in pairing mode: hold Create + PS (DualSense) or Share + PS (DualShock 4) for 3 seconds; "
	@"on an Xbox pad press the Xbox button, then hold its pair button for 3 seconds; on a Switch Pro hold Sync for 3 seconds.\n"
	@"4. Best on these Macs: Dinput mode. Hold Create (Share on a DualShock 4, View on Xbox) + D-pad Left for 3 seconds until the light blinks twice, then pair again.\n"
	@"5. A solid light means it is connected. Next time, just press PS, Xbox or any button.\n"
	@"Nothing happens? The adapter unplugs and plugs itself in again a few times while it pairs: wait a few seconds, and try another port or cable. "
	@"Some pads need a firmware update first: use 8BitDo's updater on a newer computer.\n"
	@"Not covered: wired USB pads and the Xbox 360 pad driver.";

/* the live status line */
static NSString *const kStatusNoPad = @"No game controller found yet: plug in the adapter (or the pad) and follow the steps above.";
static NSString *const kStatusWaiting = @"No input received yet: press a button on the controller.";
static NSString *const kStatusReceiving = @"Receiving input";
static NSString *const kStatusSilent = @"Controller found but sending nothing: make sure it's paired (LED solid) and press a button.";
static NSString *const kStatusQuiet = @"Receiving input earlier, quiet now: press a button.";
static NSString *const kStatusAsleep = @"Controller asleep or out of range: press the PS button to reconnect.";
static NSString *const kStatusAdapterIdle = @"Adapter found, waiting for a controller: pair it now (adapter button, then Create + PS).";
#define SILENCE_SECONDS 4.0

static int DeviceNumber(IOHIDDeviceRef device, CFStringRef key)
{
	CFTypeRef value = IOHIDDeviceGetProperty(device, key);
	int n = 0;
	if (value && CFGetTypeID(value) == CFNumberGetTypeID())
		CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &n);
	return n;
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
	NSBox *box;
	NSTextField *headingLabel, *instructions, *status;
	NSTextField *leftLabel, *rightLabel;
	NSSlider *leftSlider, *rightSlider;
	NSButton *invertBox, *saveButton, *skipButton;
	IOHIDManagerRef manager;
	NSMutableArray *devices; /* NSValue of IOHIDDeviceRef */
	NSMutableDictionary *reportBuffers; /* NSValue of IOHIDDeviceRef -> NSMutableData */
	IOHIDDeviceRef chosen;
	int reportKind;       /* MAC_PAD_REPORT_*: the chosen pad's reports are decoded */
	BOOL asleep;          /* the chosen pad's reports stopped (see refresh) */
	BOOL reportSeen;      /* a report of the chosen pad decoded: its elements are then ignored */
	unsigned long inputCount;
	CFAbsoluteTime chosenAt, lastInput;
}
- (void)buildUI;
- (void)start;
- (void)dumpText;
- (void)sliderChanged:(id)sender;
- (void)refresh:(id)sender;
- (void)readInitialValues;
- (void)handleValue:(IOHIDValueRef)value;
- (void)applyValue:(IOHIDValueRef)value counted:(BOOL)counted;
- (void)handleReport:(const uint8_t *)report length:(CFIndex)length id:(uint32_t)reportID device:(IOHIDDeviceRef)device;
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

static void InputReport(void *ctx, IOReturn r, void *sender, IOHIDReportType type, uint32_t reportID,
	uint8_t *report, CFIndex length)
{
	if (r == kIOReturnSuccess && type == kIOHIDReportTypeInput)
		[(SetupController *)ctx handleReport:report length:length id:reportID device:(IOHIDDeviceRef)sender];
}

static NSSize ThemeSizeOne(NSString *text, ThemeFontID font, float width, BOOL wrap);

/* the size theme text takes (what the launcher measures with), wrapped to width when wrap is set; each line of
   the text (split at a newline) is measured on its own, as the measuring does not break at a newline */
static NSSize ThemeSize(NSString *text, ThemeFontID font, float width, BOOL wrap)
{
	NSArray *lines = [text componentsSeparatedByString:@"\n"];
	NSSize total = NSMakeSize(0, 0);
	unsigned i;
	for (i = 0; i < [lines count]; i++) {
		NSSize one = ThemeSizeOne([lines objectAtIndex:i], font, width, wrap);
		if (one.width > total.width)
			total.width = one.width;
		total.height += one.height;
	}
	return total;
}

/* the size theme text takes (what the launcher measures with), wrapped to width when wrap is set */
static NSSize ThemeSizeOne(NSString *text, ThemeFontID font, float width, BOOL wrap)
{
	Point bounds;
	SInt16 baseline;

	/* one line: its own width and height. (GetThemeTextDimensions measures one line, as the launcher uses it, so a
	   wrapped text is broken into lines here, a word at a time.) */
	bounds.h = 0;
	bounds.v = 0;
	if (GetThemeTextDimensions((CFStringRef)(wrap && [text length] == 0 ? @" " : text), font, kThemeStateActive, false,
			&bounds, &baseline) != noErr || bounds.h <= 0 || bounds.v <= 0) {
		/* (no metrics: a generous guess) */
		float chars = (float)[text length];
		float perLine = width > 0 ? width / 6.0f : chars;
		return NSMakeSize(wrap ? width : chars * 7.0f, 15.0f * (float)(1 + (int)(chars / perLine)));
	}
	if (!wrap || bounds.h <= width)
		return NSMakeSize(bounds.h, bounds.v);
	{
		NSArray *words = [text componentsSeparatedByString:@" "];
		NSString *line = @"";
		float widest = 0;
		int lines = 1;
		unsigned i;
		for (i = 0; i < [words count]; i++) {
			NSString *tryLine = [line length] ? [line stringByAppendingFormat:@" %@", [words objectAtIndex:i]] : [words objectAtIndex:i];
			Point b2;
			b2.h = b2.v = 0;
			GetThemeTextDimensions((CFStringRef)tryLine, font, kThemeStateActive, false, &b2, &baseline);
			if (b2.h > width && [line length]) {
				Point b3;
				b3.h = b3.v = 0;
				GetThemeTextDimensions((CFStringRef)line, font, kThemeStateActive, false, &b3, &baseline);
				if (b3.h > widest)
					widest = b3.h;
				line = [words objectAtIndex:i];
				lines++;
			} else
				line = tryLine;
		}
		{
			Point b4;
			b4.h = b4.v = 0;
			GetThemeTextDimensions((CFStringRef)line, font, kThemeStateActive, false, &b4, &baseline);
			if (b4.h > widest)
				widest = b4.h;
		}
		return NSMakeSize(widest, (float)(lines * bounds.v));
	}
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

/* every device of a vendor (8BitDo's adapter waiting for a controller is not a game pad: it shows only as a
   vendor-defined page, so it is matched by its vendor to say that it is there) */
static CFMutableDictionaryRef MatchVendor(int vendor)
{
	CFMutableDictionaryRef d = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
		&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFNumberRef a = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendor);
	CFDictionarySetValue(d, CFSTR(kIOHIDVendorIDKey), a);
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

@implementation SetupController

- (NSTextField *)labelIn:(NSView *)parent frame:(NSRect)frame text:(NSString *)text bold:(BOOL)bold
{
	NSTextField *t = [[[NSTextField alloc] initWithFrame:frame] autorelease];
	[t setEditable:NO];
	[t setBordered:NO];
	[t setDrawsBackground:NO];
	[t setSelectable:NO];
	[t setStringValue:text];
	[t setFont:bold ? [NSFont boldSystemFontOfSize:13] : [NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
	[[t cell] setWraps:YES];
	[parent addSubview:t];
	return t;
}

- (NSTextField *)labelAt:(NSRect)frame text:(NSString *)text bold:(BOOL)bold
{
	return [self labelIn:[window contentView] frame:frame text:text bold:bold];
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

/* The height the status line needs: the tallest of its texts, wrapped, with a line to spare for Cocoa's own
   wrapping, which is a little wider than the theme's. */
- (float)statusHeight
{
	NSString *all[7];
	float h = 0;
	int i;
	all[0] = kStatusNoPad; all[1] = kStatusWaiting; all[2] = kStatusReceiving; all[3] = kStatusSilent; all[4] = kStatusQuiet; all[5] = kStatusAdapterIdle; all[6] = kStatusAsleep;
	for (i = 0; i < 7; i++) {
		float t = ThemeSize(all[i], kThemeSmallSystemFont, 490, YES).height;
		if (t > h)
			h = t;
	}
	/* (two lines at least: Cocoa breaks a line a little earlier than the theme's measure) */
	return (h < 26 ? 26 : h) + 4;
}

- (void)buildUI
{
	/* the window, from the bottom up: buttons, invert, sliders, the pad view (160), the status line, the instructions */
	const float margin = 10, inner = 474;
	float instrH = ThemeSize(kInstructions, kThemeSmallSystemFont, inner - 6, YES).height + 14;
	float headH = ThemeSize(kHeading, kThemeEmphasizedSystemFont, inner - 6, YES).height + 4;
	float statusH = [self statusHeight];
	float boxH = headH + instrH + 12 + 8;
	float statusY = 286, boxY = statusY + statusH + 6;
	float height = boxY + boxH + 10;
	NSRect frame = NSMakeRect(0, 0, 520, height);

	devices = [[NSMutableArray alloc] init];
	reportBuffers = [[NSMutableDictionary alloc] init];
	window = [[NSWindow alloc] initWithContentRect:frame
		styleMask:NSTitledWindowMask | NSClosableWindowMask
		backing:NSBackingStoreBuffered defer:NO];
	[window setTitle:@"PowerSpark Controller Setup"];
	[window setReleasedWhenClosed:NO];
	[window setDelegate:(id)self];

	box = [[[NSBox alloc] initWithFrame:NSMakeRect(margin, boxY, 500, boxH)] autorelease];
	[box setTitlePosition:NSNoTitle];
	[box setContentViewMargins:NSMakeSize(6, 6)];
	[[window contentView] addSubview:box];
	headingLabel = [self labelIn:[box contentView] frame:NSMakeRect(2, boxH - 12 - headH - 4, inner, headH) text:kHeading bold:YES];
	instructions = [self labelIn:[box contentView] frame:NSMakeRect(2, boxH - 12 - headH - 4 - instrH, inner, instrH) text:kInstructions bold:NO];
	status = [self labelAt:NSMakeRect(margin, statusY, 500, statusH) text:kStatusNoPad bold:NO];

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

	skipButton = [self buttonAt:NSMakeRect(10, 6, 90, 32) title:@"Skip" action:@selector(skip:)];
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
}

/* --dump-text: every text and control with its size and the size its text needs (GetThemeTextDimensions, as
   the launcher measures), and a CUT OFF line for each that does not fit. Returns the number cut off. */
static int DumpView(NSView *view, int depth, NSString *indent)
{
	NSArray *subs = [view subviews];
	unsigned i;
	int bad = 0;
	for (i = 0; i < [subs count]; i++) {
		NSView *v = [subs objectAtIndex:i];
		NSRect f = [v frame];
		NSString *text = nil;
		ThemeFontID font = kThemeSmallSystemFont;
		BOOL wrap = NO, clipped = NO;
		NSSize need = NSMakeSize(0, 0);
		if ([v isKindOfClass:[NSTextField class]]) {
			text = [(NSTextField *)v stringValue];
			font = [[(NSTextField *)v font] pointSize] > 12 ? kThemeEmphasizedSystemFont : kThemeSmallSystemFont;
			wrap = YES;
			need = ThemeSize(text, font, f.size.width - 4, YES);
			clipped = need.height > f.size.height;
		} else if ([v isKindOfClass:[NSButton class]]) {
			text = [(NSButton *)v title];
			font = kThemeSystemFont;
			need = ThemeSize(text, font, 0, NO);
			need.height = 0; /* (a button's height is the bezel's) */
			clipped = need.width + ([(NSButton *)v bezelStyle] == NSRoundedBezelStyle ? 24 : 20) > f.size.width;
		}
		printf("%s%s (%d,%d) %dx%d", [indent UTF8String], [[v className] UTF8String], (int)f.origin.x, (int)f.origin.y,
			(int)f.size.width, (int)f.size.height);
		if (text)
			printf(" needs %dx%d%s \"%s\"", (int)need.width, (int)need.height, wrap ? " wrapped" : "", [text UTF8String]);
		if (clipped) {
			printf("  <-- CUT OFF");
			bad++;
		}
		printf("\n");
		if (!NSContainsRect([view bounds], f)) {
			printf("%s  <-- OUTSIDE ITS PARENT\n", [indent UTF8String]);
			bad++;
		}
		bad += DumpView(v, depth + 1, [indent stringByAppendingString:@"  "]);
	}
	return bad;
}

- (void)dumpText
{
	NSString *all[7];
	int i, bad;
	NSRect f = [[window contentView] frame];
	all[0] = kStatusNoPad; all[1] = kStatusWaiting; all[2] = kStatusReceiving; all[3] = kStatusSilent; all[4] = kStatusQuiet; all[5] = kStatusAdapterIdle; all[6] = kStatusAsleep;
	printf("window %dx%d\n", (int)f.size.width, (int)f.size.height);
	bad = DumpView([window contentView], 0, @"  ");
	for (i = 0; i < 7; i++) {
		NSSize n = ThemeSize(all[i], kThemeSmallSystemFont, 496, YES);
		NSRect sf = [status frame];
		printf("status %d needs %dx%d in %dx%d%s: %s\n", i, (int)n.width, (int)n.height, (int)sf.size.width, (int)sf.size.height,
			n.height > sf.size.height ? "  <-- CUT OFF" : "", [all[i] UTF8String]);
		if (n.height > sf.size.height)
			bad++;
	}
	printf("%s\n", bad ? "CUT OFF: yes" : "nothing cut off");
}

- (void)start
{
	CFMutableArrayRef matches;
	CFMutableDictionaryRef m;

	[self buildUI];
	manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	matches = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	m = MatchPage(CS_PAGE_GENERIC_DESKTOP, 5); /* game pad */
	CFArrayAppendValue(matches, m);
	CFRelease(m);
	m = MatchPage(CS_PAGE_GENERIC_DESKTOP, 4); /* joystick */
	CFArrayAppendValue(matches, m);
	CFRelease(m);
	m = MatchVendor(CS_VENDOR_8BITDO);
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
	[NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(refresh:) userInfo:nil repeats:YES];
	[window center];
	[window makeKeyAndOrderFront:nil];
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

/* Whether 8BitDo's adapter is plugged in without a game pad's identity: it is waiting for a controller to pair
   (2dc8:3107, "IDLE") or is between two of its identities. */
- (BOOL)adapterWaiting
{
	unsigned i;
	for (i = 0; i < [devices count]; i++) {
		IOHIDDeviceRef d = (IOHIDDeviceRef)[[devices objectAtIndex:i] pointerValue];
		if (DeviceNumber(d, CFSTR(kIOHIDVendorIDKey)) == CS_VENDOR_8BITDO && ![self isPad:d])
			return YES;
	}
	return NO;
}

/* Chooses the first game pad (one that an Xbox driver made, if there is one, before any other). */
- (void)choose
{
	IOHIDDeviceRef best = NULL;
	unsigned i;
	BOOL bestXbox = NO;

	for (i = 0; i < [devices count]; i++) {
		IOHIDDeviceRef d = (IOHIDDeviceRef)[[devices objectAtIndex:i] pointerValue];
		if (![self isPad:d])
			continue;
		if (!best || (!bestXbox && [self isXboxPad:d])) {
			best = d;
			bestXbox = [self isXboxPad:d];
		}
	}
	if (best != chosen) {
		chosen = best;
		reportSeen = NO;
		asleep = NO;
		reportKind = best ? mac_gamepad_report_kind((unsigned)DeviceNumber(best, CFSTR(kIOHIDVendorIDKey)),
			(unsigned)DeviceNumber(best, CFSTR(kIOHIDProductIDKey))) : MAC_PAD_REPORT_NONE;
		inputCount = 0;
		chosenAt = CFAbsoluteTimeGetCurrent();
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
			NSLog(@"controller setup: chose %@ (layout %d, reports %d); reading its values", padView->padName,
				(int)padView->isXbox, reportKind);
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
			[self applyValue:v counted:NO];
	}
	CFRelease(elements);
}

- (void)deviceAdded:(IOHIDDeviceRef)device
{
	unsigned vendor = (unsigned)DeviceNumber(device, CFSTR(kIOHIDVendorIDKey));
	unsigned product = (unsigned)DeviceNumber(device, CFSTR(kIOHIDProductIDKey));
	NSLog(@"controller setup: device %04x:%04x added", vendor, product);
	[devices addObject:[NSValue valueWithPointer:device]];
	/* a Sony pad's input reports, which carry the whole state (the game reads these, mac_gamepad_report.c) */
	if (mac_gamepad_report_kind(vendor, product) != MAC_PAD_REPORT_NONE) {
		NSMutableData *buffer = [NSMutableData dataWithLength:256];
		[reportBuffers setObject:buffer forKey:[NSValue valueWithPointer:device]];
		IOHIDDeviceRegisterInputReportCallback(device, (uint8_t *)[buffer mutableBytes], 256, InputReport, self);
	}
	[self choose];
	[self refresh:nil];
}

- (void)deviceRemoved:(IOHIDDeviceRef)device
{
	NSValue *key = [NSValue valueWithPointer:device];
	if ([reportBuffers objectForKey:key]) {
		IOHIDDeviceRegisterInputReportCallback(device, (uint8_t *)[[reportBuffers objectForKey:key] mutableBytes], 256, NULL, NULL);
		[reportBuffers removeObjectForKey:key];
	}
	[devices removeObject:key];
	if (chosen == device)
		chosen = NULL;
	[self choose];
	[self refresh:nil];
}

- (void)noteInput
{
	inputCount++;
	lastInput = CFAbsoluteTimeGetCurrent();
}

- (void)applyValue:(IOHIDValueRef)value counted:(BOOL)counted
{
	IOHIDElementRef e = IOHIDValueGetElement(value);
	if (reportSeen)
		return; /* (the reports are the state; the elements only trail them) */
	if (counted)
		[self noteInput];
	if (cs_hid_apply(&padView->pad, IOHIDElementGetUsagePage(e), IOHIDElementGetUsage(e),
			(int)IOHIDValueGetIntegerValue(value), (int)IOHIDElementGetLogicalMin(e),
			(int)IOHIDElementGetLogicalMax(e)))
		[padView setNeedsDisplay:YES];
}

- (void)handleValue:(IOHIDValueRef)value
{
	IOHIDElementRef e = IOHIDValueGetElement(value);
	IOHIDDeviceRef d = IOHIDElementGetDevice(e);
	if (!chosen || d != chosen)
		return;
	[self applyValue:value counted:YES];
}

- (void)handleReport:(const uint8_t *)report length:(CFIndex)length id:(uint32_t)reportID device:(IOHIDDeviceRef)device
{
	unsigned char data[260];
	struct mac_gamepad_frame frame;
	unsigned extra = 0;
	unsigned size;

	if (!chosen || device != chosen || length <= 0 || length > 256)
		return;
	asleep = NO;
	[self noteInput];
	if (reportKind == MAC_PAD_REPORT_NONE)
		return;
	/* the decoder wants the report id first: the system may or may not have left it in */
	if (reportID != 0 && report[0] != (uint8_t)reportID) {
		data[0] = (unsigned char)reportID;
		memcpy(data + 1, report, (size_t)length);
		size = (unsigned)length + 1;
	} else {
		memcpy(data, report, (size_t)length);
		size = (unsigned)length;
	}
	if (mac_gamepad_decode_report(reportKind, data, size, &frame, &extra)) {
		cs_pad_from_frame(&padView->pad, &frame, extra);
		reportSeen = YES;
		[padView setNeedsDisplay:YES];
	}
}

- (void)refresh:(id)sender
{
	NSString *text;
	CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();

	if (!chosen)
		text = [self adapterWaiting] ? kStatusAdapterIdle : kStatusNoPad;
	else if (mac_gamepad_stream_silent(reportKind, now - (inputCount > 0 ? lastInput : chosenAt))) {
		/* the adapter streams all the time: none for a second is a pad asleep or out of range, not a pad at rest */
		text = kStatusAsleep;
		if (!asleep) {
			asleep = YES;
			reportSeen = NO;
			cs_pad_clear(&padView->pad, [self layoutOf:chosen]);
			[padView setNeedsDisplay:YES];
		}
	}
	else if (inputCount > 0 && now - lastInput < SILENCE_SECONDS)
		text = kStatusReceiving;
	else if (inputCount > 0)
		text = kStatusQuiet;
	else if (now - chosenAt < SILENCE_SECONDS)
		text = kStatusWaiting;
	else
		text = kStatusSilent;
	if (![[status stringValue] isEqualToString:text])
		[status setStringValue:text];
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
			i++; /* (older launchers pass a driver package; there is none now) */
	}
	[NSApplication sharedApplication];
	controller = [[SetupController alloc] init];
	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "--dump-text") == 0) {
			/* the window is built but never shown, and no pad is opened */
			[controller buildUI];
			[controller dumpText];
			return 0;
		}
	[controller start];
	[NSApp activateIgnoringOtherApps:YES];
	[NSApp run];
	[pool release];
	return 0;
}
