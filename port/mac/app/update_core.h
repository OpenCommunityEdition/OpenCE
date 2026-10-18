/* PowerSpark's updater, the parts that are not windows: reading the appcast, choosing the release, checking its
   signature, SHA-256, looking inside an unpacked PowerSpark.app, and swapping it in for the running one. Plain C89
   with Core Foundation (the appcast is a property list) and Monocypher's Ed25519, so that a host test can run it
   (test_update_core.c) and Mac OS X 10.4 to 10.6, ppc and i386, can run it in the launcher.

   The appcast is the user's usual one (Garden's and Captain Polliwog's): a property list whose "versions" array
   holds one dictionary for each release (a lone release may be the top level itself):
     version                 "0.2"            (dotted numbers; text after a number is ignored)
     build                   20261101         (CFBundleVersion; breaks a tie between equal versions)
     url                     https://...zip
     sha256                  64 hex digits of the zip
     length                  the zip's size in bytes (an integer)
     minimumSystemVersion    "10.4"
     notes                   what is new, shown before installing
     signature               base64 Ed25519 signature of the statement below
   The signed statement binds all of these to the release key, so a signature cannot be moved to another file:
     PowerSpark-update-1\nversion=V\nbuild=B\nsha256=H\nlength=L\nurl=U\nminimumSystemVersion=M\n
   (tools/powerspark_sign_update.py makes it; a release with no minimumSystemVersion signs an empty one). */
#ifndef UPDATE_CORE_H
#define UPDATE_CORE_H

#include <stddef.h>
#include <stdint.h>

#define UC_URL 512
#define UC_NOTES 6000
#define UC_MAX_LENGTH (1024L * 1024L * 1024L)	/* no release is larger than this */
#define UC_MAX_APPCAST (1024 * 1024)		/* an appcast is a few kilobytes */

struct uc_item
{
	char version[32];
	long build;
	char url[UC_URL];
	char sha256[65];
	long length;
	char min_os[16];
	char notes[UC_NOTES];
	char signature[128];
};

struct uc_current
{
	const char *version;		/* CFBundleShortVersionString */
	long build;			/* CFBundleVersion */
	int os_major, os_minor;		/* the Mac: 10, 4 to 6 */
	const char *public_key;		/* the release key, base64 (32 bytes) */
};

enum { UC_ERROR = -1, UC_UP_TO_DATE = 0, UC_AVAILABLE = 1 };

/* negative, 0 or positive as a is older than, the same as or newer than b ("0.3.11 Alpha" is 0.3.11) */
int uc_version_compare(const char *a, const char *b);

/* the signed statement of an item (without its notes) */
size_t uc_statement(const struct uc_item *item, char *out, size_t size);

/* base64 to bytes; the count, or -1 if the text is not base64 or does not fit */
int uc_base64_decode(const char *text, unsigned char *out, size_t size);
/* a lowercase hex string of 64 digits */
int uc_is_sha256_hex(const char *text);

/* the Ed25519 signature (base64) of the item's statement against the public key (base64): 1 if it is good */
int uc_signature_ok(const struct uc_item *item, const char *public_key_base64);

/* the newest release that is newer than the running one, runs on this Mac, is described properly, and is signed
   by the key. UC_AVAILABLE fills *out; UC_UP_TO_DATE: nothing newer; UC_ERROR: the appcast cannot be read, or its
   newest release is not signed (err says which) */
int uc_choose(const void *data, size_t length, const struct uc_current *current, struct uc_item *out, char *err,
	size_t err_size);

/* SHA-256, a piece at a time (a release is tens of megabytes on a machine with little memory to spare) */
struct uc_sha256
{
	uint32_t state[8];
	uint64_t bits;
	unsigned char block[64];
	unsigned int used;
};
void uc_sha256_start(struct uc_sha256 *s);
void uc_sha256_add(struct uc_sha256 *s, const void *data, size_t length);
void uc_sha256_finish(struct uc_sha256 *s, char hex[65]);
/* the file's size and digest; 0 if it could not be read */
int uc_sha256_file(const char *path, long *size, char hex[65]);

/* ---- the unpacked bundle */

enum { UC_CPU_PPC = 18, UC_CPU_I386 = 7 };
/* a Mach-O file, thin or universal, that holds code for the cpu (UC_CPU_*) */
int uc_macho_has_cpu(const char *path, int cpu);
/* the cpu this program runs as */
int uc_this_cpu(void);

/* The folder holds just one item, a PowerSpark.app whose Info.plist has this bundle identifier, the version and
   build that were offered (build 0: not checked), and whose launcher and game programs hold code for the cpu.
   0 if it is; else -1 and err says what is wrong */
int uc_validate_bundle(const char *app, const char *bundle_id, const char *version, long build, int cpu, char *err,
	size_t err_size);
/* the single .app in a folder (its path in out); 0, or -1 with err when there is none or something else is there */
int uc_find_app(const char *folder, char *out, size_t size, char *err, size_t err_size);

/* ---- putting it in place */

enum { UC_PLACE_OK = 0, UC_PLACE_READ_ONLY = 1, UC_PLACE_NOT_WRITABLE = 2 };
/* whether the folder holding the app can be changed by this user, with a message for the player when it cannot */
int uc_place_check(const char *app, char *err, size_t err_size);
/* removes a file or a folder and what is in it; 0, or -1 */
int uc_remove_tree(const char *path);
/* app becomes app.old and new_app becomes app (both are folders on one volume, so each is one rename). If the
   second rename fails the first is undone. old_out gets app.old's path. 0, or -1 with err; the old app is then
   where it was */
int uc_swap(const char *new_app, const char *app, char *old_out, size_t old_size, char *err, size_t err_size);
/* a PowerSpark.app.old left beside the app by an earlier update, and update folders older than an hour that a
   crash left, are removed (only what is a PowerSpark bundle, or has the update folder's name) */
void uc_clean_leftovers(const char *app, const char *bundle_id);
#define UC_WORK_PREFIX ".PowerSpark-update-"

/* ---- when to look */

/* the file holds the time of the last check (seconds since 1970): 1 if more than interval seconds ago or none */
int uc_check_due(const char *path, long now, long interval);
int uc_check_stamp(const char *path, long now);

#endif
