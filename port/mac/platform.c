/*
PLATFORM.C (the Mac's)

The one function of platform.h that port/mac does not already define.
platform_log is in player_services.c; platform_data_root is here because
the Linux layer answers it from xbox_files.c, which the Mac build does not
compile — it has its own answer, in engine_files.c, reached by a different
name.

Internet play is what wants it: p2p.c reads and writes join_link.txt in
the data root, which is how a launcher hands a halo:// invite to the game
that is already running.
*/

#include <limits.h>
#include <string.h>
#if defined(__APPLE__) && !defined(__LP64__)
#include <Carbon/Carbon.h>
#endif

#include "platform.h"
#include "engine_files.h"

const char *platform_data_root(void)
{
	/* (the root does not change while the game runs: the launcher sets
	HALO_MAC_DATA_ROOT before it starts, and nothing moves it after) */
	static char root[PATH_MAX];
	static int looked;

	if (!looked)
	{
		size_t length;

		looked = 1;
		if (!mac_engine_files_data_root(root, sizeof(root)))
			root[0] = '\0';
		/* trailing slashes off, as the Linux layer's does, so that the
		"%s/join_link.txt" its callers write does not come out doubled */
		length = strlen(root);
		while (length > 1 && root[length - 1] == '/')
			root[--length] = '\0';
	}
	return root;
}

/* The question port_mods.c asks at startup, in Carbon's standard alert: the
same call engine_messages.c shows its messages with, which Mac OS X 10.4 to
10.6 have on both PowerPC and Intel. The alert has three buttons at most (the
default, a cancel and an other). The label at default_index is the default;
of the rest, the first is the other button and the last the cancel button
(Escape), so Escape leaves "Quit" and not the choice that changes the
mods. -1 where it cannot ask: no Carbon (a 64-bit build), no window server,
or an alert that fails; the caller plays on as a run nobody watches does.
Called on the main thread before the game's window opens. */
int platform_ask(const char *title, const char *text, const char *const *labels, int count, int default_index)
{
#if defined(__APPLE__) && !defined(__LP64__)
	CFStringRef title_string = NULL;
	CFStringRef text_string = NULL;
	CFStringRef button[3] = { NULL, NULL, NULL };
	int slot_label[3] = { -1, -1, -1 }; /* the label of the default, cancel and other buttons */
	AlertStdCFStringAlertParamRec parameters;
	DialogRef alert = NULL;
	DialogItemIndex item = 0;
	OSStatus status;
	ProcessSerialNumber process;
	int index;
	int answer = -1;

	if (!title || !text || !labels || count < 1 || count > 3)
		return -1;
	if (default_index < 0 || default_index >= count)
		default_index = 0;
	slot_label[0] = default_index;
	for (index = 0; index < count; index++)
	{
		if (index == default_index)
			continue;
		if (slot_label[1] < 0)
			slot_label[1] = index;
		else
		{
			/* (the later of two is the cancel button, the earlier the other) */
			slot_label[2] = slot_label[1];
			slot_label[1] = index;
		}
	}
	title_string = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
	text_string = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
	status = (title_string && text_string) ? noErr : paramErr;
	for (index = 0; index < 3 && status == noErr; index++)
	{
		if (slot_label[index] < 0)
			continue;
		button[index] = CFStringCreateWithCString(NULL, labels[slot_label[index]], kCFStringEncodingUTF8);
		if (!button[index])
			status = paramErr;
	}
	if (status == noErr)
	{
		status = GetStandardAlertDefaultParams(&parameters, kStdCFStringAlertVersionOne);
		if (status == noErr)
		{
			parameters.defaultText = button[0];
			parameters.cancelText = button[1];
			parameters.otherText = button[2];
			parameters.defaultButton = kAlertStdAlertOKButton;
			parameters.cancelButton = button[1] ? kAlertStdAlertCancelButton : 0;
			/* (a game started from a command line or a tool has no
			menu bar until it is brought to the front) */
			if (GetCurrentProcess(&process) == noErr)
				SetFrontProcess(&process);
			status = CreateStandardAlert(kAlertCautionAlert, title_string, text_string, &parameters, &alert);
		}
		if (status == noErr && alert)
			status = RunStandardAlert(alert, NULL, &item);
		if (status == noErr)
		{
			if (item == kAlertStdAlertOKButton)
				answer = slot_label[0];
			else if (item == kAlertStdAlertCancelButton)
				answer = slot_label[1];
			else if (item == kAlertStdAlertOtherButton)
				answer = slot_label[2];
		}
	}
	for (index = 0; index < 3; index++)
		if (button[index])
			CFRelease(button[index]);
	if (title_string)
		CFRelease(title_string);
	if (text_string)
		CFRelease(text_string);
	return answer;
#else
	(void)title; (void)text; (void)labels; (void)count; (void)default_index;
	return -1;
#endif
}
