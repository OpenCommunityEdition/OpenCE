/* The Bluetooth session thread (bt_session.h). */
#include "bt_session.h"
#include "bt_usb.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>

#define REQUESTS 24

enum { REQ_NONE, REQ_SEARCH, REQ_SEARCH_STOP, REQ_PAIR, REQ_FORGET, REQ_DISCONNECT, REQ_OUTPUT };

struct request {
    int type;
    unsigned char addr[6];
    struct bt_pad_output output;
};

struct bt_session {
    pthread_t thread;
    pthread_mutex_t lock;
    int quit;
    int started;
    struct bt_snapshot snap;
    struct request requests[REQUESTS];
    unsigned req_head, req_tail;
    char firmware_dir[512];
    char data_dir[512];
    FILE *log;
};

void bt_data_dir(char *out, unsigned size)
{
    const char *home = getenv("HOME");
    char path[600];
    if (!home) home = "/tmp";
    sprintf(path, "%s/Library", home); mkdir(path, 0755);
    sprintf(path, "%s/Library/Application Support", home); mkdir(path, 0755);
    sprintf(path, "%s/Library/Application Support/OpenCE", home); mkdir(path, 0755);
    sprintf(path, "%s/Library/Application Support/OpenCE/Bluetooth", home); mkdir(path, 0755);
    strncpy(out, path, size - 1);
    out[size - 1] = 0;
}

static int exists(const char *path)
{
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISREG(sb.st_mode);
}

void bt_firmware_find(const char *extra_dir, struct bt_firmware_info *out)
{
    char dirs[3][512];
    int i, n = 0;
    const char *home = getenv("HOME");
    memset(out, 0, sizeof(*out));
    if (extra_dir && extra_dir[0]) strncpy(dirs[n++], extra_dir, 511);
    if (home) {
        sprintf(dirs[n], "%s/Library/Application Support/OpenCE/Bluetooth", home);
        n++;
    }
    strcpy(dirs[n++], "/Library/Application Support/RealtekBTFirmware");
    for (i = 0; i < n; i++) {
        char fw[600];
        dirs[i][511] = 0;
        if (out->searched[0] && strlen(out->searched) + strlen(dirs[i]) + 3 < sizeof(out->searched)) strcat(out->searched, "\n");
        if (strlen(out->searched) + strlen(dirs[i]) + 1 < sizeof(out->searched)) strcat(out->searched, dirs[i]);
        sprintf(fw, "%s/rtl8761bu_fw.bin", dirs[i]);
        if (!out->found && exists(fw)) {
            char cfg[600];
            out->found = 1;
            strncpy(out->firmware, fw, sizeof(out->firmware) - 1);
            strncpy(out->where, dirs[i], sizeof(out->where) - 1);
            sprintf(cfg, "%s/rtl8761bu_config.bin", dirs[i]);
            if (exists(cfg)) strncpy(out->config, cfg, sizeof(out->config) - 1);
        }
    }
}

static void session_log(void *ctx, const char *line)
{
    struct bt_session *s = (struct bt_session *)ctx;
    fprintf(stderr, "bluetooth: %s\n", line);
    if (s->log) {
        fprintf(s->log, "%.3f %s\n", bt_now(), line);
        fflush(s->log);
    }
}

static void publish(struct bt_session *s, struct bt_stack *st, int phase)
{
    int i;
    pthread_mutex_lock(&s->lock);
    s->snap.phase = phase;
    if (st) {
        s->snap.dongle = st->dongle;
        strcpy(s->snap.message, st->message);
        strcpy(s->snap.chip, st->chip);
        s->snap.searching = st->search_until != 0;
        s->snap.pairing = st->pairing;
        s->snap.link_up = st->link_up;
        s->snap.found_count = st->found_count;
        for (i = 0; i < st->found_count; i++) s->snap.found[i] = st->found[i];
        s->snap.paired_count = st->paired_count;
        for (i = 0; i < st->paired_count; i++) s->snap.paired[i] = st->paired[i];
        memcpy(s->snap.local_addr, st->local_addr, 6);
        s->snap.pad = st->pad;
    }
    s->snap.serial++;
    pthread_mutex_unlock(&s->lock);
}

static void fail(struct bt_session *s, int dongle, const char *message)
{
    pthread_mutex_lock(&s->lock);
    s->snap.phase = BT_PHASE_FAILED;
    s->snap.dongle = dongle;
    strncpy(s->snap.message, message, sizeof(s->snap.message) - 1);
    s->snap.serial++;
    pthread_mutex_unlock(&s->lock);
}

static void drain_requests(struct bt_session *s, struct bt_stack *st)
{
    for (;;) {
        struct request r;
        pthread_mutex_lock(&s->lock);
        if (s->req_tail == s->req_head) { pthread_mutex_unlock(&s->lock); return; }
        r = s->requests[s->req_tail];
        s->req_tail = (s->req_tail + 1) % REQUESTS;
        pthread_mutex_unlock(&s->lock);
        switch (r.type) {
        case REQ_SEARCH: bt_stack_search(st, 60.0); break;
        case REQ_SEARCH_STOP: bt_stack_search_stop(st); break;
        case REQ_PAIR: bt_stack_pair(st, r.addr); break;
        case REQ_FORGET: bt_stack_forget(st, r.addr); break;
        case REQ_DISCONNECT: bt_stack_disconnect(st); break;
        case REQ_OUTPUT: bt_stack_output(st, &r.output); break;
        }
    }
}

static void *thread_main(void *arg)
{
    struct bt_session *s = (struct bt_session *)arg;
    struct bt_stack *st = bt_stack_create();
    struct bt_firmware_info fw;
    int dongle = BT_DONGLE_NONE;
    char reason[200];
    char path[700];
    double last = 0;

    if (!st) { fail(s, BT_DONGLE_NONE, "Out of memory."); return NULL; }
    st->log = session_log;
    st->log_ctx = s;
    sprintf(path, "%s/bluetooth.log", s->data_dir);
    s->log = fopen(path, "a");
    sprintf(st->keys_path, "%s/keys.txt", s->data_dir);
    bt_firmware_find(s->firmware_dir, &fw);
    if (fw.found) {
        strcpy(st->firmware_path, fw.firmware);
        strcpy(st->config_path, fw.config);
    }
    if (bt_usb_open(&st->tr, &dongle, reason, sizeof(reason)) != 0) {
        session_log(s, reason);
        fail(s, dongle, reason);
        bt_stack_destroy(st);
        if (s->log) fclose(s->log);
        return NULL;
    }
    st->dongle = BT_DONGLE_OPEN;
    publish(s, st, BT_PHASE_STARTING);
    if (bt_stack_start(st) != 0) {
        fail(s, BT_DONGLE_OPEN, st->message);
        bt_stack_stop(st);
        bt_stack_destroy(st);
        if (s->log) fclose(s->log);
        return NULL;
    }
    publish(s, st, BT_PHASE_READY);
    for (;;) {
        int quit;
        pthread_mutex_lock(&s->lock);
        quit = s->quit;
        pthread_mutex_unlock(&s->lock);
        if (quit) break;
        drain_requests(s, st);
        bt_stack_tick(st);
        bt_stack_pump(st, 0.01);
        if (st->failed) {
            fail(s, BT_DONGLE_OPEN, "The dongle stopped answering. Was it unplugged?");
            break;
        }
        if (bt_now() - last >= 0.02) {
            last = bt_now();
            publish(s, st, BT_PHASE_READY);
        }
    }
    bt_stack_stop(st);
    bt_stack_destroy(st);
    if (s->log) fclose(s->log);
    return NULL;
}

struct bt_session *bt_session_start(const char *firmware_dir)
{
    struct bt_session *s = (struct bt_session *)calloc(1, sizeof(*s));
    if (!s) return NULL;
    pthread_mutex_init(&s->lock, NULL);
    if (firmware_dir) strncpy(s->firmware_dir, firmware_dir, sizeof(s->firmware_dir) - 1);
    bt_data_dir(s->data_dir, sizeof(s->data_dir));
    s->snap.phase = BT_PHASE_STARTING;
    strcpy(s->snap.message, "Opening the dongle...");
    if (pthread_create(&s->thread, NULL, thread_main, s) != 0) {
        free(s);
        return NULL;
    }
    s->started = 1;
    return s;
}

void bt_session_snapshot(struct bt_session *s, struct bt_snapshot *out)
{
    pthread_mutex_lock(&s->lock);
    *out = s->snap;
    pthread_mutex_unlock(&s->lock);
}

static void post(struct bt_session *s, int type, const unsigned char *addr, const struct bt_pad_output *o)
{
    unsigned next;
    pthread_mutex_lock(&s->lock);
    next = (s->req_head + 1) % REQUESTS;
    if (next != s->req_tail) {
        struct request *r = &s->requests[s->req_head];
        memset(r, 0, sizeof(*r));
        r->type = type;
        if (addr) memcpy(r->addr, addr, 6);
        if (o) r->output = *o;
        s->req_head = next;
    }
    pthread_mutex_unlock(&s->lock);
}

void bt_session_search(struct bt_session *s) { post(s, REQ_SEARCH, NULL, NULL); }
void bt_session_search_stop(struct bt_session *s) { post(s, REQ_SEARCH_STOP, NULL, NULL); }
void bt_session_pair(struct bt_session *s, const unsigned char *addr) { post(s, REQ_PAIR, addr, NULL); }
void bt_session_forget(struct bt_session *s, const unsigned char *addr) { post(s, REQ_FORGET, addr, NULL); }
void bt_session_disconnect(struct bt_session *s) { post(s, REQ_DISCONNECT, NULL, NULL); }
void bt_session_output(struct bt_session *s, const struct bt_pad_output *o) { post(s, REQ_OUTPUT, NULL, o); }

void bt_session_stop(struct bt_session *s)
{
    if (!s) return;
    pthread_mutex_lock(&s->lock);
    s->quit = 1;
    pthread_mutex_unlock(&s->lock);
    if (s->started) pthread_join(s->thread, NULL);
    pthread_mutex_destroy(&s->lock);
    free(s);
}
