/* What main.m shares with the Bluetooth panel (bt_panel.m). */
#ifndef CONTROLLER_SETUP_UI_H
#define CONTROLLER_SETUP_UI_H

#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "controller_setup_hid.h"

/* the live pad drawing: sticks with their dead zones, the d-pad, triggers, the 12 button lights, and, when
   showTouch is set, the touchpad with the fingers on it */
@interface PadView : NSView {
@public
	cs_pad_state pad;
	BOOL hasPad;
	int isXbox; /* the layout: CS_LAYOUT_* */
	int leftDeadZone;
	int rightDeadZone;
	NSString *padName;
	NSString *sonyModel; /* "DualShock 4", "DualSense" or nil */
	BOOL showTouch;
	int touchActive[2];
	int touchX[2], touchY[2];
	int touchWidth, touchHeight;
}
@end

/* the size theme text takes, as the launcher measures it, and --dump-text's walk of a view (returns the
   number of controls cut off) */
NSSize CSThemeSize(NSString *text, ThemeFontID font, float width, BOOL wrap);
int CSDumpView(NSView *view, int depth, NSString *indent);

/* the Bluetooth (experimental) window */
void CSBluetoothShow(NSString *configPath);
/* builds the window, prints its texts and sizes, returns the number cut off; shows nothing */
int CSBluetoothDumpText(void);
/* stops the Bluetooth session (if one runs) so the dongle is left tidy; call before exiting */
void CSBluetoothShutdown(void);

#endif
