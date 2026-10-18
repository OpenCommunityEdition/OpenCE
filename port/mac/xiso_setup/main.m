/* OpenCE Setup: asks for the player's own Halo: Combat Evolved Xbox disc
   image and copies its maps folder into the game's data folder.
   Cocoa for Mac OS X 10.4 and later: no blocks, no GCD, no nibs, no
   properties, no fast enumeration; the copy runs on an NSThread and the
   window reads its progress from a timer.

   Usage: "OpenCE Setup" --data DATA_ROOT [--image IMAGE]
   Installs into DATA_ROOT/maps. Exit status 0: installed; 1: quit without
   installing. --image skips the file chooser (and --test runs the copy with
   no window at all, for tests). */
#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "xiso_install.h"

static NSString *gDataRoot = nil;
static NSString *gImagePath = nil;

typedef struct
{
	volatile int cancel;
	volatile int running;
	volatile int finished;
	volatile int succeeded;
	volatile int cancelled;
	volatile unsigned long long done;
	volatile unsigned long long total;
	char error[512];
} InstallState;

static InstallState gState;

static void ProgressCallback(void *context, const char *file, unsigned long long done, unsigned long long total)
{
	(void)context;
	(void)file;
	gState.total = total;
	gState.done = done;
}

static void RunInstall(const char *image, const char *data)
{
	int cancelled = 0;
	int ok;

	gState.error[0] = 0;
	ok = xiso_install(image, data, ProgressCallback, NULL, &gState.cancel, &cancelled, gState.error,
		(int)sizeof(gState.error));
	gState.succeeded = ok;
	gState.cancelled = cancelled;
	gState.finished = 1;
}

@interface SetupController : NSObject {
	NSWindow *window;
	NSTextField *heading;
	NSTextField *body;
	NSTextField *status;
	NSButton *chooseButton;
	NSButton *quitButton;
	NSProgressIndicator *bar;
	NSTimer *timer;
	NSString *image;
}
- (void)start;
- (void)beginInstall:(NSString *)path;
@end

@implementation SetupController

- (NSTextField *)labelWithFrame:(NSRect)frame font:(NSFont *)font
{
	NSTextField *field = [[[NSTextField alloc] initWithFrame:frame] autorelease];
	[field setEditable:NO];
	[field setSelectable:NO];
	[field setBezeled:NO];
	[field setDrawsBackground:NO];
	[field setFont:font];
	return field;
}

- (void)start
{
	NSView *content;

	window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 520, 330)
		styleMask:NSTitledWindowMask | NSClosableWindowMask
		backing:NSBackingStoreBuffered defer:NO];
	[window setTitle:@"PowerSpark Setup"];
	[window setDelegate:self];
	[window setReleasedWhenClosed:NO];
	content = [window contentView];

	heading = [self labelWithFrame:NSMakeRect(20, 285, 480, 24) font:[NSFont boldSystemFontOfSize:16]];
	[heading setStringValue:@"PowerSpark needs your copy of Halo: Combat Evolved"];
	[content addSubview:heading];

	body = [self labelWithFrame:NSMakeRect(20, 130, 480, 145) font:[NSFont systemFontOfSize:13]];
	[body setStringValue:@"PowerSpark does not include the game, because it belongs to Microsoft. "
		@"To play, you need a disc image of Halo: Combat Evolved for the original Xbox, "
		@"made from a disc that you own. It is a file that ends in .iso or .xiso, "
		@"usually 1 to 8 GB.\n\n"
		@"Choose that file below. PowerSpark copies the game's maps (about 2 GB) into its own "
		@"folder, which takes a few minutes. You can delete the disc image afterwards."];
	[content addSubview:body];

	status = [self labelWithFrame:NSMakeRect(20, 95, 480, 32) font:[NSFont systemFontOfSize:12]];
	[status setStringValue:@""];
	[content addSubview:status];

	bar = [[[NSProgressIndicator alloc] initWithFrame:NSMakeRect(20, 65, 480, 20)] autorelease];
	[bar setStyle:NSProgressIndicatorBarStyle];
	[bar setIndeterminate:NO];
	[bar setMinValue:0];
	[bar setMaxValue:1000];
	[bar setDoubleValue:0];
	[bar setHidden:YES];
	[content addSubview:bar];

	chooseButton = [[[NSButton alloc] initWithFrame:NSMakeRect(300, 15, 200, 32)] autorelease];
	[chooseButton setBezelStyle:NSRoundedBezelStyle];
	[chooseButton setTitle:@"Choose Disc Image..."];
	[chooseButton setTarget:self];
	[chooseButton setAction:@selector(choose:)];
	[chooseButton setKeyEquivalent:@"\r"];
	[content addSubview:chooseButton];

	quitButton = [[[NSButton alloc] initWithFrame:NSMakeRect(20, 15, 120, 32)] autorelease];
	[quitButton setBezelStyle:NSRoundedBezelStyle];
	[quitButton setTitle:@"Quit"];
	[quitButton setTarget:self];
	[quitButton setAction:@selector(quit:)];
	[content addSubview:quitButton];

	[window center];
	[window makeKeyAndOrderFront:nil];
	if (gImagePath)
		[self beginInstall:gImagePath];
}

- (void)choose:(id)sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	NSArray *types = [NSArray arrayWithObjects:@"iso", @"xiso", @"ISO", @"XISO", nil];

	[panel setTitle:@"Choose your Halo: Combat Evolved Xbox disc image"];
	[panel setCanChooseDirectories:NO];
	[panel setAllowsMultipleSelection:NO];
	/* (10.4's call; 10.6 still takes it) */
	if ([panel runModalForDirectory:nil file:nil types:types] == NSOKButton && [[panel filenames] count] > 0)
		[self beginInstall:[[panel filenames] objectAtIndex:0]];
}

- (void)quit:(id)sender
{
	if (gState.running)
	{
		/* this is "Cancel" while copying */
		gState.cancel = 1;
		[status setStringValue:@"Cancelling..."];
		[quitButton setEnabled:NO];
		return;
	}
	exit(1);
}

- (void)beginInstall:(NSString *)path
{
	image = [path retain];
	memset((void *)&gState, 0, sizeof(gState));
	gState.running = 1;
	[chooseButton setEnabled:NO];
	[quitButton setTitle:@"Cancel"];
	[heading setStringValue:@"Copying the game's maps..."];
	[body setStringValue:@"Please keep this window open. This takes a few minutes, and "
		@"needs about 2 GB of free disk space."];
	[bar setHidden:NO];
	[bar setDoubleValue:0];
	[status setStringValue:@"Reading the disc image..."];
	timer = [[NSTimer scheduledTimerWithTimeInterval:0.2 target:self selector:@selector(tick:)
		userInfo:nil repeats:YES] retain];
	[NSThread detachNewThreadSelector:@selector(work:) toTarget:self withObject:path];
}

- (void)work:(NSString *)path
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	RunInstall([path fileSystemRepresentation], [gDataRoot fileSystemRepresentation]);
	gState.running = 0;
	[pool release];
}

- (void)tick:(NSTimer *)t
{
	if (gState.total > 0 && !gState.finished)
	{
		double fraction = (double)gState.done / (double)gState.total;
		[bar setDoubleValue:fraction * 1000.0];
		[status setStringValue:[NSString stringWithFormat:@"%d%% copied (%llu of %llu MB)", (int)(fraction * 100),
			gState.done >> 20, gState.total >> 20]];
	}
	if (!gState.finished)
		return;
	[timer invalidate];
	[timer release];
	timer = nil;
	[bar setHidden:YES];
	if (gState.succeeded)
	{
		[heading setStringValue:@"Ready"];
		[body setStringValue:@"The game's maps are installed. You can delete the disc image."];
		[status setStringValue:@""];
		NSRunInformationalAlertPanel(@"PowerSpark is ready to play", @"The game's maps were copied.", @"OK", nil, nil);
		exit(0);
	}
	if (gState.cancelled)
	{
		exit(1);
	}
	NSRunAlertPanel(@"The disc image could not be used", @"%s", @"OK", nil, nil, gState.error);
	/* try again with another file */
	[heading setStringValue:@"PowerSpark needs your copy of Halo: Combat Evolved"];
	[body setStringValue:@"Choose a disc image of Halo: Combat Evolved for the original Xbox (a .iso or .xiso file)."];
	[status setStringValue:[NSString stringWithUTF8String:gState.error]];
	[quitButton setTitle:@"Quit"];
	[quitButton setEnabled:YES];
	[chooseButton setEnabled:YES];
}

- (BOOL)windowShouldClose:(id)sender
{
	if (gState.running)
	{
		gState.cancel = 1;
		/* the copy stops at its next read; wait for its cleanup */
		while (!gState.finished)
			usleep(20000);
	}
	exit(1);
	return YES;
}

@end

int main(int argc, char **argv)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SetupController *controller;
	int i, test = 0;

	for (i = 1; i < argc; i++)
	{
		if (i + 1 < argc && strcmp(argv[i], "--data") == 0)
			gDataRoot = [[NSString stringWithUTF8String:argv[++i]] retain];
		else if (i + 1 < argc && strcmp(argv[i], "--image") == 0)
			gImagePath = [[NSString stringWithUTF8String:argv[++i]] retain];
		else if (strcmp(argv[i], "--test") == 0)
			test = 1;
	}
	if (!gDataRoot)
	{
		fprintf(stderr, "usage: OpenCE Setup --data DATA_ROOT [--image IMAGE] [--test]\n");
		return 64;
	}
	if (test)
	{
		/* no window: the same copy, for tests */
		if (!gImagePath)
			return 64;
		RunInstall([gImagePath fileSystemRepresentation], [gDataRoot fileSystemRepresentation]);
		if (!gState.succeeded)
			fprintf(stderr, "refused: %s\n", gState.error);
		return gState.succeeded ? 0 : 1;
	}
	[NSApplication sharedApplication];
	controller = [[SetupController alloc] init];
	[controller start];
	[NSApp activateIgnoringOtherApps:YES];
	[NSApp run];
	[pool release];
	return 0;
}
