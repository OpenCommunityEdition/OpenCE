/* See launcher_update.h. The windows are Control Manager controls made in code (no nib), as launcher_about.c's:
   each window's layout is first worked out as a list of items with their rectangles, from the text as this system
   sets it (a paragraph is measured at its width, so that Tiger's wider Lucida Grande does not cut the last
   line); the same list makes the controls and prints --update-dump's report.

   The work is a few steps that each do a little and are called again from a timer (tick), so that the window
   stays alive while a release downloads, is hashed and is unpacked; with HALO_MAC_UPDATE_HEADLESS they are
   called in a loop instead, with no window. */
#include "launcher_update.h"
#include "launcher_config.h"
#include "update_core.h"
#include <Carbon/Carbon.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BUNDLE_ID "org.opence.PowerSpark"
#define OFFER_WIDTH 480
#define PROGRESS_WIDTH 440
#define MARGIN 24
#define TEXT_LEFT 104
#define MAX_ITEMS 24
#define CHECK_INTERVAL (24L * 3600L)
#define FEED_SECONDS_AT_LAUNCH 8
#define FEED_SECONDS_ASKED 20
#define HASH_STEP 262144

enum
{
	CMD_INSTALL = 'UINS', CMD_LATER = 'ULAT', CMD_NEVER = 'UNVR', CMD_CANCEL = 'UCAN'
};

enum { KIND_ICON = 'I', KIND_PARA = 'P', KIND_LABEL = 'l', KIND_BUTTON = 'b', KIND_NOTES = 'N', KIND_BAR = 'B' };

struct item
{
	char kind;
	char text[512];
	short left, top, right, bottom;
	int font, reserve;
	unsigned long command;
	int width, height;	/* the text's size as measured (a paragraph: wrapped to the rectangle's width) */
};

static struct item items[MAX_ITEMS];
static int item_count;

static int headless(void)
{
	const char *v = getenv("HALO_MAC_UPDATE_HEADLESS");

	return v && *v && strcmp(v, "0") != 0;
}

/* ---------- the log */

static const struct lu_env *log_env;

static void say(const char *format, const char *a, long b)
{
	char line[800], path[PATH_MAX];
	FILE *f;
	struct stat st;

	snprintf(line, sizeof(line), format, a ? a : "", b);
	fprintf(stderr, "PowerSpark updates: %s\n", line);
	if (!log_env || !log_env->logs_dir)
		return;
	snprintf(path, sizeof(path), "%s/update.log", log_env->logs_dir);
	if (stat(path, &st) == 0 && st.st_size > 65536)
		unlink(path);
	f = fopen(path, "ab");
	if (f)
	{
		time_t now = time(NULL);
		char when[32];

		strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", localtime(&now));
		fprintf(f, "%s  %s\n", when, line);
		fclose(f);
	}
}

/* ---------- measuring (as launcher_about.c) */

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

/* a paragraph at the width, as tall as this system sets it; returns the y below it */
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

/* a line that changes while the window is open: room for the widest text it will show (measured by the
   caller), one line tall */
static struct item *add_label(const char *text, int font, int left, int top, int width, int widest)
{
	struct item *it = add_item(KIND_LABEL, text, left, top, left + width, top + 17, font, 0, 0);

	if (it)
		it->width = widest;
	return it;
}

static struct item *add_button(const char *text, int left, int top, int width, unsigned long command)
{
	struct item *it = add_item(KIND_BUTTON, text, left, top, left + width, top + 20, kThemeSystemFont, 28, command);

	if (it)
		it->width = text_width(text, kThemeSystemFont);
	return it;
}

static int button_width(const char *text)
{
	int w = text_width(text, kThemeSystemFont) + 40;

	return w < 84 ? 84 : w;
}

/* ---------- the texts */

static void size_text(long bytes, char *out, size_t size)
{
	if (bytes >= 1024L * 1024L)
		snprintf(out, size, "%.1f MB", (double)bytes / 1048576.0);
	else
		snprintf(out, size, "%ld KB", (bytes + 1023) / 1024);
}

/* the strings the progress window shows, the widest of which its labels are measured against */
static const char *const stage_texts[] = {
	"Looking for a new version\xE2\x80\xA6", "Downloading\xE2\x80\xA6", "Checking the download\xE2\x80\xA6",
	"Unpacking\xE2\x80\xA6", "Putting the new version in place\xE2\x80\xA6", "Cancelling\xE2\x80\xA6"
};
#define STAGE_COUNT 6

static void detail_for_download(long got, long total, char *out, size_t size)
{
	char a[32], b[32];

	size_text(got, a, sizeof(a));
	size_text(total, b, sizeof(b));
	snprintf(out, size, "%s of %s", a, b);
}

/* ---------- layouts */

static int widest_stage(void)
{
	int i, w = 0;

	for (i = 0; i < STAGE_COUNT; ++i)
		if (text_width(stage_texts[i], kThemeSystemFont) > w)
			w = text_width(stage_texts[i], kThemeSystemFont);
	return w;
}

/* the offer: returns the window's height */
static int layout_offer(const struct uc_item *offer, const struct lu_env *env, int at_launch)
{
	int y, column = OFFER_WIDTH - TEXT_LEFT - MARGIN, wide = OFFER_WIDTH - 2 * MARGIN, x;
	char title[160], line[300], size[32], what[96];
	int bw_install = button_width("Install"), bw_later = button_width("Later"), bw_never = button_width("Never Ask Again");

	item_count = 0;
	add_item(KIND_ICON, "", MARGIN, 20, MARGIN + 64, 84, 0, 0, 0);
	snprintf(title, sizeof(title), "PowerSpark %s is available", offer->version);
	y = add_paragraph(title, kThemeAlertHeaderFont, TEXT_LEFT, 22, column);
	size_text(offer->length, size, sizeof(size));
	snprintf(line, sizeof(line), "You have version %s (build %ld). The download is %s.", env->version, env->build, size);
	y = add_paragraph(line, kThemeSmallSystemFont, TEXT_LEFT, y + 4, column);
	if (y < 90)
		y = 90;
	y += 8;
	if (offer->notes[0])
	{
		snprintf(what, sizeof(what), "What\xE2\x80\x99s new in %s:", offer->version);
		y = add_paragraph(what, kThemeSystemFont, MARGIN, y, wide);
		y += 4;
		add_item(KIND_NOTES, offer->notes, MARGIN, y, OFFER_WIDTH - MARGIN, y + 140, 0, 0, 0);
		y += 140 + 10;
	}
	y = add_paragraph("PowerSpark will download the update, check that it is the one that was announced, replace "
		"itself and open again. Your settings and saved games are not touched.", kThemeSmallSystemFont, MARGIN, y, wide);
	y += 12;
	x = OFFER_WIDTH - MARGIN - bw_install;
	add_button("Install", x, y, bw_install, CMD_INSTALL);
	x -= 10 + bw_later;
	add_button("Later", x, y, bw_later, CMD_LATER);
	if (at_launch)
		add_button("Never Ask Again", MARGIN, y, bw_never, CMD_NEVER);
	return y + 20 + 20;
}

/* the progress window (checking, or updating): returns its height */
static int layout_progress(const char *title_text)
{
	int y, column = PROGRESS_WIDTH - TEXT_LEFT - MARGIN, bw = button_width("Cancel");

	item_count = 0;
	add_item(KIND_ICON, "", MARGIN, 20, MARGIN + 64, 84, 0, 0, 0);
	y = add_paragraph(title_text, kThemeAlertHeaderFont, TEXT_LEFT, 22, column);
	y += 8;
	add_label(stage_texts[0], kThemeSystemFont, TEXT_LEFT, y, column, widest_stage());
	y += 22;
	add_item(KIND_BAR, "", TEXT_LEFT, y, TEXT_LEFT + column, y + 16, 0, 0, 0);
	y += 16 + 6;
	{
		char widest[64];

		detail_for_download(999L * 1048576L, 999L * 1048576L, widest, sizeof(widest));
		add_label("", kThemeSmallSystemFont, TEXT_LEFT, y, column, text_width(widest, kThemeSmallSystemFont));
	}
	y += 22;
	if (y < 92)
		y = 92;
	y += 8;
	add_button("Cancel", PROGRESS_WIDTH - MARGIN - bw, y, bw, CMD_CANCEL);
	return y + 20 + 20;
}

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
			size = it->width;
			room = it->right - it->left - it->reserve;
			break;
		case KIND_PARA:
			size = it->height;
			room = it->bottom - it->top;
			break;
		}
		if (size > room && (it->kind == KIND_LABEL || it->kind == KIND_BUTTON || it->kind == KIND_PARA))
		{
			flag = "DOES NOT FIT";
			++bad;
		}
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

int lu_layout_report(const struct lu_env *env, FILE *out)
{
	struct uc_item sample;
	int bad = 0, height, i;
	static const char *const titles[] = { "Checking for updates", "Updating PowerSpark to 10.20.30" };

	memset(&sample, 0, sizeof(sample));
	snprintf(sample.version, sizeof(sample.version), "10.20.30 beta 4");
	sample.length = 987L * 1048576L;
	snprintf(sample.notes, sizeof(sample.notes), "Fixes and improvements.\n\nA second paragraph of notes that is long "
		"enough to need several lines when it is set in the system font.");
	fprintf(out, "Update offer, at launch (%d wide; this app %s build %ld)\n", OFFER_WIDTH, env->version, env->build);
	height = layout_offer(&sample, env, 1);
	bad += report_items(out, OFFER_WIDTH, height);
	fprintf(out, "Update offer, when asked, without notes\n");
	sample.notes[0] = 0;
	height = layout_offer(&sample, env, 0);
	bad += report_items(out, OFFER_WIDTH, height);
	for (i = 0; i < 2; ++i)
	{
		fprintf(out, "Progress window \"%s\" (%d wide)\n", titles[i], PROGRESS_WIDTH);
		height = layout_progress(titles[i]);
		bad += report_items(out, PROGRESS_WIDTH, height);
	}
	return bad;
}

/* ---------- controls */

static ControlRef make_label_control(WindowRef w, const struct item *it)
{
	Rect r;
	ControlRef c = NULL;
	ControlFontStyleRec style;
	CFStringRef s = cs(it->text);

	memset(&style, 0, sizeof(style));
	style.flags = kControlUseThemeFontIDMask | kControlUseJustMask;
	style.font = (SInt16)it->font;
	style.just = teFlushLeft;
	set_rect(&r, it->left, it->top, it->right, it->bottom);
	CreateStaticTextControl(w, &r, s, &style, &c);
	CFRelease(s);
	return c;
}

static void set_label(ControlRef c, const char *text)
{
	CFStringRef s = cs(text);

	if (c)
	{
		SetControlData(c, kControlEntireControl, kControlStaticTextCFStringTag, sizeof(s), &s);
		HIViewSetNeedsDisplay((HIViewRef)c, true);
	}
	CFRelease(s);
}

static HIViewRef content_view(WindowRef w)
{
	HIViewRef content = NULL;

	if (HIViewFindByID(HIViewGetRoot(w), kHIViewWindowContentID, &content) != noErr)
		return NULL;
	return content;
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
		TXNSetSelection(txn, kTXNStartOffset, kTXNStartOffset);
	}
	free(chars);
	CFRelease(s);
}

struct ui
{
	WindowRef window;
	ControlRef stage, detail, bar, cancel, default_button;
	int choice, done;
	EventLoopTimerRef timer;
	void (*tick)(void *);
	void *tick_data;
};

static void make_controls(struct ui *u, const char *app_path)
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
			make_icon_control(u->window, it, app_path);
			break;
		case KIND_PARA:
			make_label_control(u->window, it);
			break;
		case KIND_LABEL:
			c = make_label_control(u->window, it);
			if (!u->stage)
				u->stage = c;
			else
				u->detail = c;
			break;
		case KIND_BAR:
			set_rect(&r, it->left, it->top, it->right, it->bottom);
			CreateProgressBarControl(u->window, &r, 0, 0, 100, true, &u->bar);
			break;
		case KIND_BUTTON:
			set_rect(&r, it->left, it->top, it->right, it->bottom);
			{
				CFStringRef s = cs(it->text);

				CreatePushButtonControl(u->window, &r, s, &c);
				CFRelease(s);
			}
			if (c)
			{
				SetControlCommandID(c, (UInt32)it->command);
				if (it->command == CMD_INSTALL)
					u->default_button = c;
				if (it->command == CMD_CANCEL)
					u->cancel = c;
				if (it->command == CMD_LATER && !u->cancel)
					u->cancel = c;
			}
			break;
		case KIND_NOTES:
		{
			/* a bezel around a scrolling text view that cannot be edited */
			HIViewRef content = content_view(u->window), view = NULL;
			HIRect frame;
			ControlRef box = NULL;

			set_rect(&r, it->left, it->top, it->right, it->bottom);
			CreateGroupBoxControl(u->window, &r, NULL, false, &box);
			frame = CGRectMake(it->left + 3, it->top + 3, it->right - it->left - 6, it->bottom - it->top - 6);
			if (content && HITextViewCreate(&frame, 0, kTXNWantVScrollBarMask | kTXNAlwaysWrapAtViewEdgeMask |
				kTXNMonostyledTextMask | kTXNReadOnlyMask, &view) == noErr && view)
			{
				HIViewAddSubview(content, view);
				HIViewSetVisible(view, true);
				set_text_view_string(view, it->text);
			}
			break;
		}
		}
	}
}

static WindowRef make_window(int width, int height, const char *title)
{
	Rect bounds;
	WindowRef w = NULL;
	CFStringRef t = cs(title);

	set_rect(&bounds, 100, 100, 100 + width, 100 + height);
	if (CreateNewWindow(kMovableModalWindowClass,
		kWindowCompositingAttribute | kWindowStandardHandlerAttribute, &bounds, &w) != noErr)
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

static OSStatus ui_handler(EventHandlerCallRef call, EventRef event, void *data)
{
	struct ui *u = (struct ui *)data;
	UInt32 class = GetEventClass(event), kind = GetEventKind(event);

	(void)call;
	if (class == kEventClassCommand && kind == kEventCommandProcess)
	{
		HICommand command;

		GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(command), NULL, &command);
		if (command.commandID == CMD_INSTALL || command.commandID == CMD_LATER || command.commandID == CMD_NEVER ||
			command.commandID == CMD_CANCEL)
		{
			u->choice = (int)command.commandID;
			u->done = 1;
			QuitAppModalLoopForWindow(u->window);
			return noErr;
		}
		if (command.commandID == kHICommandQuit)
		{
			u->choice = (int)CMD_CANCEL;
			u->done = 1;
			QuitAppModalLoopForWindow(u->window);
			return noErr;
		}
	}
	else if (class == kEventClassWindow && kind == kEventWindowClose)
	{
		u->choice = (int)CMD_CANCEL;
		u->done = 1;
		QuitAppModalLoopForWindow(u->window);
		return noErr;
	}
	return eventNotHandledErr;
}

static pascal void ui_timer(EventLoopTimerRef timer, void *data)
{
	struct ui *u = (struct ui *)data;

	(void)timer;
	if (u->tick)
		u->tick(u->tick_data);
}

/* shows the window and runs it until a button is pressed (or the tick function ends it through
   QuitAppModalLoopForWindow); u->choice is the button's command, 0 if the tick function ended it */
static int run_window(struct ui *u, int width, int height, const char *title, const char *app_path)
{
	static const EventTypeSpec specs[] =
	{
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassWindow, kEventWindowClose }
	};

	u->window = make_window(width, height, title);
	if (!u->window)
		return -1;
	make_controls(u, app_path);
	InstallWindowEventHandler(u->window, NewEventHandlerUPP(ui_handler), 2, specs, u, NULL);
	if (u->default_button)
		SetWindowDefaultButton(u->window, u->default_button);
	if (u->cancel)
		SetWindowCancelButton(u->window, u->cancel);
	if (u->bar)
	{
		Boolean animate = true;

		SetControlData(u->bar, kControlEntireControl, kControlProgressBarAnimatingTag, sizeof(animate), &animate);
	}
	if (u->tick)
		InstallEventLoopTimer(GetCurrentEventLoop(), 0.05, 0.05, NewEventLoopTimerUPP(ui_timer), u, &u->timer);
	present(u->window);
	RunAppModalLoopForWindow(u->window);
	if (u->timer)
		RemoveEventLoopTimer(u->timer);
	HideWindow(u->window);
	DisposeWindow(u->window);
	u->window = NULL;
	return 0;
}

static void bar_set(ControlRef bar, long value, long total)
{
	Boolean indeterminate = total <= 0;

	if (!bar)
		return;
	SetControlData(bar, kControlEntireControl, kControlProgressBarIndeterminateTag, sizeof(indeterminate), &indeterminate);
	if (!indeterminate)
	{
		SetControl32BitMaximum(bar, 1000);
		SetControl32BitValue(bar, (SInt32)(value >= total ? 1000 : (double)value / (double)total * 1000.0));
	}
}

/* ---------- running the downloader, and other programs */

struct job
{
	pid_t pid;
	int fd;
	char buf[1024];
	size_t n;
	long received, total;
	int done, code;
	char error[400];
};

static int job_start(struct job *j, char *const argv[], int pipe_output)
{
	int fds[2] = { -1, -1 };

	memset(j, 0, sizeof(*j));
	j->fd = -1;
	if (pipe_output && pipe(fds) != 0)
		return -1;
	j->pid = fork();
	if (j->pid < 0)
	{
		if (pipe_output)
		{
			close(fds[0]);
			close(fds[1]);
		}
		return -1;
	}
	if (j->pid == 0)
	{
		int null = open("/dev/null", O_RDWR);

		if (pipe_output)
		{
			close(fds[0]);
			dup2(fds[1], 1);
			close(fds[1]);
		}
		else if (null >= 0)
			dup2(null, 1);
		if (null >= 0)
		{
			dup2(null, 0);
			dup2(null, 2);
		}
		execv(argv[0], argv);
		_exit(127);
	}
	if (pipe_output)
	{
		close(fds[1]);
		fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
		j->fd = fds[0];
	}
	return 0;
}

static void job_line(struct job *j, char *line)
{
	if (!strncmp(line, "progress ", 9))
		sscanf(line + 9, "%ld %ld", &j->received, &j->total);
	else if (!strncmp(line, "error ", 6))
		snprintf(j->error, sizeof(j->error), "%s", line + 6);
}

static void job_read(struct job *j)
{
	ssize_t n;

	while (j->fd >= 0 && (n = read(j->fd, j->buf + j->n, sizeof(j->buf) - 1 - j->n)) > 0)
	{
		char *nl;

		j->n += (size_t)n;
		j->buf[j->n] = 0;
		while ((nl = strchr(j->buf, '\n')) != NULL)
		{
			size_t rest = j->n - (size_t)(nl + 1 - j->buf);

			*nl = 0;
			job_line(j, j->buf);
			memmove(j->buf, nl + 1, rest);
			j->n = rest;
			j->buf[j->n] = 0;
		}
		if (j->n >= sizeof(j->buf) - 1)
			j->n = 0;	/* (a line longer than any the downloader writes: dropped) */
	}
}

/* 1 once the program has ended */
static int job_poll(struct job *j)
{
	int status = 0;
	pid_t r;

	if (j->done)
		return 1;
	job_read(j);
	r = waitpid(j->pid, &status, WNOHANG);
	if (r == 0)
		return 0;
	job_read(j);
	j->done = 1;
	j->code = r < 0 ? -1 : (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
	if (j->fd >= 0)
	{
		close(j->fd);
		j->fd = -1;
	}
	return 1;
}

static void job_stop(struct job *j)
{
	int status;

	if (j->pid > 0 && !j->done)
	{
		kill(j->pid, SIGTERM);
		waitpid(j->pid, &status, 0);
		j->done = 1;
	}
	if (j->fd >= 0)
	{
		close(j->fd);
		j->fd = -1;
	}
}

static void fetch_argv(const struct lu_env *env, const char *url, const char *out, long max, int seconds, char **argv,
	char *max_text, char *seconds_text)
{
	const struct lb_branding *b = env->branding;

	snprintf(max_text, 24, "%ld", max);
	snprintf(seconds_text, 24, "%d", seconds);
	argv[0] = (char *)b->updater_path;
	argv[1] = "--url";
	argv[2] = (char *)url;
	argv[3] = "--ca";
	argv[4] = (char *)b->ca_path;
	argv[5] = "--out";
	argv[6] = (char *)out;
	argv[7] = "--max";
	argv[8] = max_text;
	argv[9] = "--timeout";
	argv[10] = seconds_text;
	argv[11] = "--user-agent";
	argv[12] = "PowerSpark-update";
	argv[13] = NULL;
}

/* the downloader's reason, in words for the player */
static void fetch_failure(const struct job *j, char *out, size_t size)
{
	if (j->error[0])
		snprintf(out, size, "%s", j->error);
	else
		snprintf(out, size, "the downloader stopped (status %d)", j->code);
}

/* ---------- config.toml's update.auto */

static char *slurp(const char *path, size_t *length)
{
	FILE *f = fopen(path, "rb");
	char *text;
	size_t n;

	if (!f)
		return NULL;
	text = (char *)malloc(262145);
	if (!text)
	{
		fclose(f);
		return NULL;
	}
	n = fread(text, 1, 262144, f);
	fclose(f);
	text[n] = 0;
	*length = n;
	return text;
}

static int auto_setting(const struct lu_env *env)
{
	const char *over = getenv("HALO_UPDATE_AUTO");
	char *text, value[32];
	size_t length;
	int on = 1;

	if (over && *over)
		return !(!strcmp(over, "0") || !strcasecmp(over, "false") || !strcasecmp(over, "no") || !strcasecmp(over, "off"));
	text = slurp(env->config_file, &length);
	if (text)
	{
		if (lc_text_get(text, "update.auto", value, sizeof(value)))
			on = lc_bool(value);
		free(text);
	}
	return on;
}

/* "Never Ask Again": update.auto = false, as the game's own "Do not ask again" writes */
static void never_ask_again(const struct lu_env *env)
{
	size_t length = 0;
	char *text = slurp(env->config_file, &length), temp[PATH_MAX];
	FILE *f;

	if (!text)
	{
		text = (char *)malloc(1);
		if (!text)
			return;
		text[0] = 0;
		length = 0;
	}
	if (lc_text_set(&text, &length, "update.auto", "false") != 0)
	{
		free(text);
		return;
	}
	snprintf(temp, sizeof(temp), "%s.tmp", env->config_file);
	f = fopen(temp, "wb");
	if (f && fwrite(text, 1, length, f) == length && fclose(f) == 0)
		rename(temp, env->config_file);
	else if (f)
		unlink(temp);
	free(text);
	say("will not ask again (update.auto = false in config.toml)", NULL, 0);
}

/* ---------- the pieces of a run */

int lu_ready(const struct lu_env *env)
{
	return env->branding && lb_update_ready(env->branding);
}

static char stamp_path[PATH_MAX];

static const char *stamp_file(const struct lu_env *env)
{
	snprintf(stamp_path, sizeof(stamp_path), "%s/update-check", env->support);
	return stamp_path;
}

int lu_auto_wanted(const struct lu_env *env, long now)
{
	return lu_ready(env) && auto_setting(env) && uc_check_due(stamp_file(env), now, CHECK_INTERVAL);
}

void lu_system_version(int *major, int *minor)
{
	SInt32 a = 0, b = 0;

	*major = 10;
	*minor = 4;
	if (Gestalt(gestaltSystemVersionMajor, &a) == noErr && Gestalt(gestaltSystemVersionMinor, &b) == noErr && a > 0)
	{
		*major = (int)a;
		*minor = (int)b;
	}
	else if (Gestalt(gestaltSystemVersion, &a) == noErr)
		*minor = (int)((a >> 4) & 0xF);
}

static void wait_for(pid_t pid, int seconds)
{
	int i;

	if (pid <= 0)
		return;
	for (i = 0; i < seconds * 10 && kill(pid, 0) == 0; ++i)
		usleep(100000);
}

/* what the check found */
struct found
{
	int result;		/* UC_AVAILABLE, UC_UP_TO_DATE, UC_ERROR, or -2: the player cancelled */
	struct uc_item item;
	char error[500];
};

static void temp_name(char *out, size_t size, const char *what)
{
	const char *tmp = getenv("TMPDIR");

	snprintf(out, size, "%s%sPowerSpark-%s-%ld", tmp && *tmp ? tmp : "/tmp", tmp && *tmp && tmp[strlen(tmp) - 1] == '/' ? "" : "/",
		what, (long)getpid());
}

static void judge(const struct lu_env *env, const char *file, struct found *f)
{
	size_t length = 0;
	char *data = slurp(file, &length);
	struct uc_current cur;

	if (!data || length == 0)
	{
		free(data);
		f->result = UC_ERROR;
		snprintf(f->error, sizeof(f->error), "The update feed was empty.");
		return;
	}
	cur.version = env->version;
	cur.build = env->build;
	lu_system_version(&cur.os_major, &cur.os_minor);
	cur.public_key = env->branding->update_key;
	f->result = uc_choose(data, length, &cur, &f->item, f->error, sizeof(f->error));
	free(data);
}

/* the feed, with no window: for the launch, where only a newer release is worth a word */
static void look_quietly(const struct lu_env *env, struct found *f)
{
	struct job job;
	char file[PATH_MAX], *argv[16], max_text[24], seconds_text[24];
	time_t deadline = time(NULL) + FEED_SECONDS_AT_LAUNCH + 12;

	temp_name(file, sizeof(file), "appcast");
	fetch_argv(env, env->branding->update_feed, file, UC_MAX_APPCAST, FEED_SECONDS_AT_LAUNCH, argv, max_text, seconds_text);
	if (job_start(&job, argv, 1) != 0)
	{
		f->result = UC_ERROR;
		snprintf(f->error, sizeof(f->error), "the downloader could not be started");
		return;
	}
	while (!job_poll(&job) && time(NULL) < deadline)
		usleep(20000);
	if (!job.done)
	{
		job_stop(&job);
		f->result = UC_ERROR;
		snprintf(f->error, sizeof(f->error), "the update feed did not answer in time");
	}
	else if (job.code != 0)
	{
		f->result = UC_ERROR;
		fetch_failure(&job, f->error, sizeof(f->error));
	}
	else
		judge(env, file, f);
	unlink(file);
}

struct looking
{
	const struct lu_env *env;
	struct ui *ui;
	struct job job;
	struct found *found;
	char file[PATH_MAX];
	int finished;
};

static void looking_tick(void *data)
{
	struct looking *l = (struct looking *)data;

	if (l->finished || !job_poll(&l->job))
		return;
	l->finished = 1;
	if (l->job.code != 0)
	{
		l->found->result = UC_ERROR;
		fetch_failure(&l->job, l->found->error, sizeof(l->found->error));
	}
	else
		judge(l->env, l->file, l->found);
	unlink(l->file);
	if (l->ui && l->ui->window)
		QuitAppModalLoopForWindow(l->ui->window);
}

/* the feed, with a small window that can be cancelled: for when the player asked */
static void look_asked(const struct lu_env *env, struct found *f)
{
	struct looking l;
	struct ui ui;
	char *argv[16], max_text[24], seconds_text[24];
	int height;

	memset(&l, 0, sizeof(l));
	memset(&ui, 0, sizeof(ui));
	l.env = env;
	l.ui = &ui;
	l.found = f;
	temp_name(l.file, sizeof(l.file), "appcast");
	fetch_argv(env, env->branding->update_feed, l.file, UC_MAX_APPCAST, FEED_SECONDS_ASKED, argv, max_text, seconds_text);
	if (job_start(&l.job, argv, 1) != 0)
	{
		f->result = UC_ERROR;
		snprintf(f->error, sizeof(f->error), "the downloader could not be started");
		return;
	}
	if (headless())
	{
		while (!l.finished)
		{
			looking_tick(&l);
			usleep(20000);
		}
		return;
	}
	ui.tick = looking_tick;
	ui.tick_data = &l;
	height = layout_progress("Checking for updates");
	if (run_window(&ui, PROGRESS_WIDTH, height, "PowerSpark", env->app_path) != 0)
	{
		/* (no window: wait for the answer unseen) */
		while (!l.finished)
		{
			looking_tick(&l);
			usleep(20000);
		}
		return;
	}
	if (!l.finished)
	{
		/* (cancelled) */
		job_stop(&l.job);
		unlink(l.file);
		f->result = -2;
	}
}

/* the offer: CMD_INSTALL, CMD_LATER, CMD_NEVER or CMD_CANCEL */
static int offer(const struct lu_env *env, const struct uc_item *item, int mode)
{
	const char *answer = getenv("HALO_UPDATE_ANSWER");
	struct ui ui;
	int height;

	if (answer && *answer)
	{
		say("answer given (HALO_UPDATE_ANSWER=%s)", answer, 0);
		return !strcmp(answer, "yes") ? (int)CMD_INSTALL : !strcmp(answer, "never") ? (int)CMD_NEVER : (int)CMD_LATER;
	}
	if (headless())
		return (int)CMD_LATER;
	memset(&ui, 0, sizeof(ui));
	height = layout_offer(item, env, mode == LU_AT_LAUNCH);
	if (run_window(&ui, OFFER_WIDTH, height, "PowerSpark Update", env->app_path) != 0)
	{
		/* (no window: the plain alert) */
		return (int)CMD_LATER;
	}
	return ui.choice ? ui.choice : (int)CMD_LATER;
}

/* ---------- the install */

enum { ST_DOWNLOAD, ST_HASH, ST_UNPACK, ST_PLACE, ST_DONE };

struct install
{
	const struct lu_env *env;
	struct uc_item item;
	struct ui *ui;
	char parent[PATH_MAX], work[PATH_MAX], zip[PATH_MAX], unpacked[PATH_MAX], new_app[PATH_MAX], old[PATH_MAX];
	int stage, cancelled, ok;
	struct job job;
	FILE *hash_file;
	struct uc_sha256 sha;
	long hashed;
	char error[700];
};

static void install_end(struct install *in)
{
	in->stage = ST_DONE;
	if (in->ui && in->ui->window)
		QuitAppModalLoopForWindow(in->ui->window);
}

static void install_cancel(struct install *in)
{
	in->cancelled = 1;
	job_stop(&in->job);
	if (in->hash_file)
		fclose(in->hash_file);
	in->hash_file = NULL;
	in->stage = ST_DONE;
	say("update cancelled", NULL, 0);
}

static void install_fail(struct install *in, const char *why)
{
	snprintf(in->error, sizeof(in->error), "%s", why);
	say("update failed: %s", why, 0);
	job_stop(&in->job);
	if (in->hash_file)
	{
		fclose(in->hash_file);
		in->hash_file = NULL;
	}
	install_end(in);
}

static void show_stage(struct install *in, int stage_text, const char *detail, long value, long total)
{
	if (!in->ui)
		return;
	set_label(in->ui->stage, stage_texts[stage_text]);
	set_label(in->ui->detail, detail);
	bar_set(in->ui->bar, value, total);
}

static void install_tick(void *data)
{
	struct install *in = (struct install *)data;
	char detail[96];

	if (in->stage == ST_DONE)
		return;
	switch (in->stage)
	{
	case ST_DOWNLOAD:
		if (!job_poll(&in->job))
		{
			long total = in->job.total > 0 ? in->job.total : in->item.length;

			detail_for_download(in->job.received, total, detail, sizeof(detail));
			show_stage(in, 1, detail, in->job.received, total);
			return;
		}
		if (in->job.code != 0)
		{
			char why[500];

			fetch_failure(&in->job, why, sizeof(why));
			snprintf(in->error, sizeof(in->error), "The download failed: %s.", why);
			say("update failed: %s", in->error, 0);
			install_end(in);
			return;
		}
		in->hash_file = fopen(in->zip, "rb");
		if (!in->hash_file)
		{
			install_fail(in, "The downloaded file could not be read.");
			return;
		}
		uc_sha256_start(&in->sha);
		in->hashed = 0;
		in->stage = ST_HASH;
		return;
	case ST_HASH:
	{
		unsigned char buffer[HASH_STEP / 4];
		int pieces;

		for (pieces = 0; pieces < 4; ++pieces)
		{
			size_t got = fread(buffer, 1, sizeof(buffer), in->hash_file);

			if (got == 0)
				break;
			uc_sha256_add(&in->sha, buffer, got);
			in->hashed += (long)got;
		}
		if (ferror(in->hash_file))
		{
			install_fail(in, "The downloaded file could not be read.");
			return;
		}
		detail_for_download(in->hashed, in->item.length, detail, sizeof(detail));
		show_stage(in, 2, detail, in->hashed, in->item.length);
		if (pieces == 4 && !feof(in->hash_file))
			return;
		/* (the end of the file) */
		{
			char hex[65];

			fclose(in->hash_file);
			in->hash_file = NULL;
			uc_sha256_finish(&in->sha, hex);
			if (in->hashed != in->item.length)
			{
				install_fail(in, "The download is not the size the update feed announced, so it was thrown away.");
				return;
			}
			if (strcmp(hex, in->item.sha256) != 0)
			{
				install_fail(in, "The download does not match its checksum (SHA-256), so it was thrown away.");
				return;
			}
			say("download checked: SHA-256 and size match, signature was checked with the feed", NULL, 0);
		}
		if (mkdir(in->unpacked, 0700) != 0)
		{
			install_fail(in, "A folder for the update could not be made.");
			return;
		}
		{
			char *argv[] = { "/usr/bin/ditto", "-x", "-k", in->zip, in->unpacked, NULL };

			if (job_start(&in->job, argv, 0) != 0)
			{
				install_fail(in, "The update could not be unpacked (ditto did not start).");
				return;
			}
		}
		show_stage(in, 3, "", 0, 0);
		in->stage = ST_UNPACK;
		return;
	}
	case ST_UNPACK:
	{
		char why[400];

		if (!job_poll(&in->job))
			return;
		if (in->job.code != 0)
		{
			install_fail(in, "The update could not be unpacked.");
			return;
		}
		unlink(in->zip);
		if (uc_find_app(in->unpacked, in->new_app, sizeof(in->new_app), why, sizeof(why)) != 0 ||
			uc_validate_bundle(in->new_app, BUNDLE_ID, in->item.version, in->item.build, uc_this_cpu(), why, sizeof(why)) != 0)
		{
			char text[500];

			snprintf(text, sizeof(text), "%s", why[0] ? why : "The update is not a PowerSpark for this Mac.");
			install_fail(in, text);
			return;
		}
		in->stage = ST_PLACE;
		show_stage(in, 4, "", 0, 0);
		return;
	}
	case ST_PLACE:
	{
		char why[500];

		/* (the new app is next to the old one, on the same volume: each step is a rename) */
		if (uc_swap(in->new_app, in->env->app_path, in->old, sizeof(in->old), why, sizeof(why)) != 0)
		{
			install_fail(in, why);
			return;
		}
		in->ok = 1;
		say("PowerSpark %s is in place; the old one is kept as PowerSpark.app.old until the new one has opened",
			in->item.version, 0);
		install_end(in);
		return;
	}
	}
}

/* waits for this process to end, opens the new app, and then removes the old one */
static void schedule_relaunch(const char *app, const char *old)
{
	const char *opener = getenv("HALO_MAC_UPDATE_OPENER");	/* (the tests' stand-in for open) */

	static const char script[] =
		"while kill -0 \"$1\" 2>/dev/null; do sleep 1; done; \"$4\" \"$2\"; sleep \"$5\"; "
		"if [ -n \"$3\" ]; then /bin/rm -rf \"$3\"; fi";
	char pid[16];
	pid_t child;

	snprintf(pid, sizeof(pid), "%ld", (long)getpid());
	child = fork();
	if (child == 0)
	{
		int null = open("/dev/null", O_RDWR), i;

		setsid();
		if (null >= 0)
		{
			dup2(null, 0);
			dup2(null, 1);
			dup2(null, 2);
		}
		for (i = 3; i < 64; ++i)
			close(i);
		execl("/bin/sh", "sh", "-c", script, "sh", pid, app, old, opener && *opener ? opener : "/usr/bin/open",
			opener && *opener ? "2" : "8", (char *)NULL);
		_exit(127);
	}
}

/* 1: installed and the new one will open; 0: not (error: the message for the player, or empty: cancelled) */
static int install(const struct lu_env *env, const struct uc_item *item, char *error, size_t error_size)
{
	struct install in;
	struct ui ui;
	char *argv[16], max_text[24], seconds_text[24], why[600], title[160];
	char *slash;
	int ok, height;

	memset(&in, 0, sizeof(in));
	memset(&ui, 0, sizeof(ui));
	in.env = env;
	in.item = *item;
	error[0] = 0;
	if (uc_place_check(env->app_path, why, sizeof(why)) != UC_PLACE_OK)
	{
		snprintf(error, error_size, "%s", why);
		say("cannot update here: %s", why, 0);
		return 0;
	}
	snprintf(in.parent, sizeof(in.parent), "%s", env->app_path);
	slash = strrchr(in.parent, '/');
	if (slash)
		*slash = 0;
	snprintf(in.work, sizeof(in.work), "%s/%s%ld", in.parent, UC_WORK_PREFIX, (long)getpid());
	snprintf(in.zip, sizeof(in.zip), "%s/update.zip", in.work);
	snprintf(in.unpacked, sizeof(in.unpacked), "%s/unpacked", in.work);
	uc_remove_tree(in.work);
	if (mkdir(in.work, 0700) != 0)
	{
		snprintf(error, error_size, "A folder for the update could not be made next to PowerSpark (%s).", strerror(errno));
		return 0;
	}
	fetch_argv(env, item->url, in.zip, item->length + 4096, 30, argv, max_text, seconds_text);
	if (job_start(&in.job, argv, 1) != 0)
	{
		uc_remove_tree(in.work);
		snprintf(error, error_size, "The downloader could not be started.");
		return 0;
	}
	say("downloading PowerSpark %s", item->version, 0);
	if (headless())
	{
		while (in.stage != ST_DONE)
		{
			install_tick(&in);
			usleep(10000);
		}
	}
	else
	{
		snprintf(title, sizeof(title), "Updating PowerSpark to %s", item->version);
		in.ui = &ui;
		ui.tick = install_tick;
		ui.tick_data = &in;
		height = layout_progress(title);
		if (run_window(&ui, PROGRESS_WIDTH, height, "PowerSpark", env->app_path) != 0)
		{
			in.ui = NULL;
			while (in.stage != ST_DONE)
			{
				install_tick(&in);
				usleep(10000);
			}
		}
		else
		{
			in.ui = NULL;
			if (in.stage != ST_DONE)
				install_cancel(&in);	/* (Cancel, or the window was closed, before the work ended) */
		}
	}
	ok = in.ok;
	if (!ok)
	{
		uc_remove_tree(in.work);
		if (in.error[0])
			snprintf(error, error_size, "%s", in.error);
		return 0;
	}
	/* (the unpacked folder is empty now; the old app waits for the new one to open) */
	uc_remove_tree(in.work);
	schedule_relaunch(env->app_path, in.old);
	return 1;
}

/* ---------- a run */

int lu_run(const struct lu_env *env, int mode)
{
	struct found *found = (struct found *)calloc(1, sizeof(*found));
	char error[800];
	int choice, replaced = 0;

	log_env = env;
	if (!found)
		return 0;
	if (!lu_ready(env))
	{
		if (mode == LU_ASKED && env->alert)
			env->alert("This PowerSpark has no update feed",
				"This copy of PowerSpark was built without a place to look for updates, so it cannot check. "
				"Builds from the OpenCE source do not update themselves; get a new version from wherever you got this one.");
		free(found);
		return 0;
	}
	if (mode == LU_ASKED)
		wait_for(env->wait_pid, 15);
	uc_check_stamp(stamp_file(env), (long)time(NULL));
	say("looking for updates (%s)", mode == LU_ASKED ? "asked" : "at launch", 0);
	if (mode == LU_ASKED)
		look_asked(env, found);
	else
		look_quietly(env, found);
	if (found->result == -2)
	{
		say("cancelled", NULL, 0);
		free(found);
		return 0;
	}
	if (found->result == UC_ERROR)
	{
		say("could not look: %s", found->error, 0);
		if (mode == LU_ASKED && env->alert)
		{
			snprintf(error, sizeof(error), "%s\n\nPowerSpark could not reach the update feed, or could not trust what it "
				"found. Check your internet connection and the date and time of this Mac (System Preferences), and try "
				"again later.", found->error);
			env->alert("PowerSpark could not look for updates", error);
		}
		free(found);
		return 0;
	}
	if (found->result == UC_UP_TO_DATE)
	{
		say("up to date (version %s, build %ld)", env->version, env->build);
		if (mode == LU_ASKED && env->alert)
		{
			snprintf(error, sizeof(error), "You have version %s (build %ld), the newest there is.", env->version, env->build);
			env->alert("PowerSpark is up to date", error);
		}
		free(found);
		return 0;
	}
	say("version %s is available", found->item.version, 0);
	choice = offer(env, &found->item, mode);
	if (choice == (int)CMD_NEVER)
		never_ask_again(env);
	else if (choice == (int)CMD_INSTALL)
	{
		replaced = install(env, &found->item, error, sizeof(error));
		if (!replaced && error[0] && env->alert)
		{
			char message[1000];

			snprintf(message, sizeof(message), "%s\n\nYour PowerSpark was not changed.", error);
			env->alert("PowerSpark could not be updated", message);
		}
	}
	free(found);
	return replaced;
}
