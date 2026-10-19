/* The Bluetooth pad for the game (mac_bt_input.h). */
#include "mac_bt_input.h"
#include "bt/bt_session.h"
#include "../linux/src/port_config.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>

/* how long the dongle is left alone after it was unplugged or held by another program */
#define RETRY_SECONDS 10.0
/* the motors: a change goes at once but no faster than this, and a motor left on is told again
   before the stack's own timeout (0.4 s) switches it off */
#define RUMBLE_MINIMUM_SECONDS 0.05
#define RUMBLE_REPEAT_SECONDS 0.25

static struct bt_session *session;
static unsigned long settings_read_at = (unsigned long)-1;
static int enabled;
static double next_try;
static int gave_up;                   /* the stack failed for a reason that waiting does not mend */
static unsigned long gave_up_at;      /* config_changes() then: a changed setting (the firmware folder) tries again */
static int ready_logged, pad_logged;
static char pad_name[BT_NAME_LENGTH];
static int pad_connected;
static unsigned char want_strong, want_weak, sent_strong, sent_weak;
static double rumble_posted;
static int exit_hook;

static void note(const char *text)
{
    fprintf(stderr, "bluetooth input: %s\n", text);
}

static void *stop_thread(void *argument)
{
    bt_session_stop((struct bt_session *)argument);
    return NULL;
}

static void forget_pad(void)
{
    pad_connected = 0;
    pad_name[0] = 0;
    pad_logged = 0;
    ready_logged = 0;
    want_strong = want_weak = sent_strong = sent_weak = 0;
}

/* lets the session go without waiting for it (it needs about a second to disconnect the pad) */
static void release_later(void)
{
    pthread_t thread;
    pthread_attr_t attributes;
    struct bt_session *s = session;
    session = NULL;
    forget_pad();
    if (!s) return;
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &attributes, stop_thread, s) != 0)
        bt_session_stop(s);
    pthread_attr_destroy(&attributes);
}

void mac_bt_input_stop(void)
{
    struct bt_session *s = session;
    session = NULL;
    forget_pad();
    if (s) bt_session_stop(s);
}

static void read_settings(void)
{
    unsigned long version = config_changes();
    if (version == settings_read_at) return;
    settings_read_at = version;
    enabled = config_boolean("input.bluetooth") ? 1 : 0;
}

static void start_session(double now)
{
    const char *folder = config_string("input.bluetooth_firmware_dir");
    if (folder && !folder[0]) folder = NULL;
    session = bt_session_start(folder);
    if (!session) {
        note("could not start the Bluetooth thread");
        next_try = now + RETRY_SECONDS;
        return;
    }
    if (!exit_hook) {
        exit_hook = 1;
        atexit(mac_bt_input_stop);
    }
    note("opening the dongle");
}

/* port: whether this is Mac OS X 10.4 or earlier (Darwin 8 and below). There Apple's Bluetooth driver takes
   every USB Bluetooth dongle and does not let go, so a dongle that is "busy" or Apple's will not become free
   by waiting: trying again every ten seconds for the whole session only fills the log. */
static int before_leopard(void)
{
    static int known = -1;
    if (known < 0) {
        char release[32];
        size_t size = sizeof(release);
        known = 0;
        if (sysctlbyname("kern.osrelease", release, &size, NULL, 0) == 0 && size > 0) {
            release[sizeof(release) - 1] = 0;
            known = atoi(release) < 9;
        }
    }
    return known;
}

/* the stack gave up (its message says why): it is started again only when the dongle may have become
   free (not there, or held by another program), and for anything else when a setting changes */
static void session_failed(const struct bt_snapshot *snap, double now)
{
    char line[220];
    int retry = snap->dongle == BT_DONGLE_NONE || snap->dongle == BT_DONGLE_BUSY;
    sprintf(line, "unavailable: %s", snap->message);
    note(line);
    if (retry && snap->dongle == BT_DONGLE_BUSY && before_leopard()) {
        retry = 0;
        note("Mac OS X 10.4: Apple's driver holds the dongle and does not let go; not trying again");
    }
    release_later();
    if (retry) next_try = now + RETRY_SECONDS;
    else {
        gave_up = 1;
        gave_up_at = settings_read_at;
    }
}

static void post_rumble(double now)
{
    struct bt_pad_output o;
    memset(&o, 0, sizeof(o));
    o.strong = want_strong;
    o.weak = want_weak;
    o.set_rumble = 1;
    bt_session_output(session, &o);
    sent_strong = want_strong;
    sent_weak = want_weak;
    rumble_posted = now;
}

int mac_bt_input_poll(struct mac_gamepad_frame *out)
{
    struct bt_snapshot snap;
    double now;
    read_settings();
    if (!enabled) {
        if (session) {
            note("off");
            release_later();
        }
        gave_up = 0;
        return 0;
    }
    if (gave_up && gave_up_at == settings_read_at) return 0;
    gave_up = 0;
    now = bt_now();
    if (!session) {
        if (now < next_try) return 0;
        start_session(now);
        if (!session) return 0;
    }
    bt_session_snapshot(session, &snap);
    if (snap.phase == BT_PHASE_FAILED) {
        session_failed(&snap, now);
        return 0;
    }
    if (snap.phase == BT_PHASE_READY && !ready_logged) {
        ready_logged = 1;
        note("dongle ready; press PS on a paired controller");
    }
    if (!snap.pad.connected || !snap.pad.streaming) {
        if (pad_connected) {
            note("controller disconnected");
            forget_pad();
            ready_logged = 1;
        }
        return 0;
    }
    if (!pad_connected) {
        pad_connected = 1;
        strncpy(pad_name, snap.pad.name, sizeof(pad_name) - 1);
        pad_name[sizeof(pad_name) - 1] = 0;
        if (!pad_name[0]) strcpy(pad_name, "Bluetooth controller");
    }
    if (!pad_logged) {
        char line[160];
        pad_logged = 1;
        sprintf(line, "%s connected", pad_name);
        note(line);
    }
    /* the motors: a change, or a motor left on */
    if (now - rumble_posted >= RUMBLE_MINIMUM_SECONDS &&
        (want_strong != sent_strong || want_weak != sent_weak ||
         ((want_strong || want_weak) && now - rumble_posted >= RUMBLE_REPEAT_SECONDS)))
        post_rumble(now);
    *out = snap.pad.frame;
    out->connected = 1;
    return 1;
}

int mac_bt_input_rumble(unsigned char strong, unsigned char weak)
{
    if (!session || !pad_connected) return 0;
    want_strong = strong;
    want_weak = weak;
    return 1;
}

const char *mac_bt_input_name(void)
{
    return pad_connected ? pad_name : "";
}
