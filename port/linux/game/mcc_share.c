/* MCC package sharing in a network game: the game side of mcc_share_session.c.
 * See mcc_share.h and port/linux/NETCODE.md, "MCC package sharing". */
#include "cseries.h"
#include "cseries/errors.h"
#include "bungie_net/common/message_header.h"
#include "game/game.h"
#include "interface/interface.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "rasterizer/rasterizer.h"
#include "network_distributed.h"
#include "mcc_cache.h"
#include "mcc_identity.h"
#include "mcc_maps.h"
#include "mcc_package.h"
#include "mcc_share.h"
#include "mcc_share_session.h"
#include "mcc_workshop.h"
#include <xtl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* port_config.c's, network_game_globals.c's, network_server_message_handler.c's,
cseries_windows.c's */
int config_boolean(const char *name);
long config_integer(const char *name);
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
boolean network_connection_last_read_was_unreliable(void);
unsigned long system_milliseconds(void);

enum
{
	/* the list is a small text file */
	MCC_SHARE_LIST_BYTES = 0x10000,
	/* text centred at the top of the screen, as the Custom Edition download's is */
	_mcc_share_text_centered = 2,
};

/* the one share: the host's side serves, the joiner's fetches. Nothing is created
until the first frame of a game (mcc_share_game_update). */
static struct mcc_share mcc_share_globals;
static boolean mcc_share_ready;
/* the host's game record as the joiner was given it (the join that began the download) */
static char mcc_share_name[128];
static unsigned char mcc_share_digest[32];
static char mcc_share_text[256];

/* ---------- the engine's files and folders */

static int mcc_share_io_make(char const *path) { return CreateDirectoryA(path, NULL) != 0; }
static int mcc_share_io_remove(char const *path) { return RemoveDirectoryA(path) != 0; }
static int mcc_share_io_move(char const *from, char const *to) { return MoveFileA(from, to) != 0; }
static int mcc_share_io_delete(char const *path) { return DeleteFileA(path) != 0; }
static int mcc_share_io_exists(char const *path)
{
	unsigned long attributes = GetFileAttributesA(path);

	return attributes != (unsigned long)-1 && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

/* ---------- messages */

static void mcc_share_fill_header(
	struct distributed_message_header *header,
	byte type,
	word size)
{
	header->header = 0;
	build_message_header(&header->header, size, 2, 0);
	header->type = type;
	header->count = 1;
	header->game_time = game_in_progress() ? game_time_get() : 0;
}

static void mcc_share_send_request(void *user, struct mcc_share_request const *request)
{
	struct
	{
		struct distributed_message_header header;
		struct mcc_share_request request;
	} message;

	(void)user;
	csmemset(&message, 0, sizeof(message));
	mcc_share_fill_header(&message.header, _distributed_message_mcc_share_request, (word)sizeof(message));
	message.request = *request;
	network_distributed_client_send_reliably(&message, (word)sizeof(message));
}

static void mcc_share_send_response(void *user, long machine, void const *payload, unsigned bytes)
{
	static struct
	{
		struct distributed_message_header header;
		byte payload[MCC_SHARE_RESPONSE_MAXIMUM];
	} message;
	word size = (word)(sizeof(message.header) + bytes);

	(void)user;
	if (bytes > sizeof(message.payload))
		return;
	csmemset(&message.header, 0, sizeof(message.header));
	mcc_share_fill_header(&message.header, _distributed_message_mcc_share_response, size);
	csmemcpy(message.payload, payload, bytes);
	network_distributed_server_send_to_machine_reliably(machine, &message, size);
}

boolean mcc_share_game_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	csmemcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_mcc_share_request || header.type == _distributed_message_mcc_share_response;
}

void mcc_share_game_handle_message(
	long machine_index,
	word const *message,
	word size,
	boolean from_stream)
{
	struct distributed_message_header header;
	byte const *body = (byte const *)message + sizeof(header);

	/* (over the sender's connection alone: a datagram can be sent as from anyone) */
	if (!mcc_share_game_handles_message(message, size) || !from_stream || !mcc_share_ready)
		return;
	csmemcpy(&header, message, sizeof(header));
	if (header.type == _distributed_message_mcc_share_request)
	{
		/* (the host takes requests from the machines of its game) */
		if (machine_index != NONE && global_network_game_server_get())
			mcc_share_host_request(&mcc_share_globals, machine_index, body, size - sizeof(header));
	}
	else if (machine_index == NONE && !global_network_game_server_get())
	{
		mcc_share_client_response(&mcc_share_globals, body, size - sizeof(header));
	}
}

/* ---------- the host's admission */

static enum mcc_share_sidecar mcc_share_sidecar_state(
	char const *folder)
{
	struct mcc_workshop_item item;
	char path[160];
	char text[128];
	FILE *file;
	size_t length;

	snprintf(path, sizeof(path), "mcc_maps/%s/" MCC_WORKSHOP_SIDECAR, folder);
	errno = 0;
	file = fopen(path, "rb");
	if (!file)
		return errno == ENOENT ? MCC_SHARE_SIDECAR_ABSENT : MCC_SHARE_SIDECAR_UNREADABLE;
	length = fread(text, 1, sizeof(text) - 1, file);
	fclose(file);
	text[length] = 0;
	return mcc_workshop_parse_sidecar(text, &item) ? MCC_SHARE_SIDECAR_VALID : MCC_SHARE_SIDECAR_UNREADABLE;
}

/* a file's size, or 0 if it is not there (or not readable) */
static unsigned long mcc_share_file_size(
	char const *path)
{
	FILE *file = fopen(path, "rb");
	long size = 0;

	if (!file)
		return 0;
	if (!fseek(file, 0, SEEK_END))
		size = ftell(file);
	fclose(file);
	return size > 0 ? (unsigned long)size : 0;
}

static void mcc_share_add_file(
	struct mcc_share_host_package *package,
	byte kind,
	char const *folder,
	char const *leaf)
{
	int index = package->count;
	unsigned long size;

	if (index >= MCC_SHARE_STAGE_FILES)
		return;
	package->files[index].kind = kind;
	snprintf(package->files[index].leaf, sizeof(package->files[index].leaf), "%s", leaf);
	snprintf(package->files[index].path, sizeof(package->files[index].path), "mcc_maps/%s/%s", folder, leaf);
	/* (a file the package does not have is simply not offered) */
	size = mcc_share_file_size(package->files[index].path);
	if (!size || (kind == MCC_SHARE_FILE_CREDIT && size > MCC_SHARE_MAXIMUM_CREDIT_BYTES))
		return;
	package->count++;
}

/* The admission: the host's opt-in, the maintained rules, the host's list. The facts the
rules want are established here, locally, from this host's own files; nothing a joiner
sent is used except the level name it asks about, which must be the game's package. */
static void mcc_share_host_package_files(
	void *user,
	char const *level,
	struct mcc_share_host_package *package)
{
	static char list[MCC_SHARE_LIST_BYTES];
	static char const credit_leaves[] = "ModInfo.json\0credits.txt\0readme.txt\0license.txt\0";
	char canonical[64];
	char folder[64];
	char level_leaf[MCC_SHARE_LEAF_SIZE];
	char map_leaf[MCC_SHARE_LEAF_SIZE + 8];
	unsigned char digest[32];
	char const *credit;
	unsigned long list_length = 0;
	FILE *file;

	(void)user;
	csmemset(package, 0, sizeof(*package));
	package->refusal = MCC_SHARE_RESULT_NOT_SHAREABLE;
	/* (only the package this host's game is on) */
	if (!mcc_package_name(level, canonical) || !mcc_identity_host_package(canonical, digest) ||
		!mcc_workshop_package_folder(canonical, folder) || strlen(strrchr(canonical, '\\') + 1) >= sizeof(level_leaf) - 4)
	{
		return;
	}
	strcpy(level_leaf, strrchr(canonical, '\\') + 1);
	file = fopen(MCC_SHARE_LIST_FILE, "rb");
	if (file)
	{
		list_length = (unsigned long)fread(list, 1, sizeof(list), file);
		fclose(file);
	}
	package->refusal = mcc_share_admit(canonical, config_boolean("network.mcc_map_share"),
		mcc_share_sidecar_state(folder), digest, TRUE, list, list_length, package->credit);
	if (package->refusal != MCC_SHARE_RESULT_OK)
	{
		char fingerprint[65];

		/* (the fingerprint to list in mcc_share.txt, when this host means to share the package) */
		mcc_share_list_hex(digest, fingerprint);
		error(_error_silent, "mcc share: not offering %s (%s); its fingerprint is %s", canonical,
			package->refusal == MCC_SHARE_RESULT_OFF ? "network.mcc_map_share is off" :
			package->refusal == MCC_SHARE_RESULT_NO_CREDIT ? "listed in mcc_share.txt without a maker" :
			package->refusal == MCC_SHARE_RESULT_WORKSHOP ? "a Workshop item" : "not listed in mcc_share.txt or not shareable",
			fingerprint);
		return;
	}
	snprintf(map_leaf, sizeof(map_leaf), "%s.map", level_leaf);
	mcc_share_add_file(package, MCC_SHARE_FILE_MAP, folder, map_leaf);
	mcc_share_add_file(package, MCC_SHARE_FILE_BITMAPS, folder, "bitmaps.map");
	mcc_share_add_file(package, MCC_SHARE_FILE_SOUNDS, folder, "sounds.map");
	for (credit = credit_leaves; *credit; credit += strlen(credit) + 1)
		mcc_share_add_file(package, MCC_SHARE_FILE_CREDIT, folder, credit);
	if (package->count < 1 || package->files[0].kind != MCC_SHARE_FILE_MAP)
		package->refusal = MCC_SHARE_RESULT_UNAVAILABLE;
}

static uint32_t mcc_share_upload_limit(
	void *user)
{
	long kilobytes = config_integer("network.map_share_upload");

	(void)user;
	return (uint32_t)PIN(kilobytes, 16, 20480) * 1024;
}

static uint64_t mcc_share_free_bytes(
	void *user)
{
	ULARGE_INTEGER available;
	ULARGE_INTEGER total;
	ULARGE_INTEGER free_total;

	(void)user;
	if (!GetDiskFreeSpaceExA("d:\\", &available, &total, &free_total))
		return 0;
	return available.QuadPart;
}

static uint32_t mcc_share_now(
	void *user)
{
	(void)user;
	return (uint32_t)system_milliseconds();
}

/* ---------- the joiner */

/* whether an installed map has this package's folder (any level of it): a new package never
goes beside or over it */
static int mcc_share_package_taken(
	void *user,
	char const *level)
{
	char folder[64];
	char other[64];
	short count = mcc_maps_count(FALSE);
	short index;

	(void)user;
	if (!mcc_workshop_package_folder(level, folder))
		return TRUE;
	for (index = 0; index < count; index++)
	{
		if (mcc_workshop_package_folder(mcc_maps_level_name(index), other) && !csstrcasecmp(folder, other))
			return TRUE;
	}
	return FALSE;
}

/* the staged files are the host's content: fingerprinted where they are, with the package's own
files read from the staging folder, against the identity in the host's game record */
static int mcc_share_identity_check(
	void *user,
	char const *stage_dir)
{
	unsigned char actual[32];
	int ok;

	(void)user;
	mcc_package_stage_redirect(mcc_share_globals.level, stage_dir);
	ok = mcc_content_probe(mcc_share_globals.level, actual) && !csmemcmp(actual, mcc_share_digest, 32);
	mcc_package_stage_redirect(NULL, NULL);
	return ok;
}

static void mcc_share_fail_text(
	char const *reason)
{
	wchar_t text[512];
	short index;

	for (index = 0; reason[index] && index < NUMBEROF(text) - 1; index++)
		text[index] = (wchar_t)(unsigned char)reason[index];
	text[index] = 0;
	display_error_text_when_main_menu_loaded(text);
}

static void mcc_share_finished(
	void *user,
	int ok,
	char const *message)
{
	(void)user;
	if (!ok)
	{
		error(_error_silent, "mcc share: the download of %s failed: %s", mcc_share_globals.level, message);
		mcc_share_fail_text(message);
		network_game_abort();
		return;
	}
	error(_error_silent, "mcc share: installed %s from the host (credit: %s)", mcc_share_globals.level,
		message[0] ? message : "none given");
	/* (the installed package is judged as any other: its content must match the host's) */
	mcc_maps_rescan();
	if (!mcc_identity_accept_installed(mcc_share_name, mcc_share_digest))
		network_game_abort();
}

/* The joiner's package matches nothing installed (mcc_identity.c): fetch it from the host, if
this machine may and the package is not a Workshop item (those are subscribed to, not sent). */
static int mcc_share_missing(
	char const name[128],
	unsigned char const digest[32])
{
	struct mcc_workshop_item item;
	char canonical[64];

	if (!mcc_share_ready || global_network_game_server_get() || !global_network_game_client_get() ||
		!config_boolean("network.map_download") || mcc_workshop_wire_unpack(name, &item) ||
		!mcc_package_name(name, canonical))
	{
		return 0;
	}
	/* (the host changed the map during a download) */
	if (mcc_share_client_busy(&mcc_share_globals) && !mcc_share_client_level(&mcc_share_globals, canonical))
		mcc_share_client_cancel(&mcc_share_globals);
	if (mcc_share_client_level(&mcc_share_globals, canonical))
		return 1;
	if (!mcc_share_client_begin(&mcc_share_globals, canonical, digest))
		return 0;
	csmemcpy(mcc_share_name, name, sizeof(mcc_share_name));
	csmemcpy(mcc_share_digest, digest, 32);
	error(_error_silent, "mcc share: asking the host for %s", canonical);
	return 1;
}

boolean mcc_share_game_downloading(
	char const *level_name)
{
	char canonical[64];

	return mcc_share_ready && level_name && mcc_package_name(level_name, canonical) &&
		mcc_share_client_level(&mcc_share_globals, canonical);
}

boolean mcc_share_game_holds_players(
	void)
{
	return mcc_share_ready && mcc_share_client_busy(&mcc_share_globals);
}

boolean mcc_share_game_machine_active(
	long machine_index)
{
	return mcc_share_ready && machine_index >= 0 && machine_index < MCC_SHARE_HOST_MACHINES &&
		mcc_share_host_active(&mcc_share_globals, machine_index);
}

/* ---------- every frame */

void mcc_share_game_update(
	void)
{
	if (!mcc_share_ready)
	{
		struct mcc_share_env env;

		csmemset(&env, 0, sizeof(env));
		env.now = mcc_share_now;
		env.send_request = mcc_share_send_request;
		env.package_taken = mcc_share_package_taken;
		env.identity_check = mcc_share_identity_check;
		env.finished = mcc_share_finished;
		env.send_response = mcc_share_send_response;
		env.host_package = mcc_share_host_package_files;
		env.upload_limit = mcc_share_upload_limit;
		env.free_bytes = mcc_share_free_bytes;
		env.io.make_directory = mcc_share_io_make;
		env.io.remove_directory = mcc_share_io_remove;
		env.io.directory_exists = mcc_share_io_exists;
		env.io.move_file = mcc_share_io_move;
		env.io.delete_file = mcc_share_io_delete;
		mcc_share_init(&mcc_share_globals, &env);
		mcc_share_ready = TRUE;
		mcc_identity_set_missing(mcc_share_missing);
	}
	/* (the game left, its host went, or its map changed: nothing is kept) */
	if (mcc_share_client_busy(&mcc_share_globals))
	{
		struct network_game_client *client = global_network_game_client_get();
		struct network_game *game = client ? network_game_client_get_game(client) : NULL;
		char canonical[64];

		if (!game || !mcc_package_name(game->map.name, canonical) ||
			!mcc_share_client_level(&mcc_share_globals, canonical))
		{
			mcc_share_client_cancel(&mcc_share_globals);
		}
	}
	if (!global_network_game_server_get())
		mcc_share_host_stop(&mcc_share_globals);
	mcc_share_update(&mcc_share_globals);
}

void mcc_share_game_render(
	void)
{
	long font_tag_index;
	real_argb_color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	rectangle2d bounds;
	char const *name;
	int percent;

	if (!mcc_share_ready || !mcc_share_client_busy(&mcc_share_globals))
		return;
	font_tag_index = interface_get_tag_index(_interface_font_terminal);
	if (font_tag_index == NONE)
		return;
	name = strrchr(mcc_share_globals.level, '\\');
	name = name ? name + 1 : mcc_share_globals.level;
	percent = mcc_share_client_percent(&mcc_share_globals);
	if (percent <= 0)
		snprintf(mcc_share_text, sizeof(mcc_share_text), "Asking the host for the map %s...", name);
	else
		snprintf(mcc_share_text, sizeof(mcc_share_text), "Getting the map %s from the host: %d%%", name, percent);
	bounds = render.camera.window_bounds;
	offset_rectangle2d(&bounds, -render.camera.viewport_bounds.x0, -render.camera.viewport_bounds.y0);
	bounds.y0 = 24;
	bounds.y1 = 42;
	draw_string_set_draw_mode(font_tag_index, NONE, _mcc_share_text_centered, 0, &color);
	rasterizer_draw_string(&bounds, NULL, NULL, 0, mcc_share_text);
}
