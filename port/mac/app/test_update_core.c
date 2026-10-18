/*
TEST_UPDATE_CORE.C

update_core.c on any Mac (Core Foundation and Monocypher are all it needs):

    cc -std=gnu99 -Wall -I../../third_party/monocypher -o test_update_core test_update_core.c update_core.c \
        ../../third_party/monocypher/monocypher.c ../../third_party/monocypher/monocypher-ed25519.c \
        -framework CoreFoundation && ./test_update_core <scratch folder>
    ./test_update_core --appcast FILE PUBLIC_KEY_BASE64 VERSION BUILD   (what the appcast FILE offers someone
        running VERSION and BUILD; tools/test_powerspark_update.py checks the signing tool's files with it)

Checks the version order, the appcast's reading and choosing, signatures, SHA-256, the look inside an unpacked
app, the swap (and its undoing), the read-only and not-writable folders, and the daily stamp.
*/
#include "update_core.h"
#include "monocypher-ed25519.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures;

static void check(int ok, const char *what)
{
	if (!ok)
	{
		printf("FAILED: %s\n", what);
		failures++;
	}
}

static void put(const char *path, const void *data, size_t length, int mode)
{
	FILE *f = fopen(path, "wb");

	if (!f)
	{
		printf("cannot write %s\n", path);
		exit(2);
	}
	fwrite(data, 1, length, f);
	fclose(f);
	chmod(path, (mode_t)mode);
}

static void put_text(const char *path, const char *text)
{
	put(path, text, strlen(text), 0644);
}

static void base64(const unsigned char *in, int n, char *out)
{
	static const char a[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i, o = 0;

	for (i = 0; i < n; i += 3)
	{
		unsigned long v = (unsigned long)in[i] << 16;

		if (i + 1 < n)
			v |= (unsigned long)in[i + 1] << 8;
		if (i + 2 < n)
			v |= in[i + 2];
		out[o++] = a[(v >> 18) & 63];
		out[o++] = a[(v >> 12) & 63];
		out[o++] = i + 1 < n ? a[(v >> 6) & 63] : '=';
		out[o++] = i + 2 < n ? a[v & 63] : '=';
	}
	out[o] = 0;
}

static uint8_t secret[64], pub[32];
static char pub64[64];

static void make_key(unsigned char fill)
{
	uint8_t seed[32];

	memset(seed, fill, sizeof(seed));
	crypto_ed25519_key_pair(secret, pub, seed);
	base64(pub, 32, pub64);
}

/* an item signed with the test key */
static void make_item(struct uc_item *it, const char *version, long build, const char *min_os)
{
	char statement[1024], sig[64];
	uint8_t raw[64];
	size_t n;

	memset(it, 0, sizeof(*it));
	snprintf(it->version, sizeof(it->version), "%s", version);
	it->build = build;
	snprintf(it->url, sizeof(it->url), "https://example.test/PowerSpark-%s.zip", version);
	memset(it->sha256, 'a', 64);
	it->length = 1234567;
	snprintf(it->min_os, sizeof(it->min_os), "%s", min_os);
	n = uc_statement(it, statement, sizeof(statement));
	crypto_ed25519_sign(raw, secret, (const uint8_t *)statement, n);
	base64(raw, 64, sig);
	snprintf(it->signature, sizeof(it->signature), "%s", sig);
}

static void item_xml(const struct uc_item *it, char *out, size_t size, const char *signature)
{
	snprintf(out, size,
		"<dict><key>version</key><string>%s</string><key>build</key><integer>%ld</integer>"
		"<key>url</key><string>%s</string><key>sha256</key><string>%s</string>"
		"<key>length</key><integer>%ld</integer><key>minimumSystemVersion</key><string>%s</string>"
		"<key>notes</key><string>Notes for %s.\n\nSecond paragraph.</string>"
		"<key>signature</key><string>%s</string></dict>",
		it->version, it->build, it->url, it->sha256, it->length, it->min_os, it->version,
		signature ? signature : it->signature);
}

static char *feed(const char *const *items, int count, int flat)
{
	static char text[16384];
	int i;

	strcpy(text, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><plist version=\"1.0\">");
	if (flat)
		strcat(text, items[0]);
	else
	{
		strcat(text, "<dict><key>feed</key><string>powerspark</string><key>versions</key><array>");
		for (i = 0; i < count; ++i)
			strcat(text, items[i]);
		strcat(text, "</array></dict>");
	}
	strcat(text, "</plist>");
	return text;
}

static void test_versions(void)
{
	check(uc_version_compare("0.2", "0.1") > 0, "0.2 > 0.1");
	check(uc_version_compare("0.10", "0.9") > 0, "0.10 > 0.9 (a number at a time)");
	check(uc_version_compare("1.0", "1.0.0") == 0, "1.0 == 1.0.0");
	check(uc_version_compare("0.3.11 Alpha", "0.3.11") == 0, "text after a number is ignored");
	check(uc_version_compare("0.3.11 Alpha", "0.3.2") > 0, "0.3.11 Alpha > 0.3.2");
	check(uc_version_compare("0.1", "0.1.1") < 0, "0.1 < 0.1.1");
	check(uc_version_compare("", "0") == 0, "empty is zero");
}

static void test_crypto(void)
{
	struct uc_item it, other;
	unsigned char raw[80];
	char hex[65];
	struct uc_sha256 s;
	int i;

	check(uc_base64_decode("AAECAw==", raw, sizeof(raw)) == 4 && raw[3] == 3, "base64 decodes");
	check(uc_base64_decode("AAECAw", raw, sizeof(raw)) == 4, "base64 without padding");
	check(uc_base64_decode("AA!CAw==", raw, sizeof(raw)) == -1, "base64 refuses other characters");
	check(uc_base64_decode("AAECAw==AAAA", raw, sizeof(raw)) == -1, "base64 refuses data after padding");
	check(uc_base64_decode("AAECAwQFBgc=", raw, 4) == -1, "base64 refuses a result that does not fit");

	uc_sha256_start(&s);
	uc_sha256_finish(&s, hex);
	check(!strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"), "SHA-256 of nothing");
	uc_sha256_start(&s);
	uc_sha256_add(&s, "abc", 3);
	uc_sha256_finish(&s, hex);
	check(!strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), "SHA-256 of abc");
	uc_sha256_start(&s);
	for (i = 0; i < 1000000; i += 100)
		uc_sha256_add(&s, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 100);
	uc_sha256_finish(&s, hex);
	check(!strcmp(hex, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"), "SHA-256 of a million a");
	uc_sha256_start(&s);
	uc_sha256_add(&s, "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56);
	uc_sha256_finish(&s, hex);
	check(!strcmp(hex, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"), "SHA-256 of 56 bytes (two blocks)");

	make_key(7);
	make_item(&it, "0.2", 20261101, "10.4");
	check(uc_signature_ok(&it, pub64), "a signed item is good");
	other = it;
	other.length++;
	check(!uc_signature_ok(&other, pub64), "a changed length breaks the signature");
	other = it;
	other.url[10] = 'X';
	check(!uc_signature_ok(&other, pub64), "a changed address breaks the signature");
	other = it;
	other.build++;
	check(!uc_signature_ok(&other, pub64), "a changed build breaks the signature");
	other = it;
	other.sha256[0] = 'b';
	check(!uc_signature_ok(&other, pub64), "a changed digest breaks the signature");
	other = it;
	other.min_os[1] = '5';
	check(!uc_signature_ok(&other, pub64), "a changed minimum system breaks the signature");
	other = it;
	other.signature[3] = other.signature[3] == 'A' ? 'B' : 'A';
	check(!uc_signature_ok(&other, pub64), "a changed signature is refused");
	other = it;
	other.signature[0] = 0;
	check(!uc_signature_ok(&other, pub64), "no signature is refused");
	{
		char wrong[64];
		uint8_t seed[32], s2[64], p2[32];

		memset(seed, 9, sizeof(seed));
		crypto_ed25519_key_pair(s2, p2, seed);
		base64(p2, 32, wrong);
		check(!uc_signature_ok(&it, wrong), "another key's signature is refused");
	}
	check(!uc_signature_ok(&it, ""), "no key refuses");
	check(!uc_signature_ok(&it, "AAAA"), "a short key refuses");
}

static void test_choose(void)
{
	struct uc_item a, b, c, old, got;
	char xa[3000], xb[3000], xc[3000], xold[3000], xbad[3000], err[300];
	const char *list[4];
	struct uc_current cur;
	char *text;

	make_key(7);
	cur.version = "0.1";
	cur.build = 20261018;
	cur.os_major = 10;
	cur.os_minor = 5;
	cur.public_key = pub64;

	make_item(&old, "0.0.9", 20260101, "10.4");
	make_item(&a, "0.2", 20261101, "10.4");
	make_item(&b, "0.3", 20261201, "10.4");
	make_item(&c, "0.4", 20270101, "10.6");	/* needs a newer Mac than cur's 10.5 */
	item_xml(&old, xold, sizeof(xold), NULL);
	item_xml(&a, xa, sizeof(xa), NULL);
	item_xml(&b, xb, sizeof(xb), NULL);
	item_xml(&c, xc, sizeof(xc), NULL);

	list[0] = xold; list[1] = xa; list[2] = xb; list[3] = xc;
	text = feed(list, 4, 0);
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_AVAILABLE, "a newer release is available");
	check(!strcmp(got.version, "0.3") && got.build == 20261201, "the newest that runs here is chosen (not 0.4, which needs 10.6)");
	check(got.length == 1234567 && !strcmp(got.min_os, "10.4") && strncmp(got.url, "https://", 8) == 0, "its facts are read");
	check(strstr(got.notes, "Notes for 0.3.") && strstr(got.notes, "Second paragraph.") && got.notes[strlen(got.notes) - 1] == '.',
		"its notes are read");

	cur.os_minor = 6;
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_AVAILABLE && !strcmp(got.version, "0.4"),
		"on 10.6 the 10.6 release is chosen");
	cur.os_minor = 4;
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_AVAILABLE && !strcmp(got.version, "0.3"),
		"on 10.4 the 10.4 release is chosen");
	cur.os_major = 9;
	cur.os_minor = 9;
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_UP_TO_DATE,
		"a Mac older than every release is offered none");
	cur.os_major = 10;
	cur.os_minor = 5;

	cur.version = "0.3";
	cur.build = 20261201;
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_UP_TO_DATE, "the same release is up to date");
	cur.build = 20260101;
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_AVAILABLE && got.build == 20261201,
		"the same version with a newer build is an update");
	cur.version = "0.9";
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_UP_TO_DATE, "a newer app is up to date");
	cur.version = "0.1";
	cur.build = 20261018;

	/* a flat appcast (Garden's form) */
	list[0] = xa;
	text = feed(list, 1, 1);
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_AVAILABLE && !strcmp(got.version, "0.2"),
		"a single release at the top level is read");

	/* the newest is not signed: refused, and the older signed one is offered instead */
	item_xml(&b, xbad, sizeof(xbad), "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==");
	list[0] = xa; list[1] = xbad;
	text = feed(list, 2, 0);
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_AVAILABLE && !strcmp(got.version, "0.2"),
		"an unsigned newer release is passed over for a signed one");
	list[0] = xbad;
	text = feed(list, 1, 0);
	err[0] = 0;
	check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_ERROR && strstr(err, "not signed"),
		"only an unsigned release is an error that says why");

	/* a release that is not described properly is not offered */
	{
		struct uc_item p = a;
		char x[3000];

		strcpy(p.url, "http://example.test/PowerSpark.zip");
		make_item(&p, "0.5", 20270201, "10.4");
		strcpy(p.url, "http://example.test/PowerSpark.zip");
		item_xml(&p, x, sizeof(x), NULL);
		list[0] = x;
		text = feed(list, 1, 0);
		check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_UP_TO_DATE, "an http address is not offered");
		make_item(&p, "0.5", 20270201, "10.4");
		p.length = 0;
		item_xml(&p, x, sizeof(x), NULL);
		list[0] = x;
		text = feed(list, 1, 0);
		check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_UP_TO_DATE, "a length of 0 is not offered");
		make_item(&p, "0.5", 20270201, "10.4");
		p.sha256[3] = 'G';
		item_xml(&p, x, sizeof(x), NULL);
		list[0] = x;
		text = feed(list, 1, 0);
		check(uc_choose(text, strlen(text), &cur, &got, err, sizeof(err)) == UC_UP_TO_DATE, "a bad digest is not offered");
	}

	check(uc_choose("not a plist", 11, &cur, &got, err, sizeof(err)) == UC_ERROR, "garbage is an error");
	check(uc_choose("", 0, &cur, &got, err, sizeof(err)) == UC_ERROR, "an empty feed is an error");
	check(uc_choose("<plist version=\"1.0\"><array/></plist>", 38, &cur, &got, err, sizeof(err)) == UC_ERROR,
		"a feed that is not a dictionary is an error");
}

/* a fake fat file holding the cpus, and a thin one */
static void fat(const char *path, const long *cpus, int count)
{
	unsigned char b[8 + 20 * 4 + 16];
	int i;

	memset(b, 0, sizeof(b));
	b[0] = 0xCA; b[1] = 0xFE; b[2] = 0xBA; b[3] = 0xBE;
	b[7] = (unsigned char)count;
	for (i = 0; i < count; ++i)
		b[8 + 20 * i + 3] = (unsigned char)cpus[i];
	put(path, b, sizeof(b), 0755);
}

static void make_app(const char *app, const char *id, const char *version, long build, const long *cpus, int count)
{
	char path[1024], plist[1024];

	snprintf(path, sizeof(path), "%s/Contents/MacOS", app);
	snprintf(plist, sizeof(plist), "mkdir -p '%s'", path);
	if (system(plist) != 0)
		exit(2);
	snprintf(plist, sizeof(plist),
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?><plist version=\"1.0\"><dict>"
		"<key>CFBundleIdentifier</key><string>%s</string>"
		"<key>CFBundleShortVersionString</key><string>%s</string>"
		"<key>CFBundleVersion</key><string>%ld</string></dict></plist>", id, version, build);
	snprintf(path, sizeof(path), "%s/Contents/Info.plist", app);
	put_text(path, plist);
	snprintf(path, sizeof(path), "%s/Contents/MacOS/PowerSpark", app);
	fat(path, cpus, count);
	snprintf(path, sizeof(path), "%s/Contents/MacOS/PowerSpark-game", app);
	fat(path, cpus, count);
}

static int exists(const char *path)
{
	struct stat st;

	return stat(path, &st) == 0;
}

static void test_bundle_and_swap(const char *root)
{
	static const long both[] = { UC_CPU_PPC, UC_CPU_I386 }, ppc_only[] = { UC_CPU_PPC };
	char work[1024], app[1024], fresh[1024], old[1024], err[400], found[1024], path[1024], text[2048];

	snprintf(work, sizeof(work), "%s/swap", root);
	snprintf(text, sizeof(text), "rm -rf '%s' && mkdir -p '%s/unpacked'", work, work);
	if (system(text) != 0)
		exit(2);
	snprintf(fresh, sizeof(fresh), "%s/unpacked/PowerSpark.app", work);
	snprintf(app, sizeof(app), "%s/PowerSpark.app", work);

	make_app(fresh, "org.opence.PowerSpark", "0.2", 20261101, both, 2);
	check(uc_find_app(work, found, sizeof(found), err, sizeof(err)) != 0, "no app in the folder itself (it is in unpacked)");
	snprintf(path, sizeof(path), "%s/unpacked", work);
	check(uc_find_app(path, found, sizeof(found), err, sizeof(err)) == 0 && !strcmp(found, fresh), "the one app is found");
	check(uc_validate_bundle(fresh, "org.opence.PowerSpark", "0.2", 20261101, UC_CPU_I386, err, sizeof(err)) == 0,
		"a good bundle validates on Intel");
	check(uc_validate_bundle(fresh, "org.opence.PowerSpark", "0.2", 20261101, UC_CPU_PPC, err, sizeof(err)) == 0,
		"a good bundle validates on PowerPC");
	check(uc_validate_bundle(fresh, "org.opence.PowerSpark", "0.2", 0, UC_CPU_PPC, err, sizeof(err)) == 0,
		"build 0 is not checked");
	check(uc_validate_bundle(fresh, "org.other.App", "0.2", 20261101, UC_CPU_I386, err, sizeof(err)) != 0 &&
		strstr(err, "not PowerSpark"), "another bundle identifier is refused");
	check(uc_validate_bundle(fresh, "org.opence.PowerSpark", "0.3", 20261101, UC_CPU_I386, err, sizeof(err)) != 0 &&
		strstr(err, "version"), "another version is refused");
	check(uc_validate_bundle(fresh, "org.opence.PowerSpark", "0.2", 5, UC_CPU_I386, err, sizeof(err)) != 0 &&
		strstr(err, "build"), "another build is refused");
	{
		char other[1024];

		snprintf(other, sizeof(other), "%s/unpacked/Other.app", work);
		make_app(other, "org.opence.PowerSpark", "0.2", 20261101, ppc_only, 1);
		check(uc_validate_bundle(other, "org.opence.PowerSpark", "0.2", 20261101, UC_CPU_I386, err, sizeof(err)) != 0 &&
			strstr(err, "processor"), "a PowerPC-only bundle is refused on Intel");
		check(uc_validate_bundle(other, "org.opence.PowerSpark", "0.2", 20261101, UC_CPU_PPC, err, sizeof(err)) == 0,
			"and accepted on PowerPC");
		check(uc_find_app(path, found, sizeof(found), err, sizeof(err)) != 0, "two apps are refused");
		snprintf(text, sizeof(text), "rm -rf '%s'", other);
		system(text);
	}
	snprintf(path, sizeof(path), "%s/Contents/MacOS/PowerSpark-game", fresh);
	chmod(path, 0644);
	check(uc_validate_bundle(fresh, "org.opence.PowerSpark", "0.2", 20261101, UC_CPU_I386, err, sizeof(err)) != 0 &&
		strstr(err, "lacks"), "a game program that is not executable is refused");
	chmod(path, 0755);
	unlink(path);
	check(uc_validate_bundle(fresh, "org.opence.PowerSpark", "0.2", 20261101, UC_CPU_I386, err, sizeof(err)) != 0,
		"a missing game program is refused");
	fat(path, both, 2);
	check(uc_validate_bundle(fresh, "org.opence.PowerSpark", "0.2", 20261101, UC_CPU_I386, err, sizeof(err)) == 0,
		"the game program is restored");

	/* the old app, with a marker */
	make_app(app, "org.opence.PowerSpark", "0.1", 20261018, both, 2);
	snprintf(path, sizeof(path), "%s/Contents/marker", app);
	put_text(path, "old");
	snprintf(path, sizeof(path), "%s/Contents/marker", fresh);
	put_text(path, "new");

	check(uc_place_check(app, err, sizeof(err)) == UC_PLACE_OK, "a folder of ours is writable");
	check(uc_swap(fresh, app, old, sizeof(old), err, sizeof(err)) == 0, "the swap works");
	snprintf(path, sizeof(path), "%s/Contents/marker", app);
	{
		char buf[8] = "";
		FILE *f = fopen(path, "rb");

		if (f)
		{
			fread(buf, 1, 3, f);
			fclose(f);
		}
		check(!strncmp(buf, "new", 3), "the new app is in place");
	}
	check(exists(old) && strstr(old, "PowerSpark.app.old"), "the old app is PowerSpark.app.old");
	snprintf(path, sizeof(path), "%s/Contents/marker", old);
	check(exists(path), "the old app is whole");
	check(!exists(fresh), "the unpacked app moved");

	/* the new app cannot be put in place: the old one comes back */
	make_app(fresh, "org.opence.PowerSpark", "0.3", 1, both, 2);
	snprintf(text, sizeof(text), "%s/gone/PowerSpark.app", work);
	check(uc_swap(text, app, old, sizeof(old), err, sizeof(err)) != 0 && strstr(err, "old one was kept"),
		"a failed swap says the old one was kept");
	snprintf(path, sizeof(path), "%s/Contents/marker", app);
	check(exists(path) && uc_validate_bundle(app, "org.opence.PowerSpark", "0.2", 20261101, UC_CPU_I386, err, sizeof(err)) == 0,
		"and the app is the one that was there");

	/* leftovers: the .old of a PowerSpark goes, one of anything else stays */
	uc_clean_leftovers(app, "org.opence.PowerSpark");
	snprintf(path, sizeof(path), "%s/PowerSpark.app.old", work);
	check(!exists(path), "the leftover .old is removed");
	make_app(path, "org.other.App", "1", 1, both, 2);
	uc_clean_leftovers(app, "org.opence.PowerSpark");
	check(exists(path), "an .old that is not PowerSpark is left alone");
	{
		char work_dir[1024], fresh_dir[1024];

		snprintf(work_dir, sizeof(work_dir), "%s/%scrashed", work, UC_WORK_PREFIX);
		snprintf(fresh_dir, sizeof(fresh_dir), "%s/%srecent", work, UC_WORK_PREFIX);
		mkdir(work_dir, 0755);
		mkdir(fresh_dir, 0755);
		{
			char cmd[2200];

			snprintf(cmd, sizeof(cmd), "touch -t 200001010000 '%s'", work_dir);
			system(cmd);
		}
		uc_clean_leftovers(app, "org.opence.PowerSpark");
		check(!exists(work_dir), "an update folder from long ago is removed");
		check(exists(fresh_dir), "a recent one (maybe in use) is kept");
	}

	/* the places an app cannot be replaced */
	if (geteuid() != 0)
	{
		snprintf(path, sizeof(path), "%s", work);
		chmod(path, 0555);
		check(uc_place_check(app, err, sizeof(err)) == UC_PLACE_NOT_WRITABLE && strstr(err, "Nothing was changed"),
			"a folder that cannot be written says so");
		chmod(path, 0755);
	}
	check(uc_place_check("/Volumes/definitely-not-here/PowerSpark.app", err, sizeof(err)) == UC_PLACE_NOT_WRITABLE,
		"a folder that is not there is not writable");

	check(uc_remove_tree(work) == 0 && !exists(work), "a tree is removed");
}

static void test_stamp(const char *root)
{
	char path[1024];

	snprintf(path, sizeof(path), "%s/update-check", root);
	unlink(path);
	check(uc_check_due(path, 1000000, 86400), "no stamp: due");
	check(uc_check_stamp(path, 1000000) == 0, "the stamp is written");
	check(!uc_check_due(path, 1000000 + 3600, 86400), "an hour later: not due");
	check(!uc_check_due(path, 1000000 + 86399, 86400), "just under a day: not due");
	check(uc_check_due(path, 1000000 + 86400, 86400), "a day later: due");
	check(uc_check_due(path, 1000000 - 5, 86400), "a stamp from the future: due");
	put_text(path, "garbage\n");
	check(uc_check_due(path, 1000000, 86400), "a garbled stamp: due");
}

static int appcast_mode(int argc, char **argv)
{
	FILE *f;
	static char data[UC_MAX_APPCAST + 1];
	size_t n;
	struct uc_current cur;
	struct uc_item got;
	char err[300] = "";
	int r;

	if (argc != 6)
		return 2;
	f = fopen(argv[2], "rb");
	if (!f)
		return 2;
	n = fread(data, 1, UC_MAX_APPCAST, f);
	fclose(f);
	cur.version = argv[4];
	cur.build = atol(argv[5]);
	cur.os_major = 10;
	cur.os_minor = 6;
	cur.public_key = argv[3];
	r = uc_choose(data, n, &cur, &got, err, sizeof(err));
	if (r == UC_AVAILABLE)
		printf("available %s %ld %ld %s\n", got.version, got.build, got.length, got.sha256);
	else
		printf("%s %s\n", r == UC_ERROR ? "error" : "current", err);
	return r == UC_ERROR ? 1 : 0;
}

int main(int argc, char **argv)
{
	if (argc >= 2 && !strcmp(argv[1], "--appcast"))
		return appcast_mode(argc, argv);
	if (argc != 2)
	{
		printf("usage: test_update_core <scratch folder>\n");
		return 2;
	}
	mkdir(argv[1], 0755);
	test_versions();
	test_crypto();
	test_choose();
	test_bundle_and_swap(argv[1]);
	test_stamp(argv[1]);
	if (failures)
	{
		printf("%d failed\n", failures);
		return 1;
	}
	printf("update core: ok\n");
	return 0;
}
