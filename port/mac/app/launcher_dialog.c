/* See launcher_dialog.h. The window is made of Control Manager controls in code (no
   nib), so it runs on 10.4 to 10.6 on both architectures. Layout: the title, then
   Display and Graphics (and Sound) side by side, Options below, and the buttons. */
#include "launcher_dialog.h"
#include "launcher_update.h"
#include <Carbon/Carbon.h>
#include <string.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 582
#define MAX_ITEMS 160
#define MAX_MODES 64
#define MAX_CHOICES (MAX_MODES + 4)

enum
{
	CMD_PLAY = 'PLAY', CMD_QUIT = 'QUIT', CMD_FULL = 'FULL', CMD_WINDOW = 'WNDW', CMD_SOUND = 'SOND',
	CMD_DISC = 'DISC', CMD_PAD = 'CTRL', CMD_ABOUT = 'ABUT'
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
	{ "128 (Xbox)", "128" }, { "256", "256" }, { "512", "512" }, { "1024", "1024" }
};
static const struct choice fps_choices[] =
{
	{ "Auto", "0" }, { "30", "30" }, { "60", "60" }, { "120", "120" }, { "144", "144" }, { "165", "165" },
	{ "240", "240" }, { "None", "-1" }
};
static const struct choice render_choices[] =
{
	{ "Auto", "auto" }, { "Native", "native" }, { "480 lines", "original" }, { "600 lines", "600" },
	{ "720 lines", "720" }, { "768 lines", "768" }, { "900 lines", "900" }, { "1080 lines", "1080" },
	{ "1440 lines", "1440" }
};
static const struct choice adaptive_choices[] =
{
	{ "Auto", "auto" }, { "Off", "off" }, { "On, 30 fps", "30" }, { "On, 24 fps", "24" }, { "On, 45 fps", "45" },
	{ "On, 60 fps", "60" }
};
static const struct choice glyph_choices[] =
{
	{ "Auto", "auto" }, { "Xbox", "xbox" }, { "PlayStation", "playstation" }
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
static struct popup p_resolution, p_render, p_adaptive, p_fps, p_detail, p_texture, p_aa, p_shadow, p_source, p_glyphs;
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

/* what the layout holds, so that the text's width can be checked against the control's (ld_layout_report):
   the controls are made with window NULL (dry) to fill it without a window */
struct item
{
	char kind;		/* l label, c check, r radio, b button, p popup's menu item, t title */
	char text[96];
	short left, top, right, bottom;
	int font, reserve;	/* the font, and the width the control spends on what is not text */
};
static struct item items[MAX_ITEMS];
static int item_count;
static int dry;
static int resolution_rect[3];	/* the resolution popup's left, top and right */

static int text_width(const char *text, int font)
{
	Point bounds;
	SInt16 baseline;
	CFStringRef s = cs(text);
	int w;

	bounds.h = bounds.v = 0;
	if (GetThemeTextDimensions(s, (ThemeFontID)font, kThemeStateActive, false, &bounds, &baseline) != noErr ||
		bounds.h <= 0)
		w = (int)strlen(text) * (font == kThemeSmallSystemFont ? 6 : 7);	/* (no metrics: a rough width) */
	else
		w = bounds.h;
	CFRelease(s);
	return w;
}

static void note_item(char kind, const char *text, int left, int top, int right, int bottom, int font, int reserve)
{
	struct item *it;

	if (item_count >= MAX_ITEMS)
		return;
	it = &items[item_count++];
	it->kind = kind;
	snprintf(it->text, sizeof(it->text), "%s", text);
	it->left = (short)left;
	it->top = (short)top;
	it->right = (short)right;
	it->bottom = (short)bottom;
	it->font = font;
	it->reserve = reserve;
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

	note_item('l', text, left, top, right, bottom, small ? kThemeSmallSystemFont : kThemeSystemFont, 0);
	if (dry)
		return NULL;
	memset(&style, 0, sizeof(style));
	style.flags = kControlUseThemeFontIDMask | kControlUseJustMask;
	style.font = small ? kThemeSmallSystemFont : kThemeSystemFont;
	style.just = right_aligned ? teFlushRight : teFlushLeft;
	set_rect(&r, left, top, right, bottom);
	CreateStaticTextControl(window, &r, cs(text), &style, &c);
	return c;
}

/* the width of a label's text (and a little over), for the column of labels left of popups */
static int label_width(const char *const *texts, int count)
{
	int i, w = 0;

	for (i = 0; i < count; ++i)
		if (text_width(texts[i], kThemeSystemFont) > w)
			w = text_width(texts[i], kThemeSystemFont);
	return w + 6;
}

/* a help tag (the tooltip) for a control */
static void help(ControlRef c, const char *text)
{
	HMHelpContentRec tag;

	if (!c)
		return;
	memset(&tag, 0, sizeof(tag));
	tag.version = kMacHelpVersion;
	tag.tagSide = kHMDefaultSide;
	tag.content[kHMMinimumContentIndex].contentType = kHMCFStringContent;
	tag.content[kHMMinimumContentIndex].u.tagCFString = cs(text);
	tag.content[kHMMaximumContentIndex].contentType = kHMNoContent;
	HMSetControlHelpContent(c, &tag);
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

	note_item('c', title, left, top, right, top + 18, kThemeSystemFont, 22);
	if (dry)
		return NULL;
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

	note_item('r', title, left, top, right, top + 18, kThemeSystemFont, 22);
	if (dry)
		return NULL;
	set_rect(&r, left, top, right, top + 18);
	CreateRadioButtonControl(window, &r, cs(title), on ? 1 : 0, false, &c);
	SetControlCommandID(c, command);
	return c;
}

static ControlRef make_button(int left, int top, int right, const char *title, OSType command)
{
	Rect r;
	ControlRef c = NULL;

	note_item('b', title, left, top, right, top + 20, kThemeSystemFont, 28);
	if (dry)
		return NULL;
	set_rect(&r, left, top, right, top + 20);
	CreatePushButtonControl(window, &r, cs(title), &c);
	SetControlCommandID(c, command);
	return c;
}

static void make_box(int left, int top, int right, int bottom, const char *title)
{
	Rect r;
	ControlRef c = NULL;

	note_item('t', title, left, top, right, bottom, kThemeSmallSystemFont, 0);
	if (dry)
		return;
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
	int i;

	for (i = 0; i < count; ++i)
		note_item('p', choices[i].label, left, top, right, top + 20, kThemeMenuItemFont, 36);
	memset(p, 0, sizeof(*p));
	if (dry)
		return;
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

	resolution_choices[count].label = fullscreen ? "Native" : "Default";
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
	if (dry)
	{
		for (i = 0; i < count; ++i)
			note_item('p', resolution_choices[i].label, resolution_rect[0], resolution_rect[1], resolution_rect[2],
				resolution_rect[1] + 20, kThemeMenuItemFont, 36);
		return;
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
	snprintf(m->value[LC_RENDER], LC_VALUE, "%s", popup_value(&p_render));
	snprintf(m->value[LC_ADAPTIVE], LC_VALUE, "%s", popup_value(&p_adaptive));
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
	snprintf(m->value[LC_PROMPT_GLYPHS], LC_VALUE, "%s", popup_value(&p_glyphs));
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
		case CMD_ABOUT:
		case kHICommandAbout:
			if (environment->show_about)
				environment->show_about();
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

/* the dialog's layout: the controls (or, with dry, only the items to measure). Labels left of popups take the
   width their longest text measures, so the popups give room to what is longer in another language */
static void build(struct lc_model *m, ControlActionUPP action, ControlRef *play, ControlRef *quit)
{
	static const char *const left_labels[] = { "Resolution:", "Render resolution:", "Frame rate limit:" };
	static const char *const volume_label[] = { "Volume:" };
	static const char *const right_labels[] = { "Detail:", "Texture quality:", "Anti-aliasing:", "Shadow quality:", "Adaptive detail:", "Gamma:" };
	static const char *const options_labels[] = { "Button symbols:", "Game data:" };
	int lw, px, rw, rx, wide;
	Rect bounds;
	ControlRef c;

	item_count = 0;
	if (!dry)
		make_icon();
	/* port: the About window's button, at the top right (the tagline stops short of it) */
	{
		int about_width = text_width("About PowerSpark\xE2\x80\xA6", kThemeSystemFont) + 40;

		make_label(76, 14, 460, 38, "PowerSpark", 0, 0);
		make_label(76, 38, 624 - about_width - 8, 56, "Halo: Combat Evolved. Choose your settings, then click Play.", 0, 1);
		make_button(624 - about_width, 14, 624, "About PowerSpark\xE2\x80\xA6", CMD_ABOUT);
	}

	/* Display */
	lw = label_width(left_labels, 3);
	px = 28 + lw + 6;
	make_box(16, 72, 312, 272, "Display");
	c_full = make_radio(32, 94, 160, "Full Screen", fullscreen_selected, CMD_FULL);
	c_window = make_radio(170, 94, 298, "Windowed", !fullscreen_selected, CMD_WINDOW);
	make_label(28, 126, 28 + lw, 144, left_labels[0], 1, 0);
	resolution_rect[0] = px;
	resolution_rect[1] = 122;
	resolution_rect[2] = 298;
	make_popup(&p_resolution, px, 122, 298, resolution_choices, 0, "");
	fill_resolution(fullscreen_selected);
	make_label(28, 154, 28 + lw, 172, left_labels[1], 1, 0);
	make_popup(&p_render, px, 150, 298, render_choices, 9, m->value[LC_RENDER]);
	help(p_render.control, "How many pixels the game draws a frame with. Native draws the window's own; 480 lines is the Xbox's, scaled up. More than the card holds is cut down.");
	c_wide = make_check(32, 180, 298, "Widescreen", strcmp(m->value[LC_SCALING], "original") != 0, 0);
	help(c_wide, "A window or display wider than 4:3 shows a wider view. Off: the Xbox's 4:3 picture, with bars.");
	c_vsync = make_check(32, 206, 130, "V-Sync", lc_bool(m->value[LC_VSYNC]), 0);
	help(c_vsync, "Wait for the display between frames, so that the picture never tears.");
	c_smooth = make_check(140, 206, 298, "Smooth motion", lc_bool(m->value[LC_INTERPOLATION]), 0);
	help(c_smooth, "Draw a frame for every display refresh, blending between the game's 30 ticks a second.");
	make_label(28, 238, 28 + lw, 256, left_labels[2], 1, 0);
	make_popup(&p_fps, px, 234, 298, fps_choices, 8, m->value[LC_MAX_FPS]);
	help(p_fps.control, "With V-Sync off, the most frames a second. Auto: twice the display's refresh rate.");

	/* Sound */
	make_box(16, 280, 312, 362, "Sound");
	c_sound = make_check(32, 302, 150, "Sound", lc_bool(m->value[LC_AUDIO]), CMD_SOUND);
	c_reverb = make_check(170, 302, 298, "Reverb", lc_bool(m->value[LC_REVERB]), 0);
	rw = label_width(volume_label, 1);
	make_label(28, 332, 28 + rw, 350, "Volume:", 1, 0);
	if (!dry)
	{
		set_rect(&bounds, 28 + rw + 6, 332, 248, 350);
		c_volume = NULL;
		CreateSliderControl(window, &bounds, lc_tenths(m->value[LC_VOLUME], 0, 10), 0, 10, kControlSliderDoesNotPoint,
			0, true, action, &c_volume);
	}
	c_volume_text = make_label(254, 334, 306, 350, "100%", 0, 1);

	/* Graphics */
	rw = label_width(right_labels, 6);
	rx = 336 + rw + 6;
	make_box(328, 72, 624, 362, "Graphics");
	make_label(336, 98, 336 + rw, 116, right_labels[0], 1, 0);
	make_popup(&p_detail, rx, 94, 616, detail_choices, 5, m->value[LC_DETAIL]);
	help(p_detail.control, "High draws as the Xbox did; Low leaves out specular, shadows, decals and small things. Auto picks by Mac.");
	make_label(336, 130, 336 + rw, 148, right_labels[1], 1, 0);
	make_popup(&p_texture, rx, 126, 616, texture_choices, 5, m->value[LC_TEXTURE_SKIP]);
	help(p_texture.control, "Smaller textures use less video memory. Auto picks by the card.");
	make_label(336, 162, 336 + rw, 180, right_labels[2], 1, 0);
	make_popup(&p_aa, rx, 158, 616, aa_choices, 2, m->value[LC_ANTI_ALIASING]);
	make_label(336, 194, 336 + rw, 212, right_labels[3], 1, 0);
	make_popup(&p_shadow, rx, 190, 616, shadow_choices, 4, m->value[LC_SHADOW_RESOLUTION]);
	make_label(336, 226, 336 + rw, 244, right_labels[4], 1, 0);
	make_popup(&p_adaptive, rx, 222, 616, adaptive_choices, 6, m->value[LC_ADAPTIVE]);
	help(p_adaptive.control, "Lowers small details (grass, small objects, effects, lens flares, at last the resolution) while frames take longer than the target, and puts them back when there is time. Auto: on for a PowerPC Mac, off for an Intel one. The game itself plays the same.");
	make_label(336, 256, 336 + rw, 274, right_labels[5], 1, 0);
	if (!dry)
	{
		set_rect(&bounds, rx, 256, 566, 274);
		c_gamma = NULL;
		CreateSliderControl(window, &bounds, lc_tenths(m->value[LC_GAMMA], 5, 20), 5, 20, kControlSliderDoesNotPoint,
			0, true, action, &c_gamma);
	}
	c_gamma_text = make_label(574, 258, 616, 274, "1.0", 0, 1);
	c_hud = make_check(344, 284, 616, "High-resolution HUD", lc_bool(m->value[LC_HIRES_HUD]), 0);
	c_text = make_check(344, 310, 616, "High-resolution text", lc_bool(m->value[LC_HIRES_TEXT]), 0);
	c_ppl = make_check(344, 336, 616, "Per-pixel lighting (not available)", lc_bool(m->value[LC_PER_PIXEL_LIGHTING]), 0);
	/* port: the Mac renderer's shader translator does not make per-pixel lighting (mac_nv2a_arbfp.c refuses it), so the
	   row stays, as every Mac has the same options, but it is disabled and says why */
	help(c_ppl, "Not available on this renderer yet: the Mac's shaders do not draw per-pixel lighting, so this setting changes nothing. The game logs it if it is on in config.toml.");
	if (!dry)
		DisableControl(c_ppl);

	/* Options */
	make_box(16, 370, 624, 514, "Options");
	c_movies = make_check(32, 392, 300, "Play intro movies", lc_bool(m->value[LC_MOVIES]), 0);
	c_achievements = make_check(320, 392, 612, "Achievements", lc_bool(m->value[LC_ACHIEVEMENTS]), 0);
	rw = label_width(options_labels, 2);
	wide = 0;
	{
		int i;

		for (i = 0; i < 3; ++i)
		{
			if (text_width(source_choices[i].label, kThemeMenuItemFont) > wide)
				wide = text_width(source_choices[i].label, kThemeMenuItemFont);
			if (text_width(glyph_choices[i].label, kThemeMenuItemFont) > wide)
				wide = text_width(glyph_choices[i].label, kThemeMenuItemFont);
		}
	}
	make_label(28, 422, 28 + rw, 440, options_labels[0], 1, 0);
	make_popup(&p_glyphs, 28 + rw + 6, 418, 28 + rw + 6 + wide + 56, glyph_choices, 3, m->value[LC_PROMPT_GLYPHS]);
	help(p_glyphs.control, "The symbols the game's prompts and button icons show: Xbox, PlayStation, or Auto, which follows the pad in use. Choose one when an adapter (an 8BitDo in Dinput mode) cannot say which pad is paired.");
	make_label(28 + rw + 6 + wide + 56 + 10, 422, 612, 440, "Auto follows a wired PlayStation pad.", 0, 1);
	make_label(28, 448, 28 + rw, 466, options_labels[1], 1, 0);
	make_popup(&p_source, 28 + rw + 6, 444, 28 + rw + 6 + wide + 56, source_choices, 3, m->data_source);
	c_status = make_label(28 + rw + 6 + wide + 56 + 10, 442, 612, 474, "Found: Xbox disc data and MacSoft's Halo.", 0, 1);
	make_button(32, 482, 32 + text_width("Set Up from Disc Image\xE2\x80\xA6", kThemeSystemFont) + 80,
		"Set Up from Disc Image\xE2\x80\xA6", CMD_DISC);
	{
		/* Controller Setup is built for Mac OS X 10.5 (IOHIDManager) */
		ControlRef pad = make_button(32 + text_width("Set Up from Disc Image\xE2\x80\xA6", kThemeSystemFont) + 92, 482,
			32 + text_width("Set Up from Disc Image\xE2\x80\xA6", kThemeSystemFont) + 92 +
			text_width("Controller Setup\xE2\x80\xA6", kThemeSystemFont) + 80, "Controller Setup\xE2\x80\xA6", CMD_PAD);
		int major = 10, minor = 5;

		lu_system_version(&major, &minor);
		if (pad && major == 10 && minor < 5)
		{
			DisableControl(pad);
			help(pad, "Controller Setup needs Mac OS X 10.5 or later.");
		}
	}

	/* the buttons */
	c_hide = make_check(20, 524, 330, "Don't show this at startup", m->hide, 0);
	make_label(38, 546, 440, 574, "Hold Option while opening PowerSpark to show this window again.", 0, 1);
	*quit = make_button(456, 530, 536, "Quit", CMD_QUIT);
	*play = make_button(544, 530, 624, "Play", CMD_PLAY);
	(void)c;
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
	dry = 0;
	snprintf(resolution_value[0], LC_VALUE, "%s", m->value[LC_RESOLUTION]);
	snprintf(resolution_value[1], LC_VALUE, "%s", m->value[LC_WINDOW_SIZE]);
	fullscreen_selected = strcmp(m->value[LC_MODE], "windowed") != 0;

	set_rect(&bounds, 100, 100, 100 + WINDOW_WIDTH, 100 + WINDOW_HEIGHT);
	if (CreateNewWindow(kMovableModalWindowClass,
		kWindowCompositingAttribute | kWindowStandardHandlerAttribute, &bounds,
		&window) != noErr)
		return -1;
	SetWindowTitleWithCFString(window, cs("PowerSpark"));
	/* (the application menu's About item) */
	EnableMenuCommand(NULL, kHICommandAbout);
	build(m, action, &play, &quit);
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

/* every control's rectangle and its text's width in the system font, without a window; an item whose text does
   not fit (width plus what the control spends on its box or arrow, against its width) is flagged. Items with
   less than a fifth to spare are marked "tight" (another language's longer text may not fit; labels are not,
   a column of them being as wide as its longest text). Returns the number of items that do not fit */
int ld_layout_report(struct lc_model *m, FILE *out)
{
	ControlRef play, quit;
	int i, bad = 0, tight = 0, right_edge = 0, bottom_edge = 0;

	model = m;
	dry = 1;
	snprintf(resolution_value[0], LC_VALUE, "%s", m->value[LC_RESOLUTION]);
	snprintf(resolution_value[1], LC_VALUE, "%s", m->value[LC_WINDOW_SIZE]);
	fullscreen_selected = strcmp(m->value[LC_MODE], "windowed") != 0;
	build(m, NULL, &play, &quit);
	dry = 0;
	fprintf(out, "  %-2s %-34s %-22s %5s %5s  %s\n", "k", "text", "rect (l,t,r,b)", "text", "room", "");
	for (i = 0; i < item_count; ++i)
	{
		const struct item *it = &items[i];
		int w = text_width(it->text, it->font), room = it->right - it->left - it->reserve;
		const char *flag = "";

		if (it->kind == 't' || it->right - it->left <= 0)
			room = it->right - it->left - 24;
		if (it->kind != 'p' || it->right > it->left)
		{
			if (w > room)
			{
				flag = "DOES NOT FIT";
				++bad;
			}
			else if (it->kind != 'l' && w * 5 > room * 4)
			{
				flag = "tight";
				++tight;
			}
		}
		if (it->right > right_edge)
			right_edge = it->right;
		if (it->bottom > bottom_edge)
			bottom_edge = it->bottom;
		fprintf(out, "  %-2c %-34.34s (%3d,%3d,%3d,%3d) %5d %5d  %s\n", it->kind, it->text, it->left, it->top, it->right,
			it->bottom, w, room, flag);
	}
	fprintf(out, "  %d items, %d do not fit, %d tight; the layout reaches %d x %d of a %d x %d window\n", item_count, bad,
		tight, right_edge, bottom_edge, WINDOW_WIDTH, WINDOW_HEIGHT);
	if (right_edge > WINDOW_WIDTH || bottom_edge > WINDOW_HEIGHT)
		++bad;
	return bad;
}
