/* A thread that owns the dongle for a user interface (bt_session.c). The
   thread opens the transport on its own run loop, starts the stack, and
   pumps it; the interface asks for things (search, pair, light, rumble) and
   reads a snapshot, both through a mutex. Nothing else is shared. */
#ifndef BT_SESSION_H
#define BT_SESSION_H

#include "bt_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    BT_PHASE_STOPPED,    /* not started */
    BT_PHASE_STARTING,   /* opening the dongle and loading its firmware */
    BT_PHASE_READY,      /* up: waiting for a pad, or searching */
    BT_PHASE_FAILED      /* the dongle could not be used (message says why) */
};

struct bt_firmware_info {
    int found;
    char firmware[512];
    char config[512];
    char where[512];     /* the folder it was found in */
    char searched[1024]; /* the folders looked in, for the message */
};

/* Looks for rtl8761bu_fw.bin (and rtl8761bu_config.bin) in extra_dir (from the
   config file; may be NULL or empty), ~/Library/Application Support/OpenCE/Bluetooth/
   and /Library/Application Support/RealtekBTFirmware/. The firmware is not part of
   this program. */
void bt_firmware_find(const char *extra_dir, struct bt_firmware_info *out);
/* "~/Library/Application Support/OpenCE/Bluetooth" expanded, with the folder created */
void bt_data_dir(char *out, unsigned size);

struct bt_snapshot {
    int phase;
    int dongle;                 /* BT_DONGLE_* */
    char message[160];
    char chip[96];
    int searching, pairing, link_up;
    int found_count;
    struct bt_found found[BT_MAX_FOUND];
    int paired_count;
    struct bt_paired paired[BT_MAX_PAIRED];
    unsigned char local_addr[6];
    struct bt_pad_status pad;
    unsigned serial;
};

struct bt_session;

/* firmware_dir may be NULL; the data folder (keys, log) is bt_data_dir's. */
struct bt_session *bt_session_start(const char *firmware_dir);
void bt_session_snapshot(struct bt_session *s, struct bt_snapshot *out);
void bt_session_search(struct bt_session *s);
void bt_session_search_stop(struct bt_session *s);
void bt_session_pair(struct bt_session *s, const unsigned char *addr);
void bt_session_forget(struct bt_session *s, const unsigned char *addr);
void bt_session_disconnect(struct bt_session *s);
void bt_session_output(struct bt_session *s, const struct bt_pad_output *o);
/* stops the stack and the thread, and frees the session */
void bt_session_stop(struct bt_session *s);

#ifdef __cplusplus
}
#endif
#endif
