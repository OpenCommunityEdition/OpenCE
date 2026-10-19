/* The Bluetooth (experimental) window of Controller Setup: pairs and live-tests a
   PlayStation pad over PowerSpark's own Bluetooth stack (../bt/), through a
   TP-Link UB500 (Realtek RTL8761BU) dongle. It is a test window, not the game's
   feature. The stack runs on its own thread (../bt/bt_session.c); this file only
   asks it for things and draws its snapshot. */
#import <Cocoa/Cocoa.h>
#include <string.h>
#include <stdio.h>

#include "controller_setup_ui.h"
#include "controller_setup_config.h"
#include "../bt/bt_session.h"
#include "../bt/bt_usb.h"

#define ROWS 4

/* where the firmware project will be: a placeholder until the user names it */
#define FIRMWARE_PROJECT_LINK @"https://github.com/<your-account>/<firmware-project> (link to follow)"

static NSString *const kIntro =
	@"A test window: pair a PlayStation controller with PowerSpark's own Bluetooth, through a TP-Link UB500 dongle "
	@"(Realtek RTL8761BU), and check what arrives. It is not the game's controller support yet; the game does not use it.";
static NSString *const kDongleNone = @"Dongle: not found. Plug in the TP-Link UB500.";
static NSString *const kDongleFound = @"Dongle: found. Press Start to open it.";
static NSString *const kDongleApple = @"Dongle: in use by Apple's Bluetooth.";
static NSString *const kDongleBusy = @"Dongle: in use by another program.";
static NSString *const kDongleOpening = @"Dongle: opening it and loading the firmware...";
static NSString *const kDongleFailed = @"Dongle: could not be started.";
static NSString *const kNoticeFirmware =
	@"The dongle needs Realtek's firmware, which PowerSpark does not ship. Get rtl8761bu_fw.bin and rtl8761bu_config.bin from the "
	@"separate open-source project at " FIRMWARE_PROJECT_LINK @" and put them in ~/Library/Application Support/OpenCE/Bluetooth/ "
	@"(or /Library/Application Support/RealtekBTFirmware/, or the folder named by input.bluetooth_firmware_dir in config.toml).";
static NSString *const kNoticeApple =
	@"Apple's Bluetooth driver has taken the dongle. To keep it off the dongle, type in Terminal: "
	@"sudo nvram bluetoothHostControllerSwitchBehavior=never   then unplug the dongle and plug it in again. "
	@"This only stops Apple's driver switching to a second Bluetooth controller; the Mac's own Bluetooth keeps working. "
	@"To undo it: sudo nvram -d bluetoothHostControllerSwitchBehavior";
static NSString *const kNoticeNvramSet = @" (It is set on this Mac already: unplug the dongle and plug it in again.)";
static NSString *const kNoticeBusy = @"Another program has the dongle open. Quit it (another copy of this window, a Bluetooth tool) and press Start again.";
static NSString *const kMessageIdle = @"Press Start to open the dongle. Pads paired before reconnect when you press PS on them.";

static struct { const char *name; int r, g, b; } const colours[7] = {
	{ "Magenta", 255, 0, 160 }, { "Cyan", 0, 220, 255 }, { "Yellow", 255, 220, 0 }, { "Green", 0, 255, 60 },
	{ "Red", 255, 0, 0 }, { "Blue", 0, 40, 255 }, { "White", 255, 255, 255 }
};
static const unsigned char playerPatterns[6] = { 0x04, 0x0a, 0x15, 0x1b, 0x1f, 0x00 };

@interface BTPanel : NSObject {
	NSWindow *window;
	NSString *configPath;
	NSTextField *intro, *dongleLabel, *notice, *listHeading, *messageLabel, *rateLabel, *testLabel;
	NSButton *startButton, *searchButton, *disconnectButton, *lightButton, *playersButton, *rumbleButton, *closeButton;
	NSTextField *rowLabel[ROWS];
	NSButton *rowButton[ROWS];
	int rowKind[ROWS];              /* 0 none, 1 paired (Forget), 2 found (Pair) */
	unsigned char rowAddr[ROWS][6];
	PadView *padView;
	struct bt_session *session;
	NSTimer *timer;
	int colourIndex, playerIndex;
	struct bt_usb_probe probe;
	struct bt_firmware_info firmware;
	CFAbsoluteTime lastProbe;
	float noticeHeight;
}
- (void)buildUI;
- (void)show;
- (void)setConfigPath:(NSString *)path;
- (int)dumpText;
- (void)shutdown;
- (void)refresh:(id)sender;
@end

static BTPanel *gPanel = nil;

@implementation BTPanel

- (NSTextField *)labelAt:(NSRect)frame text:(NSString *)text bold:(BOOL)bold
{
	NSTextField *t = [[[NSTextField alloc] initWithFrame:frame] autorelease];
	[t setEditable:NO];
	[t setBordered:NO];
	[t setDrawsBackground:NO];
	[t setSelectable:NO];
	[t setStringValue:text];
	[t setFont:bold ? [NSFont boldSystemFontOfSize:13] : [NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
	[[t cell] setWraps:YES];
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

/* the window from the top down; y is measured down from the top, and every control is placed by its top */
- (void)buildUI
{
	const float margin = 10, width = 500;
	float introH = CSThemeSize(kIntro, kThemeSmallSystemFont, width - 8, YES).height + 6;
	float headH = 20, dongleH = 32, msgH = 32;
	float h, y;
	NSString *notices[4];
	int i;

	notices[0] = kNoticeFirmware; notices[1] = [kNoticeApple stringByAppendingString:kNoticeNvramSet];
	notices[2] = kNoticeBusy; notices[3] = kNoticeApple;
	noticeHeight = 0;
	for (i = 0; i < 4; i++) {
		float n = CSThemeSize(notices[i], kThemeSmallSystemFont, width - 8, YES).height + 6;
		if (n > noticeHeight)
			noticeHeight = n;
	}
	/* heading, intro, dongle, notice, buttons, list heading, rows, message, pad view, test row, rate, close */
	h = 8 + headH + introH + dongleH + noticeHeight + 34 + 18 + ROWS * 26 + msgH + 160 + 14 + 34 + 20 + 40;
	window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 520, h) styleMask:NSTitledWindowMask | NSClosableWindowMask
		backing:NSBackingStoreBuffered defer:NO];
	[window setTitle:@"PowerSpark Bluetooth (experimental)"];
	[window setReleasedWhenClosed:NO];
	[window setDelegate:(id)self];

	y = 8;
	[self labelAt:NSMakeRect(margin, h - y - headH, width, headH) text:@"Bluetooth (experimental)" bold:YES];
	y += headH;
	intro = [self labelAt:NSMakeRect(margin, h - y - introH, width, introH) text:kIntro bold:NO];
	y += introH;
	dongleLabel = [self labelAt:NSMakeRect(margin, h - y - dongleH, width, dongleH) text:kDongleNone bold:NO];
	y += dongleH;
	notice = [self labelAt:NSMakeRect(margin, h - y - noticeHeight, width, noticeHeight) text:@"" bold:NO];
	y += noticeHeight + 2;
	startButton = [self buttonAt:NSMakeRect(margin, h - y - 30, 100, 32) title:@"Start" action:@selector(startPressed:)];
	searchButton = [self buttonAt:NSMakeRect(margin + 110, h - y - 30, 180, 32) title:@"Search and pair" action:@selector(searchPressed:)];
	disconnectButton = [self buttonAt:NSMakeRect(margin + 300, h - y - 30, 110, 32) title:@"Disconnect" action:@selector(disconnectPressed:)];
	y += 34;
	listHeading = [self labelAt:NSMakeRect(margin, h - y - 16, width, 16) text:@"Controllers (press PS on a paired one to connect it):" bold:NO];
	y += 18;
	for (i = 0; i < ROWS; i++) {
		rowLabel[i] = [self labelAt:NSMakeRect(margin, h - y - 22, 400, 22) text:@"" bold:NO];
		rowButton[i] = [self buttonAt:NSMakeRect(margin + 410, h - y - 24, 90, 26) title:@"Pair" action:@selector(rowPressed:)];
		[rowButton[i] setTag:i];
		[rowButton[i] setHidden:YES];
		y += 26;
	}
	messageLabel = [self labelAt:NSMakeRect(margin, h - y - msgH, width, msgH) text:kMessageIdle bold:NO];
	y += msgH;
	padView = [[[PadView alloc] initWithFrame:NSMakeRect(margin, h - y - 160, width, 160)] autorelease];
	padView->leftDeadZone = padView->rightDeadZone = 9000;
	padView->showTouch = YES;
	[[window contentView] addSubview:padView];
	y += 160 + 14;
	lightButton = [self buttonAt:NSMakeRect(margin, h - y - 30, 140, 32) title:@"Light bar color" action:@selector(lightPressed:)];
	playersButton = [self buttonAt:NSMakeRect(margin + 150, h - y - 30, 120, 32) title:@"Player lights" action:@selector(playersPressed:)];
	rumbleButton = [self buttonAt:NSMakeRect(margin + 280, h - y - 30, 90, 32) title:@"Rumble" action:@selector(rumblePressed:)];
	y += 34;
	rateLabel = [self labelAt:NSMakeRect(margin, h - y - 18, width, 18) text:@"Reports: -" bold:NO];
	y += 20;
	closeButton = [self buttonAt:NSMakeRect(margin + 400, h - y - 36, 100, 32) title:@"Close" action:@selector(closePressed:)];
	(void)testLabel;
	[padView setNeedsDisplay:YES];
}

- (NSString *)firmwareDir
{
	char v[600];
	if (configPath && cs_config_get([configPath fileSystemRepresentation], "input.bluetooth_firmware_dir", v, sizeof v) == 1) {
		unsigned n = (unsigned)strlen(v);
		if (n >= 2 && v[0] == '"' && v[n - 1] == '"') { v[n - 1] = 0; return [NSString stringWithUTF8String:v + 1]; }
		return [NSString stringWithUTF8String:v];
	}
	return nil;
}

- (void)setConfigPath:(NSString *)path
{
	[path retain];
	[configPath release];
	configPath = path;
}

- (void)show
{
	if (!window)
		[self buildUI];
	[self refresh:nil];
	if (!timer)
		timer = [[NSTimer scheduledTimerWithTimeInterval:0.04 target:self selector:@selector(refresh:) userInfo:nil repeats:YES] retain];
	[window center];
	[window makeKeyAndOrderFront:nil];
}

- (void)startPressed:(id)sender
{
	BOOL wasFailed = NO;
	if (session) {
		struct bt_snapshot snap;
		bt_session_snapshot(session, &snap);
		wasFailed = snap.phase == BT_PHASE_FAILED;
		bt_session_stop(session);
		session = NULL;
		if (!wasFailed) { [self refresh:nil]; return; }
	}
	{
		NSString *dir = [self firmwareDir];
		session = bt_session_start(dir ? [dir fileSystemRepresentation] : NULL);
	}
	[self refresh:nil];
}

- (void)searchPressed:(id)sender
{
	struct bt_snapshot snap;
	if (!session)
		return;
	bt_session_snapshot(session, &snap);
	if (snap.searching)
		bt_session_search_stop(session);
	else
		bt_session_search(session);
}

- (void)disconnectPressed:(id)sender
{
	if (session)
		bt_session_disconnect(session);
}

- (void)rowPressed:(id)sender
{
	int i = (int)[sender tag];
	if (!session || i < 0 || i >= ROWS)
		return;
	if (rowKind[i] == 1)
		bt_session_forget(session, rowAddr[i]);
	else if (rowKind[i] == 2)
		bt_session_pair(session, rowAddr[i]);
}

- (void)lightPressed:(id)sender
{
	struct bt_pad_output o;
	if (!session)
		return;
	memset(&o, 0, sizeof o);
	o.red = (unsigned char)colours[colourIndex].r;
	o.green = (unsigned char)colours[colourIndex].g;
	o.blue = (unsigned char)colours[colourIndex].b;
	o.set_light = 1;
	bt_session_output(session, &o);
	colourIndex = (colourIndex + 1) % 7;
}

- (void)playersPressed:(id)sender
{
	struct bt_pad_output o;
	if (!session)
		return;
	memset(&o, 0, sizeof o);
	o.player_leds = playerPatterns[playerIndex];
	o.set_players = 1;
	bt_session_output(session, &o);
	playerIndex = (playerIndex + 1) % 6;
}

- (void)rumblePressed:(id)sender
{
	struct bt_pad_output o;
	if (!session)
		return;
	memset(&o, 0, sizeof o);
	o.strong = 200;
	o.weak = 120;
	o.set_rumble = 1;
	bt_session_output(session, &o);   /* (the stack switches it off again after a moment) */
}

- (void)closePressed:(id)sender
{
	[window close];
}

- (void)windowWillClose:(NSNotification *)n
{
	[self shutdown];
}

- (void)shutdown
{
	if (session) {
		bt_session_stop(session);
		session = NULL;
	}
	if (timer) {
		[timer invalidate];
		[timer release];
		timer = nil;
	}
}

static NSString *AddressText(const unsigned char *a)
{
	char t[20];
	bt_addr_text(a, t);
	return [NSString stringWithUTF8String:t];
}

- (void)refresh:(id)sender
{
	struct bt_snapshot snap;
	CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
	NSString *dongleText, *noticeText = @"", *message;
	int dongle, i, rows = 0, running = session != NULL;
	BOOL failed = NO, ready = NO, haveNvram;

	memset(&snap, 0, sizeof snap);
	if (session)
		bt_session_snapshot(session, &snap);
	if (!session && (now - lastProbe > 2.0 || lastProbe == 0)) {
		bt_usb_probe(&probe);
		bt_firmware_find([self firmwareDir] ? [[self firmwareDir] fileSystemRepresentation] : NULL, &firmware);
		lastProbe = now;
	}
	haveNvram = probe.nvram_never;
	dongle = session ? snap.dongle : probe.dongle;
	failed = session && snap.phase == BT_PHASE_FAILED;
	ready = session && snap.phase == BT_PHASE_READY;

	if (ready)
		dongleText = [NSString stringWithFormat:@"Dongle: ready, %s. Address %@.", snap.chip, AddressText(snap.local_addr)];
	else if (session && !failed)
		dongleText = kDongleOpening;
	else if (dongle == BT_DONGLE_NONE)
		dongleText = kDongleNone;
	else if (dongle == BT_DONGLE_APPLE)
		dongleText = kDongleApple;
	else if (dongle == BT_DONGLE_BUSY)
		dongleText = kDongleBusy;
	else if (failed)
		dongleText = kDongleFailed;
	else
		dongleText = kDongleFound;
	if (![[dongleLabel stringValue] isEqualToString:dongleText])
		[dongleLabel setStringValue:dongleText];

	if (!ready && (!session || failed)) {
		if (dongle == BT_DONGLE_APPLE)
			noticeText = haveNvram ? [kNoticeApple stringByAppendingString:kNoticeNvramSet] : kNoticeApple;
		else if (dongle == BT_DONGLE_BUSY)
			noticeText = kNoticeBusy;
		else if (dongle != BT_DONGLE_NONE && !firmware.found)
			noticeText = kNoticeFirmware;
	}
	if (![[notice stringValue] isEqualToString:noticeText])
		[notice setStringValue:noticeText];

	message = session ? [NSString stringWithUTF8String:snap.message] : kMessageIdle;
	if (![[messageLabel stringValue] isEqualToString:message])
		[messageLabel setStringValue:message];

	[startButton setTitle:failed ? @"Try again" : running ? @"Stop" : @"Start"];
	[startButton setEnabled:running || dongle != BT_DONGLE_NONE];
	[searchButton setTitle:ready && snap.searching ? @"Stop searching" : @"Search and pair"];
	[searchButton setEnabled:ready && !snap.link_up && !snap.pairing];
	[disconnectButton setEnabled:ready && snap.link_up];

	/* the list: paired pads first, then game pads found that are not paired */
	if (ready) {
		for (i = 0; i < snap.paired_count && rows < ROWS; i++) {
			BOOL on = snap.link_up && !memcmp(snap.pad.addr, snap.paired[i].addr, 6) && snap.pad.connected;
			const char *nm = snap.paired[i].name[0] ? snap.paired[i].name : "Controller";
			rowKind[rows] = 1;
			memcpy(rowAddr[rows], snap.paired[i].addr, 6);
			[rowLabel[rows] setStringValue:[NSString stringWithFormat:@"%s  %@  %@", nm, AddressText(snap.paired[i].addr),
				on ? @"connected" : @"paired: press PS"]];
			[rowButton[rows] setTitle:@"Forget"];
			[rowButton[rows] setEnabled:YES];
			rows++;
		}
		for (i = 0; i < snap.found_count && rows < ROWS; i++) {
			int k, paired = 0;
			for (k = 0; k < snap.paired_count; k++)
				if (!memcmp(snap.paired[k].addr, snap.found[i].addr, 6))
					paired = 1;
			if (paired || !snap.found[i].gamepad)
				continue;
			rowKind[rows] = 2;
			memcpy(rowAddr[rows], snap.found[i].addr, 6);
			[rowLabel[rows] setStringValue:[NSString stringWithFormat:@"%s  %@  %s",
				snap.found[i].name[0] ? snap.found[i].name : "Game controller", AddressText(snap.found[i].addr),
				snap.found[i].named && !snap.found[i].supported ? "not supported yet" : "found"]];
			[rowButton[rows] setTitle:@"Pair"];
			[rowButton[rows] setEnabled:snap.found[i].named && snap.found[i].supported && !snap.pairing && !snap.link_up];
			rows++;
		}
	}
	for (i = 0; i < ROWS; i++) {
		if (i >= rows) {
			rowKind[i] = 0;
			[rowLabel[i] setStringValue:@""];
		}
		[rowButton[i] setHidden:i >= rows];
	}

	/* the live pad */
	{
		struct bt_pad_status *p = &snap.pad;
		BOOL on = ready && p->connected;
		padView->hasPad = on;
		if (on) {
			NSString *name = [NSString stringWithUTF8String:p->name[0] ? p->name : bt_pad_kind_name(p->kind)];
			cs_pad_from_frame(&padView->pad, &p->frame, p->extra);
			padView->isXbox = CS_LAYOUT_PLAYSTATION;
			if (!padView->padName || ![padView->padName isEqualToString:name]) {
				[padView->padName release];
				padView->padName = [name retain];
				[padView->sonyModel release];
				padView->sonyModel = [[NSString stringWithUTF8String:bt_pad_kind_name(p->kind)] retain];
			}
			for (i = 0; i < 2; i++) {
				padView->touchActive[i] = p->touch[i].active;
				padView->touchX[i] = p->touch[i].x;
				padView->touchY[i] = p->touch[i].y;
			}
			padView->touchWidth = p->touch_width;
			padView->touchHeight = p->touch_height;
			if (p->streaming) {
				NSString *r = [NSString stringWithFormat:@"Reports: %.0f/s (%u in all, CRC errors %u)%@", p->rate, p->reports, p->crc_errors,
					p->battery_percent >= 0 ? [NSString stringWithFormat:@"   Battery %d%%", p->battery_percent] : @""];
				if (![[rateLabel stringValue] isEqualToString:r])
					[rateLabel setStringValue:r];
			} else
				[rateLabel setStringValue:@"Connected, waiting for the full reports..."];
		} else {
			cs_pad_clear(&padView->pad, CS_LAYOUT_PLAYSTATION);
			padView->touchActive[0] = padView->touchActive[1] = 0;
			if (![[rateLabel stringValue] isEqualToString:@"Reports: -"])
				[rateLabel setStringValue:@"Reports: -"];
		}
		[padView setNeedsDisplay:YES];
		[lightButton setEnabled:on && p->streaming];
		[playersButton setEnabled:on && p->streaming];
		[rumbleButton setEnabled:on && p->streaming];
	}
}

/* --dump-text: the texts of every state, measured as the launcher measures */
- (int)dumpText
{
	NSString *all[10];
	int i, bad;
	NSRect f;
	if (!window)
		[self buildUI];
	f = [[window contentView] frame];
	printf("window %dx%d\n", (int)f.size.width, (int)f.size.height);
	bad = CSDumpView([window contentView], 0, @"  ");
	all[0] = kDongleNone; all[1] = kDongleFound; all[2] = kDongleApple; all[3] = kDongleBusy; all[4] = kDongleOpening;
	all[5] = kDongleFailed;
	all[6] = @"Dongle: ready, Realtek RTL8761B, firmware dfc6d922. Address 20:e1:5d:8d:15:d8.";
	all[7] = @"Search finished: nothing found. Is the controller in pairing mode?";
	all[8] = @"Searching... put the controller in pairing mode (DualSense: hold Create and PS until the light flashes fast).";
	all[9] = @"The controller does not accept the stored key (status 0x06): forget it, then pair again.";
	for (i = 0; i < 10; i++) {
		NSTextField *label = i < 7 ? dongleLabel : messageLabel;
		NSSize n = CSThemeSize(all[i], kThemeSmallSystemFont, 496, YES);
		NSRect lf = [label frame];
		printf("text %d needs %dx%d in %dx%d%s: %s\n", i, (int)n.width, (int)n.height, (int)lf.size.width, (int)lf.size.height,
			n.height > lf.size.height ? "  <-- CUT OFF" : "", [all[i] UTF8String]);
		if (n.height > lf.size.height)
			bad++;
	}
	{
		NSString *rows[2];
		rows[0] = @"DualSense Wireless Controller  4c:b9:9b:fd:1d:81  paired: press PS";
		rows[1] = @"DualSense Wireless Controller  4c:b9:9b:fd:1d:81  not supported yet";
		for (i = 0; i < 2; i++) {
			NSSize n = CSThemeSize(rows[i], kThemeSmallSystemFont, 396, YES);
			printf("row %d needs %dx%d in 400x22%s: %s\n", i, (int)n.width, (int)n.height, n.height > 22 ? "  <-- CUT OFF" : "", [rows[i] UTF8String]);
			if (n.height > 22)
				bad++;
		}
	}
	return bad;
}

@end

void CSBluetoothShow(NSString *configPath)
{
	if (!gPanel)
		gPanel = [[BTPanel alloc] init];
	[gPanel setConfigPath:configPath];
	[gPanel show];
}

int CSBluetoothDumpText(void)
{
	if (!gPanel)
		gPanel = [[BTPanel alloc] init];
	return [gPanel dumpText];
}

void CSBluetoothShutdown(void)
{
	[gPanel shutdown];
}
