/*
DUALSENSE.C

A PS5 DualSense's adaptive triggers, lightbar and player lights (dualsense.h). The bytes are dualsense_rules.c's; what
each player wants is the game's (port/linux/game/dualsense_game.c).
*/

#include <string.h>

#include "platform.h"
#include "port_config.h"
#include "dualsense.h"

#ifndef DUALSENSE_NONE
#include <stdlib.h>
#include <SDL3/SDL_atomic.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#ifdef __linux__
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#endif

/* the udev rule that lets the player at the seat write the pad's hidraw node (a DualSense is 054C:0CE6, a DualSense
Edge 054C:0DF2; the HID device's own name, bus:vendor:product.instance, is the same on USB and Bluetooth) */
#define DUALSENSE_UDEV_RULE \
	"KERNEL==\"hidraw*\", KERNELS==\"*:054C:0CE6.*|*:054C:0DF2.*\", MODE=\"0660\", TAG+=\"uaccess\""

#define PORTS 4
#define PADS 8
/* a port's wish this old (the player died, a menu, the game left) lets the triggers go */
#define WISH_STALE_MS 300
/* the triggers sent again though unchanged, while both are up: this long after the game's rumble changed, and at
least this often */
#define REFRESH_AFTER_RUMBLE_MS 40
#define REFRESH_EVERY_MS 1000
/* a trigger counts as up below this (SDL's axis, 0-32767) */
#define TRIGGER_UP 3000
/* the game's motors this old (it stopped setting them) are stopped (SDL's rumble lasts 100 ms a call) */
#define RUMBLE_STALE_MS 150
/* a node that went is looked for again this often */
#define REOPEN_MS 1000
/* a pad is given its fields back for this long at exit and when it leaves, if its queue is full */
#define RESET_WAIT_MS 200

enum backend
{
	BACKEND_UNKNOWN,
	BACKEND_NONE,   /* not a DualSense, or no way to write to it */
	BACKEND_SDL,    /* SDL's own PS5 driver */
	BACKEND_HIDRAW  /* Linux: the kernel's driver reads it, we write its hidraw node */
};

struct pad
{
	SDL_JoystickID id;
	int used;
	int port;
	enum backend backend;
	int fd;
	int bluetooth;
	int sequence;
	char node[32];
	/* what the pad has from us and what is wanted, by field; on Bluetooth one report every 50 ms at most */
	struct dualsense_link link;
	/* the hidraw node went (the pad left: ENODEV, EIO): closed, and opened again when the pad is back */
	int gone;
	Uint64 reopen_at;
	int reopen_logged;
	int failed_logged;
	/* the trigger's kick under way (until kick_until, SDL_GetTicks ms) and the events seen */
	int events_known;
	unsigned long kicks_right;
	Uint64 kick_until;
	unsigned char kick[DUALSENSE_EFFECT_SIZE];
	/* the rumble seen, and when the triggers are next sent again though unchanged */
	unsigned short rumble_left, rumble_right;
	Uint64 refresh_at;
	Uint64 triggers_sent_at;
	/* the fields to be given back (a busy write keeps them for the next frame) */
	int reset_pending;
};

/* shared with the game's thread and XInputSetState's (spinlock); the pads are the event thread's */
static SDL_SpinLock shared_lock;
static struct
{
	SDL_JoystickID bound[PORTS];
	struct dualsense_wish wish[PORTS];
	Uint64 wish_ms[PORTS];
	int wished[PORTS];
	/* the motors on each port (XInputSetState) and when they were set */
	unsigned short rumble_left[PORTS], rumble_right[PORTS];
	Uint64 rumble_ms[PORTS];
	/* whether the port's pad is a DualSense this machine drives (the game works out a wish for it only then) */
	int active[PORTS];
} shared;
static struct pad pads[PADS];
static int exit_registered;
/* the settings as last sent (for the change to off: the triggers let go, the lightbar given back once) */
static struct dualsense_settings applied;
static int applied_known;

/* (read by the game's thread and the event thread: their own lock) */
static SDL_SpinLock settings_lock;

struct dualsense_settings dualsense_settings_get(void)
{
	static struct dualsense_settings settings;
	static unsigned long read_at;
	static int read;
	struct dualsense_settings result, before;

	SDL_LockSpinlock(&settings_lock);
	if (!read || read_at != config_changes())
	{
		char const *strength = config_string("input.trigger_strength");
		char const *lightbar = config_string("input.lightbar");

		before = settings;
		settings.triggers = config_boolean("input.adaptive_triggers");
		settings.percent = dualsense_strength_percent(strength ? strength : "");
		settings.lights = lightbar && !strcmp(lightbar, "player_color");
		if (!read || memcmp(&before, &settings, sizeof(settings)))
			platform_log("dualsense: adaptive triggers %s (strength %d%%), lightbar %s (a DualSense's only)",
				settings.triggers ? "on" : "off", settings.percent, settings.lights ? "player color" : "off");
		read = 1;
		read_at = config_changes();
	}
	result = settings;
	SDL_UnlockSpinlock(&settings_lock);
	return result;
}

int dualsense_on(void)
{
	struct dualsense_settings settings = dualsense_settings_get();

	return settings.triggers || settings.lights;
}

int dualsense_port_active(int port)
{
	int active;

	if (port < 0 || port >= PORTS)
		return 0;
	SDL_LockSpinlock(&shared_lock);
	active = shared.active[port];
	SDL_UnlockSpinlock(&shared_lock);
	return active;
}

void dualsense_wish_set(int port, struct dualsense_wish const *wish)
{
	if (port < 0 || port >= PORTS || !wish)
		return;
	SDL_LockSpinlock(&shared_lock);
	shared.wish[port] = *wish;
	shared.wish_ms[port] = SDL_GetTicks();
	shared.wished[port] = 1;
	SDL_UnlockSpinlock(&shared_lock);
}

void dualsense_port(int port, struct SDL_Gamepad *gamepad, unsigned short left_motor, unsigned short right_motor)
{
	SDL_JoystickID id;

	if (port < 0 || port >= PORTS)
		return;
	id = gamepad ? SDL_GetGamepadID(gamepad) : 0;
	SDL_LockSpinlock(&shared_lock);
	/* (one pad, one port: a pad that moved here is no longer its old port's) */
	dualsense_bind(shared.bound, PORTS, port, id);
	shared.rumble_left[port] = left_motor;
	shared.rumble_right[port] = right_motor;
	shared.rumble_ms[port] = SDL_GetTicks();
	SDL_UnlockSpinlock(&shared_lock);
}

#ifdef __linux__
/* the pad's hidraw node from its input event node (/dev/input/eventN -> /sys/class/input/eventN/device/device is
the HID device, whose hidraw/ folder names the node), and whether it is on Bluetooth (HID_ID's bus 0005); 0 when it
is not a DualSense (054C:0CE6) or DualSense Edge (054C:0DF2) */
static int hidraw_find(char const *event_path, char *node, size_t node_size, int *bluetooth)
{
	char const *name = strrchr(event_path, '/');
	char path[160];
	char line[160];
	FILE *uevent;
	DIR *folder;
	struct dirent *entry;
	unsigned int bus = 0, vendor = 0, product = 0;
	int found = 0;

	name = name ? name + 1 : event_path;
	if (strncmp(name, "event", 5))
		return 0;
	snprintf(path, sizeof(path), "/sys/class/input/%s/device/device/uevent", name);
	uevent = fopen(path, "r");
	if (!uevent)
		return 0;
	while (fgets(line, sizeof(line), uevent))
	{
		if (!strncmp(line, "HID_ID=", 7))
			sscanf(line + 7, "%x:%x:%x", &bus, &vendor, &product);
	}
	fclose(uevent);
	if (vendor != 0x054c || (product != 0x0ce6 && product != 0x0df2))
		return 0;
	*bluetooth = bus == 0x0005;
	snprintf(path, sizeof(path), "/sys/class/input/%s/device/device/hidraw", name);
	folder = opendir(path);
	if (!folder)
		return 0;
	while ((entry = readdir(folder)) != NULL)
	{
		if (!strncmp(entry->d_name, "hidraw", 6))
		{
			snprintf(node, node_size, "/dev/%s", entry->d_name);
			found = 1;
			break;
		}
	}
	closedir(folder);
	return found;
}
#endif

/* how this pad is driven, said once in debug.txt: the way, the bus, write access, the settings */
static void pad_choose_backend(struct pad *pad, SDL_Gamepad *gamepad, struct dualsense_settings const *settings)
{
	char const *name = SDL_GetGamepadName(gamepad);
	SDL_PropertiesID properties = SDL_GetGamepadProperties(gamepad);

	if (!name)
		name = "(unnamed)";
	pad->backend = BACKEND_NONE;
	pad->fd = -1;
	dualsense_link_init(&pad->link, 0);
	/* (other makers' pads that SDL takes for a PS5 pad are left alone) */
	if (!dualsense_is_dualsense(SDL_GetGamepadVendor(gamepad), SDL_GetGamepadProduct(gamepad)))
		return;
	if (properties && SDL_GetBooleanProperty(properties, SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN, false))
	{
		int wireless = SDL_GetGamepadConnectionState(gamepad) == SDL_JOYSTICK_CONNECTION_WIRELESS;

		pad->backend = BACKEND_SDL;
		dualsense_link_init(&pad->link, wireless);
		platform_log("dualsense: %s on port %d: through SDL's own PS5 driver (effects with SDL_SendGamepadEffect%s); "
			"adaptive triggers %s, lightbar %s", name, pad->port + 1,
			wireless ? "; wireless: one report every 50 ms at most" : "", settings->triggers ? "on" : "off",
			settings->lights ? "on" : "off");
		return;
	}
#ifdef __linux__
	{
		char const *path = SDL_GetGamepadPath(gamepad);

		if (!path || !hidraw_find(path, pad->node, sizeof(pad->node), &pad->bluetooth))
		{
			platform_log("dualsense: %s on port %d: no hidraw node found for %s: no adaptive triggers or lightbar",
				name, pad->port + 1, path ? path : "(no path)");
			return;
		}
		pad->fd = open(pad->node, O_WRONLY | O_CLOEXEC | O_NONBLOCK);
		if (pad->fd < 0)
		{
			platform_log("dualsense: %s on port %d: cannot write %s (%s): no adaptive triggers or lightbar. A udev "
				"rule gives the player at the seat the node: put this line in /etc/udev/rules.d/70-dualsense.rules, "
				"run sudo udevadm control --reload, and plug the pad in again: %s", name, pad->port + 1, pad->node,
				strerror(errno), DUALSENSE_UDEV_RULE);
			return;
		}
		pad->backend = BACKEND_HIDRAW;
		dualsense_link_init(&pad->link, pad->bluetooth);
		platform_log("dualsense: %s on port %d: input through the kernel's driver (%s), effects written to %s (%s); "
			"adaptive triggers %s, lightbar %s", name, pad->port + 1, path, pad->node,
			pad->bluetooth ? "Bluetooth" : "USB", settings->triggers ? "on" : "off", settings->lights ? "on" : "off");
	}
#else
	platform_log("dualsense: %s on port %d: SDL's driver does not drive it: no adaptive triggers or lightbar", name,
		pad->port + 1);
#endif
}

/* sends the body (only the enable bits set in it count) */
static enum dualsense_write_result pad_send(struct pad *pad, SDL_Gamepad *gamepad,
	unsigned char const body[DUALSENSE_BODY_SIZE])
{
	if (pad->backend == BACKEND_SDL)
	{
		int ok = 1;

		if (gamepad && (body[DUALSENSE_BODY_ENABLE1] &
			(DUALSENSE_ENABLE1_RIGHT_TRIGGER | DUALSENSE_ENABLE1_LEFT_TRIGGER)))
		{
			unsigned char triggers[DUALSENSE_BODY_SIZE];

			memcpy(triggers, body, sizeof(triggers));
			triggers[DUALSENSE_BODY_ENABLE2] = 0;
			ok = SDL_SendGamepadEffect(gamepad, triggers, (int)sizeof(triggers));
		}
		/* (SDL keeps its own LED state: its calls, not our bytes) */
		if (gamepad && (body[DUALSENSE_BODY_ENABLE2] & DUALSENSE_ENABLE2_LIGHTBAR))
			ok = SDL_SetGamepadLED(gamepad, body[DUALSENSE_BODY_RED], body[DUALSENSE_BODY_GREEN],
				body[DUALSENSE_BODY_BLUE]) && ok;
		if (gamepad && (body[DUALSENSE_BODY_ENABLE2] & DUALSENSE_ENABLE2_PLAYER_LIGHTS))
			ok = SDL_SetGamepadPlayerIndex(gamepad,
				dualsense_player_index_of_lights(body[DUALSENSE_BODY_PLAYER_LIGHTS])) && ok;
		if (!ok && !pad->failed_logged)
		{
			platform_log("dualsense: SDL refused the effect: %s", SDL_GetError());
			pad->failed_logged = 1;
		}
		return ok ? DUALSENSE_WRITE_SENT : DUALSENSE_WRITE_REFUSED;
	}
#ifdef __linux__
	if (pad->backend == BACKEND_HIDRAW)
	{
		unsigned char report[DUALSENSE_BLUETOOTH_REPORT_SIZE];
		size_t size = dualsense_output_report(report, sizeof(report), pad->bluetooth, pad->sequence, body);
		enum dualsense_write_result result;
		ssize_t written;
		int error;

		if (pad->fd < 0)
			return DUALSENSE_WRITE_GONE;
		written = write(pad->fd, report, size);
		error = errno;
		/* (the next sequence number only once this report went: a busy write is sent again with the same one) */
		if (written == (ssize_t)size)
		{
			pad->sequence = (pad->sequence + 1) & 0x0f;
			return DUALSENSE_WRITE_SENT;
		}
		/* (a short write: tried again as the next window opens) */
		result = written < 0 ? dualsense_write_result_of_errno(error) : DUALSENSE_WRITE_BUSY;
		if (result == DUALSENSE_WRITE_GONE)
			platform_log("dualsense: port %d: writing %s failed (%s): the pad went; its node is opened again when it "
				"is back, and everything sent again", pad->port + 1, pad->node, strerror(error));
		else if (result == DUALSENSE_WRITE_REFUSED)
			platform_log("dualsense: port %d: writing %s failed (%s): the pad is left alone", pad->port + 1,
				pad->node, strerror(error));
		return result;
	}
#endif
	(void)gamepad;
	return DUALSENSE_WRITE_REFUSED;
}

static void pad_close(struct pad *pad)
{
#ifdef __linux__
	if (pad->fd >= 0)
		close(pad->fd);
#endif
	memset(pad, 0, sizeof(*pad));
	pad->fd = -1;
}

/* the hidraw node went (ENODEV, EIO): closed; opened again when the pad is back */
static void pad_lose(struct pad *pad, Uint64 now)
{
#ifdef __linux__
	if (pad->fd >= 0)
		close(pad->fd);
#endif
	pad->fd = -1;
	pad->gone = 1;
	pad->reopen_at = now + REOPEN_MS;
	pad->reopen_logged = 0;
}

#ifdef __linux__
/* the pad's node again (its path may have changed: a new hidraw number, or the other bus); everything is sent again */
static void pad_reopen(struct pad *pad, SDL_Gamepad *gamepad, Uint64 now)
{
	char const *path = SDL_GetGamepadPath(gamepad);
	char node[sizeof(pad->node)];
	int bluetooth = 0, fd = -1;

	pad->reopen_at = now + REOPEN_MS;
	if (path && hidraw_find(path, node, sizeof(node), &bluetooth))
		fd = open(node, O_WRONLY | O_CLOEXEC | O_NONBLOCK);
	if (fd < 0)
	{
		if (!pad->reopen_logged)
			platform_log("dualsense: port %d: waiting for the pad's node to come back (tried every %d ms)",
				pad->port + 1, REOPEN_MS);
		pad->reopen_logged = 1;
		return;
	}
	pad->fd = fd;
	memcpy(pad->node, node, sizeof(node));
	pad->bluetooth = bluetooth;
	pad->gone = 0;
	pad->failed_logged = 0;
	pad->sequence = 0;
	pad->link.bluetooth = bluetooth;
	dualsense_link_forget(&pad->link);
	platform_log("dualsense: port %d: %s opened again (%s): everything sent again", pad->port + 1, pad->node,
		bluetooth ? "Bluetooth" : "USB");
}
#endif

/* one write if one is due (immediate: the window ignored); the fields written, 0 for none */
static int pad_write(struct pad *pad, SDL_Gamepad *gamepad, Uint64 now, int immediate,
	unsigned char body[DUALSENSE_BODY_SIZE])
{
	enum dualsense_write_result result;
	int fields;

	if (pad->gone || pad->backend == BACKEND_NONE || pad->backend == BACKEND_UNKNOWN)
		return 0;
	fields = immediate ? dualsense_link_due_now(&pad->link, now, body) : dualsense_link_due(&pad->link, now, body);
	if (!fields)
		return 0;
	result = pad_send(pad, gamepad, body);
	dualsense_link_result(&pad->link, fields, result, now);
	if (result == DUALSENSE_WRITE_SENT)
		return fields;
	if (result == DUALSENSE_WRITE_GONE)
		pad_lose(pad, now);
	else if (result == DUALSENSE_WRITE_REFUSED)
		/* (a pad that refuses is left alone until it comes back) */
		pad->backend = BACKEND_NONE;
	return 0;
}

/* these fields given back, at once (the window ignored), where the pad has them from us: the triggers let go, the
player lights off (SDL: player index -1, its lights off), the lightbar the kernel's own blue (hid-playstation's default;
the hidraw way only: SDL's driver keeps the last colour set). Returns the fields still to give back: those a busy
write kept, to be tried again; 0 when all are sent or the pad is gone */
static int pad_reset(struct pad *pad, SDL_Gamepad *gamepad, int fields, Uint64 now)
{
	unsigned char body[DUALSENSE_BODY_SIZE];
	unsigned char off[DUALSENSE_EFFECT_SIZE];
	int wanted, forced, sent;

	fields &= pad->link.known & DUALSENSE_FIELDS;
	if (pad->backend != BACKEND_HIDRAW)
		fields &= ~DUALSENSE_FIELD_LIGHTBAR;
	if (!fields || pad->gone || (pad->backend != BACKEND_SDL && pad->backend != BACKEND_HIDRAW))
		return 0;
	wanted = pad->link.wanted;
	forced = pad->link.forced;
	if (fields & DUALSENSE_FIELD_TRIGGERS)
	{
		dualsense_effect_off(off);
		dualsense_link_want_triggers(&pad->link, off, off);
	}
	if (fields & DUALSENSE_FIELD_LIGHTBAR)
		dualsense_link_want_lightbar(&pad->link, 0, 0, 128);
	if (fields & DUALSENSE_FIELD_PLAYER_LIGHTS)
		dualsense_link_want_player_lights(&pad->link, 0);
	/* (only these fields, sent though the pad may have them already) */
	pad->link.wanted = fields;
	pad->link.forced = fields;
	sent = pad_write(pad, gamepad, now, 1, body);
	pad->link.wanted = wanted & ~fields;
	pad->link.forced = forced & ~fields;
	if (!sent)
		/* (busy: kept; gone or refused: nothing more to give back) */
		return pad->gone || pad->backend == BACKEND_NONE ? 0 : fields;
	/* (driven again later: sent again then) */
	pad->link.known &= ~sent;
	return fields & ~sent;
}

/* as pad_reset, tried again for up to `ms` (at exit and when a pad leaves, when no later frame will do it) */
static void pad_reset_wait(struct pad *pad, SDL_Gamepad *gamepad, int fields, Uint64 ms)
{
	Uint64 end = SDL_GetTicks() + ms;
	Uint64 now = SDL_GetTicks();

	for (fields = pad_reset(pad, gamepad, fields, now); fields && now < end; now = SDL_GetTicks())
	{
		SDL_Delay(4);
		fields = pad_reset(pad, gamepad, fields, SDL_GetTicks());
	}
}

static void dualsense_exit(void)
{
	Uint64 now = SDL_GetTicks();
	int i;

	for (i = 0; i < PADS; i++)
	{
		if (pads[i].used)
		{
			pad_reset_wait(&pads[i], SDL_GetGamepadFromID(pads[i].id), DUALSENSE_FIELDS, RESET_WAIT_MS);
			pad_close(&pads[i]);
		}
	}
}

static struct pad *pad_for(SDL_JoystickID id, int port)
{
	int i, free_slot = -1;

	for (i = 0; i < PADS; i++)
	{
		if (pads[i].used && pads[i].id == id)
		{
			pads[i].port = port;
			return &pads[i];
		}
		if (!pads[i].used && free_slot < 0)
			free_slot = i;
	}
	if (free_slot < 0)
		return NULL;
	memset(&pads[free_slot], 0, sizeof(pads[free_slot]));
	pads[free_slot].used = 1;
	pads[free_slot].id = id;
	pads[free_slot].port = port;
	pads[free_slot].fd = -1;
	pads[free_slot].backend = BACKEND_UNKNOWN;
	return &pads[free_slot];
}

/* both triggers up (SDL's axes) */
static int triggers_up(SDL_Gamepad *gamepad)
{
	return SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) < TRIGGER_UP &&
		SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) < TRIGGER_UP;
}

/* one pad on its port this frame: what is wanted, then at most one report */
static void pad_frame(struct pad *pad, SDL_Gamepad *gamepad, struct dualsense_wish const *wish, int fresh,
	unsigned short rumble_left, unsigned short rumble_right, struct dualsense_settings const *settings, Uint64 now)
{
	unsigned char body[DUALSENSE_BODY_SIZE];
	unsigned char right[DUALSENSE_EFFECT_SIZE], left[DUALSENSE_EFFECT_SIZE];
	int written;

	/* (driven again: nothing more to give back of what is wanted) */
	if (settings->triggers)
		pad->reset_pending &= ~DUALSENSE_FIELD_TRIGGERS;
	if (settings->lights)
		pad->reset_pending &= ~(DUALSENSE_FIELD_LIGHTBAR | DUALSENSE_FIELD_PLAYER_LIGHTS);

	/* the rumble changed: it is the kernel's or SDL's own report, so the trigger effects are sent again once both
	triggers are up, in case the pad dropped them */
	if (rumble_left != pad->rumble_left || rumble_right != pad->rumble_right)
	{
		pad->rumble_left = rumble_left;
		pad->rumble_right = rumble_right;
		pad->refresh_at = now + REFRESH_AFTER_RUMBLE_MS;
	}
	if (settings->triggers)
	{
		int unchanged, kicking = 0;

		if (fresh)
		{
			/* an event: its kick now, the held shape again after its time */
			if (pad->events_known && wish->kicks_right != pad->kicks_right && wish->kick_right_ms > 0)
			{
				pad->kick_until = now + (Uint64)wish->kick_right_ms;
				memcpy(pad->kick, wish->kick_right, sizeof(pad->kick));
			}
			pad->kicks_right = wish->kicks_right;
			pad->events_known = 1;
			kicking = now < pad->kick_until;
			memcpy(right, kicking ? pad->kick : wish->right, sizeof(right));
			memcpy(left, wish->left, sizeof(left));
		}
		else
		{
			pad->kick_until = 0;
			pad->events_known = 0;
			dualsense_effect_off(right);
			dualsense_effect_off(left);
		}
		dualsense_link_want_triggers(&pad->link, right, left);
		unchanged = (pad->link.known & DUALSENSE_FIELD_TRIGGERS) &&
			!(dualsense_link_dirty(&pad->link) & DUALSENSE_FIELD_TRIGGERS);
		/* (a refresh waiting for the Bluetooth window is let go if a trigger is pulled meanwhile) */
		if ((pad->link.forced & DUALSENSE_FIELD_TRIGGERS) && !triggers_up(gamepad))
			pad->link.forced &= ~DUALSENSE_FIELD_TRIGGERS;
		if (unchanged && fresh && !kicking && !(pad->link.forced & DUALSENSE_FIELD_TRIGGERS) &&
			((pad->refresh_at && now >= pad->refresh_at) || now - pad->triggers_sent_at >= REFRESH_EVERY_MS) &&
			triggers_up(gamepad))
		{
			dualsense_link_force(&pad->link, DUALSENSE_FIELD_TRIGGERS);
			pad->refresh_at = 0;
		}
	}
	else
		dualsense_link_drop(&pad->link, DUALSENSE_FIELD_TRIGGERS);

	if (settings->lights)
	{
		if (fresh)
		{
			dualsense_link_want_lightbar(&pad->link, wish->red, wish->green, wish->blue);
			if (wish->player_lights)
				dualsense_link_want_player_lights(&pad->link, wish->player_lights);
		}
	}
	else
		dualsense_link_drop(&pad->link, DUALSENSE_FIELD_LIGHTBAR | DUALSENSE_FIELD_PLAYER_LIGHTS);

	written = pad_write(pad, gamepad, now, 0, body);
	if ((written & DUALSENSE_FIELD_TRIGGERS) != 0)
		pad->triggers_sent_at = now;
}

void dualsense_frame(void)
{
	struct dualsense_settings settings = dualsense_settings_get();
	SDL_JoystickID bound[PORTS];
	struct dualsense_wish wish[PORTS];
	int fresh[PORTS];
	unsigned short rumble_left[PORTS], rumble_right[PORTS];
	Uint64 now = SDL_GetTicks();
	int port, i;

	/* a setting turned off: the pads' triggers let go and the lightbar and player lights given back */
	if (applied_known)
	{
		int fields = 0;

		if (applied.triggers && !settings.triggers)
			fields |= DUALSENSE_FIELD_TRIGGERS;
		if (applied.lights && !settings.lights)
			fields |= DUALSENSE_FIELD_LIGHTBAR | DUALSENSE_FIELD_PLAYER_LIGHTS;
		for (i = 0; i < PADS && fields; i++)
		{
			if (pads[i].used)
				pads[i].reset_pending |= fields;
		}
	}
	applied = settings;
	applied_known = 1;
	/* (what a busy write kept, until it is sent) */
	for (i = 0; i < PADS; i++)
	{
		if (pads[i].used && pads[i].reset_pending)
			pads[i].reset_pending = pad_reset(&pads[i], SDL_GetGamepadFromID(pads[i].id), pads[i].reset_pending, now);
	}
	if (!settings.triggers && !settings.lights)
	{
		SDL_LockSpinlock(&shared_lock);
		memset(shared.active, 0, sizeof(shared.active));
		SDL_UnlockSpinlock(&shared_lock);
		return;
	}
	if (!exit_registered)
	{
		atexit(dualsense_exit);
		exit_registered = 1;
	}
	SDL_LockSpinlock(&shared_lock);
	for (port = 0; port < PORTS; port++)
	{
		int rumble_fresh = now - shared.rumble_ms[port] < RUMBLE_STALE_MS;

		bound[port] = shared.bound[port];
		wish[port] = shared.wish[port];
		fresh[port] = shared.wished[port] && now - shared.wish_ms[port] < WISH_STALE_MS;
		/* (the game no longer setting the motors: stopped, as SDL's rumble would after its 100 ms) */
		rumble_left[port] = rumble_fresh ? shared.rumble_left[port] : 0;
		rumble_right[port] = rumble_fresh ? shared.rumble_right[port] : 0;
	}
	SDL_UnlockSpinlock(&shared_lock);

	/* a pad no longer bound to a port lets its triggers go */
	for (i = 0; i < PADS; i++)
	{
		int still = 0;

		if (!pads[i].used)
			continue;
		for (port = 0; port < PORTS; port++)
			still |= bound[port] == pads[i].id && pads[i].port == port;
		if (!still && (pads[i].link.known & DUALSENSE_FIELD_TRIGGERS))
			pads[i].reset_pending |= DUALSENSE_FIELD_TRIGGERS;
	}

	for (port = 0; port < PORTS; port++)
	{
		SDL_Gamepad *gamepad = bound[port] ? SDL_GetGamepadFromID(bound[port]) : NULL;
		struct pad *pad = gamepad ? pad_for(bound[port], port) : NULL;
		int active;

		if (pad && pad->backend == BACKEND_UNKNOWN)
			pad_choose_backend(pad, gamepad, &settings);
#ifdef __linux__
		if (pad && pad->backend == BACKEND_HIDRAW && pad->gone && now >= pad->reopen_at)
			pad_reopen(pad, gamepad, now);
#endif
		active = pad && pad->backend != BACKEND_NONE;
		SDL_LockSpinlock(&shared_lock);
		shared.active[port] = active;
		SDL_UnlockSpinlock(&shared_lock);
		if (active && !pad->gone)
			pad_frame(pad, gamepad, &wish[port], fresh[port], rumble_left[port], rumble_right[port], &settings, now);
	}
}

void dualsense_removed(struct SDL_Gamepad *gamepad)
{
	Uint64 now = SDL_GetTicks();
	SDL_JoystickID id;
	int i;

	if (!gamepad)
		return;
	id = SDL_GetGamepadID(gamepad);
	for (i = 0; i < PADS; i++)
	{
		if (pads[i].used && pads[i].id == id)
		{
			/* (the pad may still be there: SDL closing it for another reason; its hidraw node gone, it is not) */
#ifdef __linux__
			if (pads[i].backend != BACKEND_HIDRAW || (!pads[i].gone && access(pads[i].node, F_OK) == 0))
#endif
				pad_reset_wait(&pads[i], gamepad, DUALSENSE_FIELDS, RESET_WAIT_MS);
			if (pads[i].backend != BACKEND_NONE)
				platform_log("dualsense: the pad on port %d left", pads[i].port + 1);
			pad_close(&pads[i]);
		}
	}
	SDL_LockSpinlock(&shared_lock);
	for (i = 0; i < PORTS; i++)
	{
		if (shared.bound[i] == id)
		{
			shared.bound[i] = 0;
			shared.active[i] = 0;
		}
	}
	SDL_UnlockSpinlock(&shared_lock);
}

#endif
