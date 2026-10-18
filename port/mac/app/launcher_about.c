/* See launcher_about.h. Like launcher_dialog.c the windows are Control Manager controls made in code (no nib).
   Each window's layout is first worked out as a list of items with their rectangles, from the text as this
   system sets it (a paragraph is measured at its width, so that Tiger's wider Lucida Grande does not cut
   the last line); the same list makes the controls and prints --about-dump's report. */
#include "launcher_about.h"
#include "feedback_report.h"
#include <Carbon/Carbon.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define ABOUT_WIDTH 520
#define FEEDBACK_WIDTH 520
#define MARGIN 24
#define MAX_ITEMS 48
#define PROJECT_URL "https://github.com/OpenCommunityEdition/OpenCE"
#define PROJECT_TITLE "OpenCE Project"

enum
{
	CMD_CLOSE = 'ACLS', CMD_FEEDBACK = 'AFBK', CMD_LINK = 'ALK0',	/* LINK + 0 is OpenCE, then the branding's */
	CMD_SEND = 'FSND', CMD_CANCEL = 'FCNL'
};

enum { KIND_ICON = 'I', KIND_LABEL = 'l', KIND_PARA = 'P', KIND_SEP = 'S', KIND_LOGO = 'G', KIND_BUTTON = 'b',
	KIND_CHECK = 'c', KIND_POPUP = 'p', KIND_EDIT = 'e', KIND_TEXT = 'x' };

struct item
{
	char kind;
	char text[1304];
	short left, top, right, bottom;
	int font;
	int reserve;		/* the width a control spends on what is not text */
	unsigned long command;
	int width, height;	/* the text's size as measured (a paragraph: wrapped to the rectangle's width) */
};

static struct item items[MAX_ITEMS];
static int item_count;
static int layout_height;

/* ---------- measuring */

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

/* the text's size, wrapped to wrap_width when that is more than 0 */
static void measure(const char *text, int font, int wrap_width, int *width, int *height)
{
	Point bounds;
	SInt16 baseline;
	CFStringRef s = cs(text);

	bounds.h = (short)wrap_width;
	bounds.v = 0;
	if (GetThemeTextDimensions(s, (ThemeFontID)font, kThemeStateActive, wrap_width > 0, &bounds, &baseline) != noErr ||
		bounds.h <= 0 || bounds.v <= 0)
	{
		/* (no metrics: a rough size, generous) */
		int per = font == kThemeSmallSystemFont || font == kThemeSmallEmphasizedSystemFont ? 6 : 7, line = per * 2;
		int chars = (int)strlen(text), columns = wrap_width > 0 ? wrap_width / per : chars, lines = 1;

		if (columns > 0 && chars > columns)
			lines = chars / columns + 2;
		bounds.h = (short)(wrap_width > 0 ? wrap_width : chars * per);
		bounds.v = (short)(lines * (line - 1));
	}
	CFRelease(s);
	*width = bounds.h;
	*height = bounds.v;
}

static int text_width(const char *text, int font)
{
	int w, h;

	measure(text, font, 0, &w, &h);
	return w;
}

static struct item *add_item(char kind, const char *text, int left, int top, int right, int bottom, int font,
	int reserve, unsigned long command)
{
	struct item *it;

	if (item_count >= MAX_ITEMS)
		return NULL;
	it = &items[item_count++];
	memset(it, 0, sizeof(*it));
	it->kind = kind;
	snprintf(it->text, sizeof(it->text), "%s", text ? text : "");
	it->left = (short)left;
	it->top = (short)top;
	it->right = (short)right;
	it->bottom = (short)bottom;
	it->font = font;
	it->reserve = reserve;
	it->command = command;
	return it;
}

/* a paragraph at the width, as tall as this system sets it (and two lines' slack under it for another font);
   returns the y below it */
static int add_paragraph(const char *text, int font, int left, int top, int width)
{
	struct item *it;
	int w, h;

	if (!text[0])
		return top;
	measure(text, font, width, &w, &h);
	it = add_item(KIND_PARA, text, left, top, left + width, top + h + 3, font, 0, 0);
	if (it)
	{
		it->width = w;
		it->height = h;
	}
	return top + h + 3;
}

static struct item *add_label(const char *text, int font, int left, int top, int width)
{
	int w, h;
	struct item *it;

	measure(text, font, 0, &w, &h);
	it = add_item(KIND_LABEL, text, left, top, left + width, top + (h > 14 ? h + 3 : 17), font, 0, 0);
	if (it)
	{
		it->width = w;
		it->height = h;
	}
	return it;
}

static struct item *add_button(const char *text, int left, int top, int width, unsigned long command)
{
	struct item *it = add_item(KIND_BUTTON, text, left, top, left + width, top + 20, kThemeSystemFont, 28, command);

	if (it)
		it->width = text_width(text, kThemeSystemFont);
	return it;
}

/* ---------- the application's facts */

static void bundle_text(const char *key, char *out, size_t size, const char *fallback)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef k = cs(key);
	CFTypeRef v = bundle && k ? CFBundleGetValueForInfoDictionaryKey(bundle, k) : NULL;

	snprintf(out, size, "%s", fallback);
	if (v && CFGetTypeID(v) == CFStringGetTypeID())
		CFStringGetCString((CFStringRef)v, out, (CFIndex)size, kCFStringEncodingUTF8);
	if (k)
		CFRelease(k);
}

static void version_line(char *out, size_t size, char *version, size_t vsize, char *build, size_t bsize)
{
	bundle_text("CFBundleShortVersionString", version, vsize, "?");
	bundle_text("CFBundleVersion", build, bsize, "?");
	snprintf(out, size, "Version %s (build %s)", version, build);
}

/* ---------- the About window's layout */

static const char *const disclaimer =
	"PowerSpark is not made by or affiliated with Microsoft, Bungie or 343 Industries. It is built on OpenCE, a "
	"community port of the Xbox game's decompilation, and plays from your own copy of the game: an Xbox disc "
	"image, or Halo for Mac if you have it installed.";
static const char *const credit =
	"OpenCE is the work of its contributors, who have released it into the public domain. PowerSpark is the "
	"Mac version of that port.";

static int feedback_available(const struct la_env *env)
{
	return env->branding && lb_feedback_ready(env->branding);
}

/* the About window: returns its height */
static int layout_about(const struct la_env *env)
{
	const struct lb_branding *b = env->branding;
	char version[32], build[32], line[96], stage[32];
	int y, x, row_top, i, count, widths[LB_LINKS + 2], total, column = ABOUT_WIDTH - 2 * MARGIN;
	struct item *it;

	item_count = 0;
	version_line(line, sizeof(line), version, sizeof(version), build, sizeof(build));
	bundle_text("OpenCEBuildStage", stage, sizeof(stage), "");

	add_item(KIND_ICON, "", MARGIN, 20, MARGIN + 64, 84, 0, 0, 0);
	add_label("PowerSpark", kThemeAlertHeaderFont, MARGIN + 80, 24, column - 80);
	add_label(line, kThemeSmallSystemFont, MARGIN + 80, 46, column - 80);
	add_label("Halo: Combat Evolved for PowerPC and Intel Macs", kThemeSmallSystemFont, MARGIN + 80, 62, column - 80);
	y = 96;
	add_item(KIND_SEP, "", MARGIN, y, ABOUT_WIDTH - MARGIN, y + 2, 0, 0, 0);
	y += 12;
	if (stage[0])
	{
		char note[160];

		snprintf(note, sizeof(note), "This is an %s build. Expect rough edges, and please say when you find one.", stage);
		y = add_paragraph(note, kThemeSmallEmphasizedSystemFont, MARGIN, y, column) + 6;
	}
	y = add_paragraph(disclaimer, kThemeSmallSystemFont, MARGIN, y, column) + 6;
	y = add_paragraph(credit, kThemeSmallSystemFont, MARGIN, y, column) + 6;
	if (b->publisher[0])
		y = add_paragraph(b->publisher, kThemeSmallSystemFont, MARGIN, y, column) + 8;
	if (b->logo_path[0])
	{
		CGDataProviderRef provider;
		CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)b->logo_path, (CFIndex)strlen(b->logo_path),
			false);
		CGImageRef image = NULL;
		int w = 0, h = 0;

		provider = url ? CGDataProviderCreateWithURL(url) : NULL;
		if (provider)
		{
			image = CGImageCreateWithPNGDataProvider(provider, NULL, false, kCGRenderingIntentDefault);
			CGDataProviderRelease(provider);
		}
		if (url)
			CFRelease(url);
		if (image)
		{
			w = (int)CGImageGetWidth(image);
			h = (int)CGImageGetHeight(image);
			CGImageRelease(image);
		}
		if (w > 0 && h > 0)
		{
			int lw = w < 260 ? w : 260, lh = lw * h / w;

			add_item(KIND_LOGO, b->logo_path, (ABOUT_WIDTH - lw) / 2, y, (ABOUT_WIDTH - lw) / 2 + lw, y + lh, 0, 0, 0);
			y += lh + 8;
		}
	}
	y += 6;

	/* the links (OpenCE's, then the build's), as wide as their text, in rows */
	count = 1 + b->link_count;
	widths[0] = text_width(PROJECT_TITLE, kThemeSystemFont) + 40;
	for (i = 0; i < b->link_count; ++i)
		widths[1 + i] = text_width(b->links[i].title, kThemeSystemFont) + 40;
	x = MARGIN;
	row_top = y;
	for (i = 0; i < count; ++i)
	{
		if (x > MARGIN && x + widths[i] > ABOUT_WIDTH - MARGIN)
		{
			x = MARGIN;
			row_top += 30;
		}
		it = add_button(i == 0 ? PROJECT_TITLE : b->links[i - 1].title, x, row_top, widths[i], CMD_LINK + i);
		(void)it;
		x += widths[i] + 8;
	}
	y = row_top + 30 + 6;

	/* Send Feedback (if the build has an endpoint) on the left, Close on the right */
	total = text_width("Close", kThemeSystemFont) + 40;
	if (total < 80)
		total = 80;
	if (feedback_available(env))
		add_button("Send Feedback\xE2\x80\xA6", MARGIN, y, text_width("Send Feedback\xE2\x80\xA6", kThemeSystemFont) + 40,
			CMD_FEEDBACK);
	add_button("Close", ABOUT_WIDTH - MARGIN - total, y, total, CMD_CLOSE);
	return y + 20 + 20;
}

/* ---------- the Feedback window's layout */

static int topic_width(void)
{
	int i, w = 0;

	for (i = 0; i < FB_TOPICS; ++i)
		if (text_width(fb_topics[i], kThemeMenuItemFont) > w)
			w = text_width(fb_topics[i], kThemeMenuItemFont);
	return w;
}

/* a note of what travels with a report */
static void sent_note(const struct la_env *env, char *out, size_t size)
{
	char version[32], build[32], line[96];

	version_line(line, sizeof(line), version, sizeof(version), build, sizeof(build));
	(void)env;
	snprintf(out, size, "Sent with this: PowerSpark %s (build %s), your Mac's model, system and processor, and "
		"the log if you tick it. Nothing else, and nothing until you press Send.", version, build);
}

static int layout_feedback(const struct la_env *env)
{
	int y = 20, column = FEEDBACK_WIDTH - 2 * MARGIN, lw, ew;
	char note[400];
	struct item *it;

	item_count = 0;
	lw = text_width("What is this about?", kThemeSystemFont) + 6;
	add_label("What is this about?", kThemeSystemFont, MARGIN, y + 3, lw);
	it = add_item(KIND_POPUP, fb_topics[0], MARGIN + lw + 8, y, MARGIN + lw + 8 + topic_width() + 56, y + 20,
		kThemeMenuItemFont, 36, 0);
	if (it)
		it->width = topic_width();
	y += 36;
	add_label("What happened?", kThemeSystemFont, MARGIN, y, column);
	y += 22;
	add_item(KIND_TEXT, "", MARGIN, y, MARGIN + column, y + 150, 0, 0, 0);
	y += 150 + 14;
	lw = text_width("Your email (only if you want an answer):", kThemeSystemFont) + 6;
	add_label("Your email (only if you want an answer):", kThemeSystemFont, MARGIN, y + 3, lw);
	ew = column - lw - 8;
	add_item(KIND_EDIT, "", MARGIN + lw + 8, y, MARGIN + lw + 8 + ew, y + 22, 0, 0, 0);
	y += 34;
	it = add_item(KIND_CHECK, "Include the end of PowerSpark's latest log", MARGIN, y,
		MARGIN + column, y + 18, kThemeSystemFont, 22, 0);
	if (it)
		it->width = text_width(it->text, kThemeSystemFont);
	y += 30;
	sent_note(env, note, sizeof(note));
	y = add_paragraph(note, kThemeSmallSystemFont, MARGIN, y, column) + 14;
	{
		int cw = text_width("Cancel", kThemeSystemFont) + 40, sw = text_width("Send", kThemeSystemFont) + 40;

		if (cw < 84)
			cw = 84;
		if (sw < 84)
			sw = 84;
		add_label("", kThemeSmallSystemFont, MARGIN, y + 3, FEEDBACK_WIDTH - 2 * MARGIN - cw - sw - 20);
		add_button("Cancel", FEEDBACK_WIDTH - MARGIN - sw - 10 - cw, y, cw, CMD_CANCEL);
		add_button("Send", FEEDBACK_WIDTH - MARGIN - sw, y, sw, CMD_SEND);
	}
	return y + 20 + 20;
}

/* ---------- the layout report */

static int report_items(FILE *out, int width, int height)
{
	int i, bad = 0, right = 0, bottom = 0;

	fprintf(out, "  %-2s %-34s %-22s %5s %5s  %s\n", "k", "text", "rect (l,t,r,b)", "text", "room", "");
	for (i = 0; i < item_count; ++i)
	{
		const struct item *it = &items[i];
		int size = 0, room = 0;
		const char *flag = "";

		switch (it->kind)
		{
		case KIND_LABEL:
			size = it->width;
			room = it->right - it->left;
			break;
		case KIND_BUTTON:
		case KIND_CHECK:
			size = it->width;
			room = it->right - it->left - it->reserve;
			break;
		case KIND_PARA:
			size = it->height;
			room = it->bottom - it->top;
			break;
		case KIND_POPUP:
			size = it->width;
			room = it->right - it->left - it->reserve;
			break;
		}
		if (size > room && (it->kind == KIND_LABEL || it->kind == KIND_BUTTON || it->kind == KIND_CHECK ||
			it->kind == KIND_PARA || it->kind == KIND_POPUP))
		{
			flag = "DOES NOT FIT";
			++bad;
		}
		else if (it->kind == KIND_CHECK && size * 5 > room * 4)
			flag = "tight";
		if (it->right > right)
			right = it->right;
		if (it->bottom > bottom)
			bottom = it->bottom;
		fprintf(out, "  %-2c %-34.34s (%3d,%3d,%3d,%3d) %5d %5d  %s\n", it->kind, it->text, it->left, it->top, it->right,
			it->bottom, size, room, flag);
	}
	fprintf(out, "  %d items, %d do not fit; the layout reaches %d x %d of a %d x %d window\n", item_count, bad, right, bottom,
		width, height);
	if (right > width || bottom > height)
	{
		fprintf(out, "  DOES NOT FIT: the layout is beyond the window\n");
		++bad;
	}
	return bad;
}

int la_layout_report(const struct la_env *env, FILE *out)
{
	int bad;

	fprintf(out, "About window (%d wide; feedback %s; publisher %s; logo %s; %d links)\n", ABOUT_WIDTH,
		feedback_available(env) ? "on" : "off (no endpoint, CA list or sender)",
		env->branding->publisher[0] ? "yes" : "no", env->branding->logo_path[0] ? "yes" : "no",
		env->branding->link_count);
	layout_height = layout_about(env);
	bad = report_items(out, ABOUT_WIDTH, layout_height);
	fprintf(out, "Send Feedback window (%d wide)\n", FEEDBACK_WIDTH);
	layout_height = layout_feedback(env);
	bad += report_items(out, FEEDBACK_WIDTH, layout_height);
	return bad;
}

/* ---------- making the controls */

struct window_state
{
	WindowRef window;
	const struct la_env *env;
	int done;
	/* About */
	int pending_feedback;
	/* Feedback */
	ControlRef popup, check_log, edit, status, send, cancel, close_button;
	HIViewRef text;
	MenuRef menu;
	char *log;
	pid_t child;
	int pipe_fd;
	char answer[512];
	size_t answer_length;
	char report_path[1024];
	EventLoopTimerRef timer;
};

static ControlRef make_label_control(WindowRef w, const struct item *it, int right_aligned)
{
	Rect r;
	ControlRef c = NULL;
	ControlFontStyleRec style;
	CFStringRef s = cs(it->text);

	memset(&style, 0, sizeof(style));
	style.flags = kControlUseThemeFontIDMask | kControlUseJustMask;
	style.font = (SInt16)it->font;
	style.just = right_aligned ? teFlushRight : teFlushLeft;
	set_rect(&r, it->left, it->top, it->right, it->bottom);
	CreateStaticTextControl(w, &r, s, &style, &c);
	CFRelease(s);
	return c;
}

static void set_label(ControlRef c, const char *text)
{
	CFStringRef s = cs(text);

	SetControlData(c, kControlEntireControl, kControlStaticTextCFStringTag, sizeof(s), &s);
	CFRelease(s);
	HIViewSetNeedsDisplay((HIViewRef)c, true);
}

static ControlRef make_button_control(WindowRef w, const struct item *it)
{
	Rect r;
	ControlRef c = NULL;
	CFStringRef s = cs(it->text);

	set_rect(&r, it->left, it->top, it->right, it->bottom);
	CreatePushButtonControl(w, &r, s, &c);
	CFRelease(s);
	if (c)
		SetControlCommandID(c, (UInt32)it->command);
	return c;
}

static void make_icon_control(WindowRef w, const struct item *it, const char *app_path)
{
	FSRef ref;
	IconRef icon = NULL;
	SInt16 label;
	ControlButtonContentInfo content;
	Rect r;
	ControlRef c = NULL;

	if (!app_path || FSPathMakeRef((const UInt8 *)app_path, &ref, NULL) != noErr)
		return;
	if (GetIconRefFromFileInfo(&ref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, &icon, &label) != noErr)
		return;
	memset(&content, 0, sizeof(content));
	content.contentType = kControlContentIconRef;
	content.u.iconRef = icon;
	set_rect(&r, it->left, it->top, it->right, it->bottom);
	CreateIconControl(w, &r, &content, true, &c);
}

static HIViewRef content_view(WindowRef w)
{
	HIViewRef content = NULL;

	if (HIViewFindByID(HIViewGetRoot(w), kHIViewWindowContentID, &content) != noErr)
		return NULL;
	return content;
}

static void make_logo_control(WindowRef w, const struct item *it)
{
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)it->text, (CFIndex)strlen(it->text), false);
	CGDataProviderRef provider = url ? CGDataProviderCreateWithURL(url) : NULL;
	CGImageRef image = provider ? CGImageCreateWithPNGDataProvider(provider, NULL, false, kCGRenderingIntentDefault) : NULL;
	HIViewRef view = NULL, content = content_view(w);
	HIRect frame;

	if (provider)
		CGDataProviderRelease(provider);
	if (url)
		CFRelease(url);
	if (image && content && HIImageViewCreate(image, &view) == noErr && view)
	{
		frame = CGRectMake(it->left, it->top, it->right - it->left, it->bottom - it->top);
		HIImageViewSetScaleToFit(view, true);
		HIViewSetFrame(view, &frame);
		HIViewAddSubview(content, view);
		HIViewSetVisible(view, true);
	}
	if (image)
		CGImageRelease(image);
}

static void make_controls(struct window_state *s, const char *app_path)
{
	int i;

	for (i = 0; i < item_count; ++i)
	{
		const struct item *it = &items[i];
		Rect r;
		ControlRef c = NULL;

		switch (it->kind)
		{
		case KIND_ICON:
			make_icon_control(s->window, it, app_path);
			break;
		case KIND_LABEL:
		case KIND_PARA:
			c = make_label_control(s->window, it, 0);
			if (i > 0 && it->kind == KIND_LABEL && it->text[0] == 0)
				s->status = c;
			break;
		case KIND_SEP:
			set_rect(&r, it->left, it->top, it->right, it->bottom);
			CreateSeparatorControl(s->window, &r, &c);
			break;
		case KIND_LOGO:
			make_logo_control(s->window, it);
			break;
		case KIND_BUTTON:
			c = make_button_control(s->window, it);
			if (it->command == CMD_SEND)
				s->send = c;
			else if (it->command == CMD_CANCEL)
				s->cancel = c;
			else if (it->command == CMD_CLOSE)
				s->close_button = c;
			break;
		case KIND_CHECK:
		{
			CFStringRef t = cs(it->text);

			set_rect(&r, it->left, it->top, it->right, it->bottom);
			CreateCheckBoxControl(s->window, &r, t, 0, true, &s->check_log);
			CFRelease(t);
			break;
		}
		case KIND_POPUP:
		{
			static SInt16 menu_id = 400;
			int k;

			set_rect(&r, it->left, it->top, it->right, it->bottom);
			CreatePopupButtonControl(s->window, &r, NULL, -12345, false, 0, 0, 0, &s->popup);
			CreateNewMenu(menu_id++, 0, &s->menu);
			for (k = 0; k < FB_TOPICS; ++k)
			{
				CFStringRef t = cs(fb_topics[k]);

				AppendMenuItemTextWithCFString(s->menu, t, 0, 0, NULL);
				CFRelease(t);
			}
			SetControlData(s->popup, kControlEntireControl, kControlPopupButtonMenuRefTag, sizeof(MenuRef), &s->menu);
			SetControl32BitMinimum(s->popup, 1);
			SetControl32BitMaximum(s->popup, FB_TOPICS);
			SetControl32BitValue(s->popup, 1);
			break;
		}
		case KIND_EDIT:
		{
			set_rect(&r, it->left, it->top, it->right, it->bottom);
			CreateEditUnicodeTextControl(s->window, &r, CFSTR(""), false, NULL, &s->edit);
			break;
		}
		case KIND_TEXT:
		{
			/* a bezel around a scrolling text view */
			HIViewRef content = content_view(s->window);
			HIRect frame;
			ControlRef box = NULL;

			set_rect(&r, it->left, it->top, it->right, it->bottom);
			CreateGroupBoxControl(s->window, &r, NULL, false, &box);
			frame = CGRectMake(it->left + 3, it->top + 3, it->right - it->left - 6, it->bottom - it->top - 6);
			if (content && HITextViewCreate(&frame, 0, kTXNWantVScrollBarMask | kTXNAlwaysWrapAtViewEdgeMask |
				kTXNMonostyledTextMask, &s->text) == noErr && s->text)
			{
				HIViewAddSubview(content, s->text);
				HIViewSetVisible(s->text, true);
			}
			break;
		}
		}
		(void)c;
	}
}

/* the text view's text as a malloc'ed UTF-8 string */
static char *text_view_string(HIViewRef view)
{
	TXNObject txn = HITextViewGetTXNObject(view);
	Handle h = NULL;
	CFStringRef s;
	char *out;
	CFIndex need;

	if (!txn || TXNGetDataEncoded(txn, kTXNStartOffset, kTXNEndOffset, &h, kTXNUnicodeTextData) != noErr || !h)
		return strdup("");
	HLock(h);
	s = CFStringCreateWithCharacters(NULL, (const UniChar *)*h, GetHandleSize(h) / 2);
	HUnlock(h);
	DisposeHandle(h);
	if (!s)
		return strdup("");
	need = CFStringGetMaximumSizeForEncoding(CFStringGetLength(s), kCFStringEncodingUTF8) + 1;
	out = (char *)malloc((size_t)need);
	if (out && !CFStringGetCString(s, out, need, kCFStringEncodingUTF8))
		out[0] = 0;
	CFRelease(s);
	return out ? out : strdup("");
}

static void set_text_view_string(HIViewRef view, const char *text)
{
	TXNObject txn = HITextViewGetTXNObject(view);
	CFStringRef s = cs(text);
	CFIndex n = CFStringGetLength(s);
	UniChar *chars = (UniChar *)malloc((size_t)(n + 1) * sizeof(UniChar));

	if (chars && txn)
	{
		CFStringGetCharacters(s, CFRangeMake(0, n), chars);
		TXNSetData(txn, kTXNUnicodeTextData, chars, (ByteCount)n * 2, kTXNStartOffset, kTXNEndOffset);
	}
	free(chars);
	CFRelease(s);
}

static char *edit_string(ControlRef c)
{
	CFStringRef s = NULL;
	char *out = NULL;
	CFIndex need;

	if (GetControlData(c, kControlEntireControl, kControlEditTextCFStringTag, sizeof(s), &s, NULL) == noErr && s)
	{
		need = CFStringGetMaximumSizeForEncoding(CFStringGetLength(s), kCFStringEncodingUTF8) + 1;
		out = (char *)malloc((size_t)need);
		if (out && !CFStringGetCString(s, out, need, kCFStringEncodingUTF8))
			out[0] = 0;
		CFRelease(s);
	}
	return out ? out : strdup("");
}

static int quiet;	/* the self-test: alerts are printed, not shown */

static void note_alert(const char *title, const char *message)
{
	CFStringRef t, m, ok;
	CFOptionFlags response;

	if (quiet)
	{
		printf("  (alert) %s: %s\n", title, message);
		return;
	}
	t = cs(title);
	m = cs(message);
	ok = cs("OK");

	CFUserNotificationDisplayAlert(0, kCFUserNotificationNoteAlertLevel, NULL, NULL, NULL, t, m, ok, NULL, NULL, &response);
	CFRelease(t);
	CFRelease(m);
	CFRelease(ok);
}

static void open_address(const char *address)
{
	CFStringRef s = cs(address);
	CFURLRef url = s ? CFURLCreateWithString(NULL, s, NULL) : NULL;

	if (url)
	{
		/* the default browser, whichever one this Mac has */
		LSOpenCFURLRef(url, NULL);
		CFRelease(url);
	}
	if (s)
		CFRelease(s);
}

/* ---------- the windows */

static WindowRef make_window(int width, int height, const char *title)
{
	Rect bounds;
	WindowRef w = NULL;
	CFStringRef t = cs(title);

	set_rect(&bounds, 100, 100, 100 + width, 100 + height);
	if (CreateNewWindow(kMovableModalWindowClass,
		kWindowCloseBoxAttribute | kWindowCompositingAttribute | kWindowStandardHandlerAttribute, &bounds, &w) != noErr)
		w = NULL;
	if (w)
		SetWindowTitleWithCFString(w, t);
	CFRelease(t);
	return w;
}

static void present(WindowRef w)
{
	ProcessSerialNumber psn;

	RepositionWindow(w, NULL, kWindowCenterOnMainScreen);
	if (GetCurrentProcess(&psn) == noErr)
		SetFrontProcess(&psn);
	ShowWindow(w);
	SelectWindow(w);
}

static void finish(struct window_state *s)
{
	s->done = 1;
	if (!quiet)
		QuitAppModalLoopForWindow(s->window);
}

static void feedback_finished(struct window_state *s, int status);

static OSStatus about_handler(EventHandlerCallRef call, EventRef event, void *data)
{
	struct window_state *s = (struct window_state *)data;
	UInt32 class = GetEventClass(event), kind = GetEventKind(event);

	(void)call;
	if (class == kEventClassCommand && kind == kEventCommandProcess)
	{
		HICommand command;

		GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(command), NULL, &command);
		if (command.commandID == CMD_CLOSE || command.commandID == kHICommandQuit)
		{
			finish(s);
			return noErr;
		}
		if (command.commandID == CMD_FEEDBACK)
		{
			s->pending_feedback = 1;
			finish(s);
			return noErr;
		}
		if (command.commandID >= CMD_LINK && command.commandID < CMD_LINK + 1 + LB_LINKS)
		{
			int k = (int)(command.commandID - CMD_LINK);

			open_address(k == 0 ? PROJECT_URL : s->env->branding->links[k - 1].url);
			return noErr;
		}
		if (command.commandID == kHICommandAbout)
			return noErr;
	}
	else if (class == kEventClassWindow && kind == kEventWindowClose)
	{
		finish(s);
		return noErr;
	}
	return eventNotHandledErr;
}

void la_show_about(const struct la_env *env)
{
	static const EventTypeSpec specs[] =
	{
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassWindow, kEventWindowClose }
	};
	struct window_state s;
	int height;

	{
		memset(&s, 0, sizeof(s));
		s.env = env;
		height = layout_about(env);
		s.window = make_window(ABOUT_WIDTH, height, "About PowerSpark");
		if (!s.window)
			return;
		make_controls(&s, env->app_path);
		InstallWindowEventHandler(s.window, NewEventHandlerUPP(about_handler), 2, specs, &s, NULL);
		/* (Return and Escape close it) */
		if (s.close_button)
		{
			SetWindowDefaultButton(s.window, s.close_button);
			SetWindowCancelButton(s.window, s.close_button);
		}
		present(s.window);
		RunAppModalLoopForWindow(s.window);
		HideWindow(s.window);
		DisposeWindow(s.window);
		if (s.pending_feedback)
			la_show_feedback(env);
	}
}

/* ---------- the Feedback window */

static void stop_sending(struct window_state *s)
{
	if (s->timer)
	{
		RemoveEventLoopTimer(s->timer);
		s->timer = NULL;
	}
	if (s->pipe_fd >= 0)
	{
		close(s->pipe_fd);
		s->pipe_fd = -1;
	}
}

static void start_sending(struct window_state *s)
{
	const struct lb_branding *b = s->env->branding;
	char version[32], build[32], line[96];
	int fds[2];
	pid_t pid;

	version_line(line, sizeof(line), version, sizeof(version), build, sizeof(build));
	if (pipe(fds) != 0)
		return;
	pid = fork();
	if (pid < 0)
	{
		close(fds[0]);
		close(fds[1]);
		return;
	}
	if (pid == 0)
	{
		int null = open("/dev/null", O_RDONLY);

		close(fds[0]);
		dup2(fds[1], 1);
		dup2(fds[1], 2);
		if (null >= 0)
			dup2(null, 0);
		execl(b->sender_path, b->sender_path, "--url", b->feedback_url, "--ca", b->ca_path, "--token", b->feedback_token,
			"--app-version", version, s->report_path, (char *)NULL);
		_exit(127);
	}
	close(fds[1]);
	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
	s->child = pid;
	s->pipe_fd = fds[0];
	s->answer_length = 0;
	s->answer[0] = 0;
}

/* polls the sender; when it has ended, shows the outcome */
static pascal void poll_sender(EventLoopTimerRef timer, void *data)
{
	struct window_state *s = (struct window_state *)data;
	int status = 0;
	pid_t r;
	char buf[256];
	ssize_t n;

	(void)timer;
	if (s->child <= 0)
		return;
	while (s->pipe_fd >= 0 && (n = read(s->pipe_fd, buf, sizeof(buf))) > 0)
	{
		size_t room = sizeof(s->answer) - 1 - s->answer_length, take = (size_t)n < room ? (size_t)n : room;

		memcpy(s->answer + s->answer_length, buf, take);
		s->answer_length += take;
		s->answer[s->answer_length] = 0;
	}
	r = waitpid(s->child, &status, WNOHANG);
	if (r == 0)
		return;
	s->child = 0;
	stop_sending(s);
	feedback_finished(s, r < 0 ? -1 : (WIFEXITED(status) ? WEXITSTATUS(status) : -1));
}

/* the sender's reason: the text after "kept: " or "refused: " on its line */
static const char *reason_of(const char *answer)
{
	const char *p = strstr(answer, "kept: ");

	if (p)
		return p + 6;
	p = strstr(answer, "refused: ");
	return p ? p + 9 : "";
}

static void feedback_finished(struct window_state *s, int status)
{
	char message[900], reason[512];
	const char *r = reason_of(s->answer);
	size_t n = strcspn(r, "\r\n");

	if (n >= sizeof(reason))
		n = sizeof(reason) - 1;
	memcpy(reason, r, n);
	reason[n] = 0;
	if (status == 0)
	{
		note_alert("Thank you", "Your report has been sent.");
		finish(s);
		return;
	}
	if (status == 2)
	{
		snprintf(message, sizeof(message), "The site refused the report%s%s. It was not kept.", reason[0] ? ": " : "", reason);
		note_alert("That could not be sent", message);
		EnableControl(s->send);
		set_label(s->status, "Not sent.");
		return;
	}
	snprintf(message, sizeof(message), "%s\n\nYour report has been kept, and PowerSpark will send it again the next time "
		"it starts.", reason[0] ? reason : "The report could not be sent.");
	note_alert("That could not be sent", message);
	finish(s);
}

/* validates and sends what the window holds */
static void feedback_send(struct window_state *s)
{
	char *message = text_view_string(s->text), *email = edit_string(s->edit), id[64], dir[1024], *log = NULL;
	char version[32], build[32], line[96], screen[32];
	struct fb_report report;
	struct fb_system system;
	struct fb_buf json;
	const char *p;
	int topic = (int)GetControl32BitValue(s->popup) - 1, letters = 0;

	for (p = message; *p; ++p)
		if (*p != ' ' && *p != '\n' && *p != '\r' && *p != '\t')
			++letters;
	if (letters < 5)
	{
		note_alert("Say a little more", "A sentence about what happened is what makes a report worth sending.");
		free(message);
		free(email);
		return;
	}
	if (topic < 0 || topic >= FB_TOPICS)
		topic = FB_TOPICS - 1;
	if (GetControl32BitValue(s->check_log))
		log = fb_read_log_tail(s->env->logs_dir, s->env->home, FB_LOG_MAX);
	version_line(line, sizeof(line), version, sizeof(version), build, sizeof(build));
	fb_system_info(&system);
	snprintf(screen, sizeof(screen), "%dx%d", (int)CGDisplayPixelsWide(CGMainDisplayID()),
		(int)CGDisplayPixelsHigh(CGMainDisplayID()));
	snprintf(system.screen, sizeof(system.screen), "%s", screen);
	fb_new_id(id, sizeof(id));
	memset(&report, 0, sizeof(report));
	report.id = id;
	report.app = s->env->branding->feedback_app;
	report.version = version;
	report.build = build;
	report.topic = fb_topics[topic];
	report.message = message;
	report.email = email;
	report.log = log;
	report.system = &system;
	memset(&json, 0, sizeof(json));
	fb_report_json(&report, &json);
	if (json.failed || fb_outbox_dir(s->env->home, dir, sizeof(dir), 1) != 0 ||
		fb_outbox_store(dir, id, json.data, json.length) != 0)
		note_alert("That could not be kept", "PowerSpark could not write the report to its Outbox folder, in "
			"~/Library/Application Support/OpenCE.");
	else
	{
		snprintf(s->report_path, sizeof(s->report_path), "%s/%s.json", dir, id);
		DisableControl(s->send);
		set_label(s->status, "Sending\xE2\x80\xA6");
		start_sending(s);
		if (s->child <= 0)
		{
			note_alert("That could not be sent", "The report has been kept, and PowerSpark will send it the next time it "
				"starts.");
			finish(s);
		}
		else
			InstallEventLoopTimer(GetCurrentEventLoop(), 0.1, 0.1, NewEventLoopTimerUPP(poll_sender), s, &s->timer);
	}
	fb_buf_free(&json);
	free(log);
	free(message);
	free(email);
}

static OSStatus feedback_handler(EventHandlerCallRef call, EventRef event, void *data)
{
	struct window_state *s = (struct window_state *)data;
	UInt32 class = GetEventClass(event), kind = GetEventKind(event);

	(void)call;
	if (class == kEventClassCommand && kind == kEventCommandProcess)
	{
		HICommand command;

		GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(command), NULL, &command);
		if (command.commandID == CMD_SEND)
		{
			if (s->child <= 0)
				feedback_send(s);
			return noErr;
		}
		if (command.commandID == CMD_CANCEL || command.commandID == kHICommandQuit)
		{
			if (s->child > 0)
				kill(s->child, SIGTERM);	/* (the report stays in the Outbox) */
			finish(s);
			return noErr;
		}
		if (command.commandID == kHICommandAbout)
			return noErr;
	}
	else if (class == kEventClassWindow && kind == kEventWindowClose)
	{
		if (s->child > 0)
			kill(s->child, SIGTERM);
		finish(s);
		return noErr;
	}
	return eventNotHandledErr;
}

static void feedback_open(struct window_state *s, const struct la_env *env)
{
	int height = layout_feedback(env);

	memset(s, 0, sizeof(*s));
	s->env = env;
	s->pipe_fd = -1;
	s->window = make_window(FEEDBACK_WIDTH, height, "Send Feedback");
	if (!s->window)
		return;
	make_controls(s, env->app_path);
	s->log = fb_read_log_tail(env->logs_dir, env->home, 1);
	if (!s->log && s->check_log)
		DisableControl(s->check_log);
	if (s->send)
		SetWindowDefaultButton(s->window, s->send);
	if (s->cancel)
		SetWindowCancelButton(s->window, s->cancel);
}

void la_show_feedback(const struct la_env *env)
{
	static const EventTypeSpec specs[] =
	{
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassWindow, kEventWindowClose }
	};
	struct window_state s;

	if (!feedback_available(env))
		return;
	feedback_open(&s, env);
	if (!s.window)
		return;
	InstallWindowEventHandler(s.window, NewEventHandlerUPP(feedback_handler), 2, specs, &s, NULL);
	present(s.window);
	if (s.text)
		SetKeyboardFocus(s.window, (ControlRef)s.text, kControlFocusNextPart);
	RunAppModalLoopForWindow(s.window);
	stop_sending(&s);
	HideWindow(s.window);
	DisposeWindow(s.window);
	free(s.log);
}

/* ---------- the check that makes the windows without showing them */

/* a report through the window's own code to a test server (HALO_MAC_FEEDBACK_TEST_URL and _CA, with the support
   folders under HALO_MAC_FEEDBACK_TEST_HOME): the text and the log are put in, Send is pressed, and the event loop
   is run until the sender ends. The report's app is "PowerSpark-test". Returns the failures */
static int selftest_send(const struct la_env *env, struct window_state *s, FILE *out)
{
	struct lb_branding b = *env->branding;
	struct la_env test = *env;
	char logs[1024], dir[1024], path[1100], log_file[1100];
	FILE *f;
	int i, failures = 0, files = 0;
	DIR *d;
	struct dirent *e;

	snprintf(b.feedback_url, sizeof(b.feedback_url), "%s", getenv("HALO_MAC_FEEDBACK_TEST_URL"));
	snprintf(b.ca_path, sizeof(b.ca_path), "%s", getenv("HALO_MAC_FEEDBACK_TEST_CA"));
	snprintf(b.feedback_app, sizeof(b.feedback_app), "PowerSpark-test");
	test.branding = &b;
	test.home = getenv("HALO_MAC_FEEDBACK_TEST_HOME");
	snprintf(logs, sizeof(logs), "%s/logs", test.home);
	test.logs_dir = logs;
	snprintf(path, sizeof(path), "mkdir -p '%s'", logs);
	system(path);
	snprintf(log_file, sizeof(log_file), "%s/OpenCE-20261018-120000.log", logs);
	f = fopen(log_file, "wb");
	if (f)
	{
		fprintf(f, "PowerSpark test log, home %s\nsecond line\n", test.home);
		fclose(f);
	}
	s->env = &test;
	set_text_view_string(s->text, "This is a test report from the self-test (PowerSpark-test). Ignore it.");
	SetControl32BitValue(s->check_log, 1);
	SetControl32BitValue(s->popup, 3);
	quiet = 1;
	s->done = 0;
	feedback_send(s);
	for (i = 0; i < 120 && !s->done; ++i)
		RunCurrentEventLoop(0.25);
	quiet = 0;
	fb_outbox_dir(test.home, dir, sizeof(dir), 0);
	d = opendir(dir);
	while (d && (e = readdir(d)) != NULL)
		if (strstr(e->d_name, ".json"))
			++files;
	if (d)
		closedir(d);
	fprintf(out, "sent through the window: done %d, %d report(s) left in the Outbox\n", s->done, files);
	if (!s->done)
		++failures;
	s->env = env;
	return failures;
}

int la_selftest(const struct la_env *env, FILE *out)
{
	struct window_state s;
	int failures = 0, height;
	char *text, *mail;

	height = layout_about(env);
	memset(&s, 0, sizeof(s));
	s.env = env;
	s.window = make_window(ABOUT_WIDTH, height, "About PowerSpark");
	if (!s.window)
	{
		fprintf(out, "FAIL: no About window\n");
		return 1;
	}
	make_controls(&s, env->app_path);
	fprintf(out, "About window made (%d controls laid out)\n", item_count);
	DisposeWindow(s.window);

	feedback_open(&s, env);
	if (!s.window)
	{
		fprintf(out, "FAIL: no Feedback window\n");
		return failures + 1;
	}
	if (!s.popup || !s.text || !s.edit || !s.check_log || !s.send || !s.cancel)
	{
		fprintf(out, "FAIL: a control is missing (popup %p, text %p, edit %p, log %p, send %p, cancel %p)\n", (void *)s.popup,
			(void *)s.text, (void *)s.edit, (void *)s.check_log, (void *)s.send, (void *)s.cancel);
		++failures;
	}
	else
	{
		CFStringRef e = cs("someone@example.com");

		set_text_view_string(s.text, "It crashed \xE2\x80\x94 caf\xC3\xA9\nsecond line");
		SetControlData(s.edit, kControlEntireControl, kControlEditTextCFStringTag, sizeof(e), &e);
		CFRelease(e);
		text = text_view_string(s.text);
		mail = edit_string(s.edit);
		fprintf(out, "text box reads back: \"%s\"; email: \"%s\"\n", text, mail);
		if (strcmp(text, "It crashed \xE2\x80\x94 caf\xC3\xA9\nsecond line") != 0 && strcmp(text, "It crashed \xE2\x80\x94 caf\xC3\xA9\rsecond line") != 0)
		{
			fprintf(out, "FAIL: the text box did not hold the text\n");
			++failures;
		}
		if (strcmp(mail, "someone@example.com") != 0)
		{
			fprintf(out, "FAIL: the email field did not hold the address\n");
			++failures;
		}
		free(text);
		free(mail);
	}
	if (getenv("HALO_MAC_FEEDBACK_TEST_URL") && getenv("HALO_MAC_FEEDBACK_TEST_CA") && getenv("HALO_MAC_FEEDBACK_TEST_HOME"))
		failures += selftest_send(env, &s, out);
	DisposeWindow(s.window);
	free(s.log);
	return failures;
}
