/* See launcher_dialog.h. The window is made of Control Manager controls in code (no
   nib), so it runs on 10.4 to 10.6 on both architectures. Layout: the title, then
   Display and Graphics (and Sound) side by side, Options below, and the buttons. */
#include "launcher_dialog.h"
#include <Carbon/Carbon.h>
#include <string.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 510
#define MAX_MODES 64
#define MAX_CHOICES (MAX_MODES + 4)

enum
{
	CMD_PLAY = 'PLAY', CMD_QUIT = 'QUIT', CMD_FULL = 'FULL', CMD_WINDOW = 'WNDW', CMD_SOUND = 'SOND',
	CMD_DISC = 'DISC', CMD_PAD = 'CTRL'
};

struct choice
{
	const char *label, *value;
};

struct popup
{
	ControlRef control;
	MenuRef menu;
	const struct choice *choices;
	int count;
	char extra[LC_VALUE];	/* a value the file holds that the list does not */
};

static const struct choice detail_choices[] =
{
	{ "Auto", "auto" }, { "High", "high" }, { "Medium", "medium" }, { "Low", "low" }, { "Lowest", "lowest" }
};
static const struct choice texture_choices[] =
{
	{ "Auto", "-1" }, { "High", "0" }, { "Medium", "1" }, { "Low", "2" }, { "Lowest", "3" }
};
static const struct choice aa_choices[] = { { "Off", "off" }, { "FXAA", "fxaa" } };
static const struct choice shadow_choices[] =
{
	{ "128 (as the Xbox)", "128" }, { "256", "256" }, { "512", "512" }, { "1024", "1024" }
};
static const struct choice fps_choices[] =
{
	{ "Auto", "0" }, { "30", "30" }, { "60", "60" }, { "120", "120" }, { "144", "144" }, { "165", "165" },
	{ "240", "240" }, { "None", "-1" }
};
static const struct choice source_choices[] =
{
	{ "Automatic", "auto" }, { "Xbox disc data", "xbox" }, { "MacSoft's Halo", "retail" }
};

static struct lc_model *model;
static const struct ld_env *environment;
static WindowRef window;
static int result;
static int fullscreen_selected;
static char resolution_value[2][LC_VALUE];	/* [0] fullscreen's display.resolution, [1] the window's size */
static struct choice resolution_choices[MAX_CHOICES];
static char resolution_labels[MAX_CHOICES][32], resolution_values[MAX_CHOICES][16];
static struct popup p_resolution, p_fps, p_detail, p_texture, p_aa, p_shadow, p_source;
static ControlRef c_full, c_window, c_wide, c_vsync, c_smooth, c_gamma, c_gamma_text, c_hud, c_text, c_ppl;
static ControlRef c_sound, c_reverb, c_volume, c_volume_text, c_movies, c_achievements, c_status, c_hide;

/* ---------- the display's modes */

int ld_modes(int modes[][2], int max)
{
	CFArrayRef list = CGDisplayAvailableModes(kCGDirectMainDisplay);
	int count = 0, i, j;

	if (!list)
		return 0;
	for (i = 0; i < CFArrayGetCount(list) && count < max; ++i)
	{
		CFDictionaryRef mode = (CFDictionaryRef)CFArrayGetValueAtIndex(list, i);
		CFNumberRef w = (CFNumberRef)CFDictionaryGetValue(mode, kCGDisplayWidth);
		CFNumberRef h = (CFNumberRef)CFDictionaryGetValue(mode, kCGDisplayHeight);
		int width = 0, height = 0, duplicate = 0;

		if (!w || !h)
			continue;
		CFNumberGetValue(w, kCFNumberIntType, &width);
		CFNumberGetValue(h, kCFNumberIntType, &height);
		if (width < 640 || height < 480)
			continue;
		for (j = 0; j < count; ++j)
			if (modes[j][0] == width && modes[j][1] == height)
				duplicate = 1;
		if (!duplicate)
		{
			modes[count][0] = width;
			modes[count][1] = height;
			++count;
		}
	}
	for (i = 1; i < count; ++i)
		for (j = i; j > 0 && (modes[j - 1][0] < modes[j][0] ||
			(modes[j - 1][0] == modes[j][0] && modes[j - 1][1] < modes[j][1])); --j)
		{
			int t0 = modes[j][0], t1 = modes[j][1];

			modes[j][0] = modes[j - 1][0];
			modes[j][1] = modes[j - 1][1];
			modes[j - 1][0] = t0;
			modes[j - 1][1] = t1;
		}
	return count;
}

/* ---------- controls */

static CFStringRef cs(const char *s)
{
	return CFStringCreateWithCString(NULL, s, kCFStringEncodingUTF8);
}

static void set_rect(Rect *r, int left, int top, int right, int bottom)
{
	r->left = (short)left;
	r->top = (short)top;
	r->right = (short)right;
	r->bottom = (short)bottom;
}

static ControlRef make_label(int left, int top, int right, int bottom, const char *text, int right_aligned, int small)
{
	Rect r;
	ControlRef c = NULL;
	ControlFontStyleRec style;

	memset(&style, 0, sizeof(style));
	style.flags = kControlUseThemeFontIDMask | kControlUseJustMask;
	style.font = small ? kThemeSmallSystemFont : kThemeSystemFont;
	style.just = right_aligned ? teFlushRight : teFlushLeft;
	set_rect(&r, left, top, right, bottom);
	CreateStaticTextControl(window, &r, cs(text), &style, &c);
	return c;
}

static void set_text(ControlRef c, const char *text)
{
	CFStringRef s = cs(text);

	SetControlData(c, kControlEntireControl, kControlStaticTextCFStringTag, sizeof(s), &s);
	CFRelease(s);
	HIViewSetNeedsDisplay((HIViewRef)c, true);
}

static ControlRef make_check(int left, int top, int right, const char *title, int on, OSType command)
{
	Rect r;
	ControlRef c = NULL;

	set_rect(&r, left, top, right, top + 18);
	CreateCheckBoxControl(window, &r, cs(title), on ? 1 : 0, true, &c);
	if (command)
		SetControlCommandID(c, command);
	return c;
}

static ControlRef make_radio(int left, int top, int right, const char *title, int on, OSType command)
{
	Rect r;
	ControlRef c = NULL;

	set_rect(&r, left, top, right, top + 18);
	CreateRadioButtonControl(window, &r, cs(title), on ? 1 : 0, false, &c);
	SetControlCommandID(c, command);
	return c;
}

static ControlRef make_button(int left, int top, int right, const char *title, OSType command)
{
	Rect r;
	ControlRef c = NULL;

	set_rect(&r, left, top, right, top + 20);
	CreatePushButtonControl(window, &r, cs(title), &c);
	SetControlCommandID(c, command);
	return c;
}

static void make_box(int left, int top, int right, int bottom, const char *title)
{
	Rect r;
	ControlRef c = NULL;

	set_rect(&r, left, top, right, bottom);
	CreateGroupBoxControl(window, &r, cs(title), true, &c);
}

static int value_is(const struct choice *choices, int count, const char *value)
{
	int i;

	for (i = 0; i < count; ++i)
		if (strcmp(choices[i].value, value) == 0)
			return i;
	return -1;
}

/* fills the popup's menu with choices and selects value (an item of its own is added for a value the list lacks) */
static void popup_fill(struct popup *p, const struct choice *choices, int count, const char *value)
{
	int i, at = value_is(choices, count, value);
	MenuItemIndex n = CountMenuItems(p->menu);

	if (n > 0)
		DeleteMenuItems(p->menu, 1, n);
	for (i = 0; i < count; ++i)
		AppendMenuItemTextWithCFString(p->menu, cs(choices[i].label), 0, 0, NULL);
	p->choices = choices;
	p->count = count;
	p->extra[0] = 0;
	if (at < 0)
	{
		snprintf(p->extra, sizeof(p->extra), "%s", value);
		AppendMenuItemTextWithCFString(p->menu, cs(value[0] ? value : "(none)"), 0, 0, NULL);
		at = count;
	}
	SetControl32BitMinimum(p->control, 1);
	SetControl32BitMaximum(p->control, count + (p->extra[0] || at == count ? 1 : 0));
	SetControl32BitValue(p->control, at + 1);
	HIViewSetNeedsDisplay((HIViewRef)p->control, true);
}

static const char *popup_value(const struct popup *p)
{
	int v = (int)GetControl32BitValue(p->control) - 1;

	if (v >= 0 && v < p->count)
		return p->choices[v].value;
	return p->extra;
}

static void make_popup(struct popup *p, int left, int top, int right, const struct choice *choices, int count,
	const char *value)
{
	static SInt16 menu_id = 300;
	Rect r;

	memset(p, 0, sizeof(*p));
	set_rect(&r, left, top, right, top + 20);
	CreatePopupButtonControl(window, &r, NULL, -12345, false, 0, 0, 0, &p->control);
	CreateNewMenu(menu_id++, 0, &p->menu);
	SetControlData(p->control, kControlEntireControl, kControlPopupButtonMenuRefTag, sizeof(MenuRef), &p->menu);
	popup_fill(p, choices, count, value);
}

/* the resolution popup: the display's modes for fullscreen, or window sizes */
static void fill_resolution(int fullscreen)
{
	int modes[MAX_MODES][2], n = ld_modes(modes, MAX_MODES), i, count = 0;

	resolution_choices[count].label = fullscreen ? "Native (the desktop's)" : "Default (640 x 480)";
	resolution_choices[count].value = fullscreen ? "native" : "";
	++count;
	for (i = 0; i < n && count < MAX_CHOICES; ++i)
	{
		snprintf(resolution_labels[count], sizeof(resolution_labels[0]), "%d x %d", modes[i][0], modes[i][1]);
		snprintf(resolution_values[count], sizeof(resolution_values[0]), "%dx%d", modes[i][0], modes[i][1]);
		resolution_choices[count].label = resolution_labels[count];
		resolution_choices[count].value = resolution_values[count];
		++count;
	}
	popup_fill(&p_resolution, resolution_choices, count, resolution_value[fullscreen ? 0 : 1]);
}

static void select_mode(int fullscreen)
{
	snprintf(resolution_value[fullscreen_selected ? 0 : 1], LC_VALUE, "%s", popup_value(&p_resolution));
	fullscreen_selected = fullscreen;
	SetControl32BitValue(c_full, fullscreen ? 1 : 0);
	SetControl32BitValue(c_window, fullscreen ? 0 : 1);
	fill_resolution(fullscreen);
}

static void update_labels(void)
{
	char s[16];
	int g = (int)GetControl32BitValue(c_gamma), v = (int)GetControl32BitValue(c_volume);

	snprintf(s, sizeof(s), "%d.%d", g / 10, g % 10);
	set_text(c_gamma_text, s);
	snprintf(s, sizeof(s), "%d%%", v * 10);
	set_text(c_volume_text, s);
	if (GetControl32BitValue(c_sound))
	{
		EnableControl(c_volume);
		EnableControl(c_reverb);
	}
	else
	{
		DisableControl(c_volume);
		DisableControl(c_reverb);
	}
}

static void slider_action(ControlRef c, ControlPartCode part)
{
	(void)c;
	(void)part;
	update_labels();
}

static void refresh_status(void)
{
	char s[256];

	s[0] = 0;
	if (environment->data_status)
		environment->data_status(s, sizeof(s));
	set_text(c_status, s);
}

/* what the controls hold, into the model */
static void gather(void)
{
	struct lc_model *m = model;
	int fs = fullscreen_selected;

	snprintf(resolution_value[fs ? 0 : 1], LC_VALUE, "%s", popup_value(&p_resolution));
	/* (a borderless mode stays borderless while Full Screen is chosen; the file's own words stay) */
	if (fs && strcmp(m->value[LC_MODE], "borderless") != 0)
		snprintf(m->value[LC_MODE], LC_VALUE, "fullscreen");
	else if (!fs)
		snprintf(m->value[LC_MODE], LC_VALUE, "windowed");
	snprintf(m->value[LC_RESOLUTION], LC_VALUE, "%s", resolution_value[0]);
	snprintf(m->value[LC_WINDOW_SIZE], LC_VALUE, "%s", resolution_value[1]);
	snprintf(m->value[LC_SCALING], LC_VALUE, "%s", GetControl32BitValue(c_wide) ? "native" : "original");
	lc_set_bool(m, LC_VSYNC, GetControl32BitValue(c_vsync));
	lc_set_bool(m, LC_INTERPOLATION, GetControl32BitValue(c_smooth));
	snprintf(m->value[LC_MAX_FPS], LC_VALUE, "%s", popup_value(&p_fps));
	lc_set_tenths(m, LC_GAMMA, (int)GetControl32BitValue(c_gamma));
	snprintf(m->value[LC_DETAIL], LC_VALUE, "%s", popup_value(&p_detail));
	snprintf(m->value[LC_TEXTURE_SKIP], LC_VALUE, "%s", popup_value(&p_texture));
	snprintf(m->value[LC_ANTI_ALIASING], LC_VALUE, "%s", popup_value(&p_aa));
	snprintf(m->value[LC_SHADOW_RESOLUTION], LC_VALUE, "%s", popup_value(&p_shadow));
	lc_set_bool(m, LC_HIRES_HUD, GetControl32BitValue(c_hud));
	lc_set_bool(m, LC_HIRES_TEXT, GetControl32BitValue(c_text));
	lc_set_bool(m, LC_PER_PIXEL_LIGHTING, GetControl32BitValue(c_ppl));
	lc_set_bool(m, LC_AUDIO, GetControl32BitValue(c_sound));
	lc_set_bool(m, LC_REVERB, GetControl32BitValue(c_reverb));
	lc_set_tenths(m, LC_VOLUME, (int)GetControl32BitValue(c_volume));
	/* (a volume of 1.0 is written 1.0; 10 steps of a tenth) */
	lc_set_bool(m, LC_MOVIES, GetControl32BitValue(c_movies));
	lc_set_bool(m, LC_INTRO_MOVIES, GetControl32BitValue(c_movies));
	lc_set_bool(m, LC_ACHIEVEMENTS, GetControl32BitValue(c_achievements));
	snprintf(m->data_source, sizeof(m->data_source), "%s", popup_value(&p_source));
	m->hide = (int)GetControl32BitValue(c_hide);
}

static void finish(int play)
{
	if (play)
		gather();
	result = play;
	QuitAppModalLoopForWindow(window);
}

static void run_helper(void (*helper)(void))
{
	if (!helper)
		return;
	HideWindow(window);
	helper();
	ShowWindow(window);
	SelectWindow(window);
	refresh_status();
}

static OSStatus window_handler(EventHandlerCallRef call, EventRef event, void *data)
{
	UInt32 class = GetEventClass(event), kind = GetEventKind(event);

	(void)call;
	(void)data;
	if (class == kEventClassCommand && kind == kEventCommandProcess)
	{
		HICommand command;

		GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(command), NULL, &command);
		switch (command.commandID)
		{
		case CMD_PLAY:
			finish(1);
			return noErr;
		case CMD_QUIT:
		case kHICommandQuit:
			finish(0);
			return noErr;
		case CMD_FULL:
			if (!fullscreen_selected)
				select_mode(1);
			return noErr;
		case CMD_WINDOW:
			if (fullscreen_selected)
				select_mode(0);
			return noErr;
		case CMD_SOUND:
			update_labels();
			return noErr;
		case CMD_DISC:
			run_helper(environment->run_disc_setup);
			return noErr;
		case CMD_PAD:
			run_helper(environment->run_controller_setup);
			return noErr;
		}
	}
	else if (class == kEventClassWindow && kind == kEventWindowClose)
	{
		finish(0);
		return noErr;
	}
	else if (class == kEventClassKeyboard && kind == kEventRawKeyDown)
	{
		UInt32 code = 0, modifiers = 0;

		GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL, sizeof(code), NULL, &code);
		GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
		if ((modifiers & cmdKey) && code == 12)		/* Command-Q */
		{
			finish(0);
			return noErr;
		}
		if (!(modifiers & cmdKey) && (code == 36 || code == 76))	/* Return, Enter */
		{
			finish(1);
			return noErr;
		}
		if (code == 53)		/* Escape */
		{
			finish(0);
			return noErr;
		}
	}
	return eventNotHandledErr;
}

static void make_icon(void)
{
	FSRef ref;
	IconRef icon = NULL;
	SInt16 label;
	ControlButtonContentInfo content;
	Rect r;
	ControlRef c = NULL;

	if (!environment->app_path || FSPathMakeRef((const UInt8 *)environment->app_path, &ref, NULL) != noErr)
		return;
	if (GetIconRefFromFileInfo(&ref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, &icon, &label) != noErr)
		return;
	memset(&content, 0, sizeof(content));
	content.contentType = kControlContentIconRef;
	content.u.iconRef = icon;
	set_rect(&r, 18, 12, 66, 60);
	CreateIconControl(window, &r, &content, true, &c);
}

int ld_run(struct lc_model *m, const struct ld_env *env)
{
	static const EventTypeSpec specs[] =
	{
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassWindow, kEventWindowClose },
		{ kEventClassKeyboard, kEventRawKeyDown }
	};
	Rect bounds;
	ControlRef play, quit;
	ProcessSerialNumber psn;
	ControlActionUPP action = NewControlActionUPP(slider_action);

	model = m;
	environment = env;
	result = -1;
	snprintf(resolution_value[0], LC_VALUE, "%s", m->value[LC_RESOLUTION]);
	snprintf(resolution_value[1], LC_VALUE, "%s", m->value[LC_WINDOW_SIZE]);
	fullscreen_selected = strcmp(m->value[LC_MODE], "windowed") != 0;

	set_rect(&bounds, 100, 100, 100 + WINDOW_WIDTH, 100 + WINDOW_HEIGHT);
	if (CreateNewWindow(kMovableModalWindowClass,
		kWindowCloseBoxAttribute | kWindowCompositingAttribute | kWindowStandardHandlerAttribute, &bounds,
		&window) != noErr)
		return -1;
	SetWindowTitleWithCFString(window, cs("PowerSpark"));

	make_icon();
	make_label(76, 14, 460, 38, "PowerSpark", 0, 0);
	make_label(76, 38, 620, 56, "Halo: Combat Evolved. Choose your settings, then click Play.", 0, 1);

	/* Display */
	make_box(16, 72, 312, 240, "Display");
	c_full = make_radio(32, 94, 160, "Full Screen", fullscreen_selected, CMD_FULL);
	c_window = make_radio(170, 94, 298, "Windowed", !fullscreen_selected, CMD_WINDOW);
	make_label(28, 124, 112, 142, "Resolution:", 1, 0);
	make_popup(&p_resolution, 118, 120, 298, resolution_choices, 0, "");
	fill_resolution(fullscreen_selected);
	c_wide = make_check(32, 152, 298, "Widescreen (a wider view on a wide display)",
		strcmp(m->value[LC_SCALING], "original") != 0, 0);
	c_vsync = make_check(32, 180, 160, "V-Sync", lc_bool(m->value[LC_VSYNC]), 0);
	c_smooth = make_check(170, 180, 298, "Smooth motion", lc_bool(m->value[LC_INTERPOLATION]), 0);
	make_label(28, 210, 128, 228, "Frame rate limit:", 1, 0);
	make_popup(&p_fps, 134, 206, 298, fps_choices, 8, m->value[LC_MAX_FPS]);

	/* Sound */
	make_box(16, 248, 312, 330, "Sound");
	c_sound = make_check(32, 270, 150, "Sound", lc_bool(m->value[LC_AUDIO]), CMD_SOUND);
	c_reverb = make_check(170, 270, 298, "Reverb", lc_bool(m->value[LC_REVERB]), 0);
	make_label(28, 300, 92, 318, "Volume:", 1, 0);
	set_rect(&bounds, 98, 300, 248, 318);
	c_volume = NULL;
	CreateSliderControl(window, &bounds, lc_tenths(m->value[LC_VOLUME], 0, 10), 0, 10, kControlSliderDoesNotPoint, 0,
		true, action, &c_volume);
	c_volume_text = make_label(254, 302, 306, 318, "", 0, 1);

	/* Graphics */
	make_box(328, 72, 624, 330, "Graphics");
	make_label(336, 98, 450, 116, "Detail:", 1, 0);
	make_popup(&p_detail, 456, 94, 616, detail_choices, 5, m->value[LC_DETAIL]);
	make_label(336, 126, 450, 144, "Texture quality:", 1, 0);
	make_popup(&p_texture, 456, 122, 616, texture_choices, 5, m->value[LC_TEXTURE_SKIP]);
	make_label(336, 154, 450, 172, "Anti-aliasing:", 1, 0);
	make_popup(&p_aa, 456, 150, 616, aa_choices, 2, m->value[LC_ANTI_ALIASING]);
	make_label(336, 182, 450, 200, "Shadow quality:", 1, 0);
	make_popup(&p_shadow, 456, 178, 616, shadow_choices, 4, m->value[LC_SHADOW_RESOLUTION]);
	make_label(336, 212, 450, 230, "Gamma:", 1, 0);
	set_rect(&bounds, 456, 212, 566, 230);
	c_gamma = NULL;
	CreateSliderControl(window, &bounds, lc_tenths(m->value[LC_GAMMA], 5, 20), 5, 20, kControlSliderDoesNotPoint, 0,
		true, action, &c_gamma);
	c_gamma_text = make_label(574, 214, 616, 230, "", 0, 1);
	c_hud = make_check(344, 242, 616, "High-resolution HUD", lc_bool(m->value[LC_HIRES_HUD]), 0);
	c_text = make_check(344, 266, 616, "High-resolution text", lc_bool(m->value[LC_HIRES_TEXT]), 0);
	c_ppl = make_check(344, 290, 616, "Per-pixel lighting", lc_bool(m->value[LC_PER_PIXEL_LIGHTING]), 0);

	/* Options */
	make_box(16, 338, 624, 452, "Options");
	c_movies = make_check(32, 360, 300, "Play intro movies", lc_bool(m->value[LC_MOVIES]), 0);
	c_achievements = make_check(320, 360, 612, "Achievements", lc_bool(m->value[LC_ACHIEVEMENTS]), 0);
	make_label(28, 390, 112, 408, "Game data:", 1, 0);
	make_popup(&p_source, 118, 386, 278, source_choices, 3, m->data_source);
	c_status = make_label(288, 390, 612, 406, "", 0, 1);
	make_button(32, 416, 232, "Set Up from Disc Image\xE2\x80\xA6", CMD_DISC);
	make_button(244, 416, 404, "Controller Setup\xE2\x80\xA6", CMD_PAD);

	/* the buttons */
	c_hide = make_check(20, 462, 330, "Don't show this at startup", m->hide, 0);
	make_label(38, 484, 440, 500, "Hold Option while opening PowerSpark to show this window again.", 0, 1);
	quit = make_button(456, 468, 536, "Quit", CMD_QUIT);
	play = make_button(544, 468, 624, "Play", CMD_PLAY);

	update_labels();
	refresh_status();
	SetWindowDefaultButton(window, play);
	SetWindowCancelButton(window, quit);
	InstallWindowEventHandler(window, NewEventHandlerUPP(window_handler), 3, specs, NULL, NULL);
	RepositionWindow(window, NULL, kWindowCenterOnMainScreen);
	if (GetCurrentProcess(&psn) == noErr)
		SetFrontProcess(&psn);
	ShowWindow(window);
	SelectWindow(window);
	RunAppModalLoopForWindow(window);
	HideWindow(window);
	DisposeWindow(window);
	DisposeControlActionUPP(action);
	return result;
}
