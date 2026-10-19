/* A small Bluetooth Classic HID host in user space, for Mac OS X 10.4 and
   later (PowerPC and Intel), over a USB dongle with a Realtek RTL8761B chip
   (TP-Link UB500). Written for this project from the Bluetooth Core
   Specification (HCI, L2CAP, Secure Simple Pairing), the HID profile and
   public descriptions of the DualSense's and DualShock 4's reports; nothing
   is copied from another stack. C89, no Objective-C, and every field read from
   the air or the dongle is read a byte at a time (no struct casts), so a
   big-endian PowerPC and a little-endian Intel Mac behave alike.

   The pieces, one file each:
     bt_usb_iokit.c  the HCI USB transport (IOKit), with its run-loop source
     bt_realtek.c    the Realtek firmware loader (vendor command 0xFC20)
     bt_hci.c        commands, events, discovery, links
     bt_ssp.c        Secure Simple Pairing and link keys
     bt_l2cap.c      ACL fragmenting, channels, signalling
     bt_hid.c        the HID control (0x11) and interrupt (0x13) channels
     bt_pad.c        DualSense and DualShock 4 report shims, output reports
     bt_keys.c       the link-key file
     bt_session.c    a thread that owns the dongle, for a user interface
   The stack itself (everything but the transport and the session) has no
   threads and no platform calls: a transport (struct bt_transport) feeds it
   packets, so tests run it against a fake dongle (bt_fake.c).

   Addresses are kept in the order the wire has them, least significant byte
   first; bt_addr_text prints them the usual way round. */
#ifndef BT_STACK_H
#define BT_STACK_H

#include "../mac_gamepad_report.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- helpers (bt_util.c) ---- */

unsigned bt_get16(const unsigned char *p);
unsigned bt_get24(const unsigned char *p);
unsigned bt_get32(const unsigned char *p);
void bt_put16(unsigned char *p, unsigned value);
void bt_put32(unsigned char *p, unsigned value);
/* the reflected IEEE CRC-32 (polynomial 0xEDB88320), as Sony's reports use:
   start with 0xFFFFFFFF and complement the result */
unsigned bt_crc32_update(unsigned crc, const unsigned char *data, unsigned size);
/* the CRC of a report: a one-byte HIDP header (0xA1 input, 0xA2 output) is
   part of it though it is not in the report */
unsigned bt_crc32_report(unsigned char header, const unsigned char *data, unsigned size);
/* "aa:bb:cc:dd:ee:ff" in out[18] */
void bt_addr_text(const unsigned char *addr, char *out);
int bt_addr_parse(const char *text, unsigned char *addr);
double bt_now(void);

/* ---- the transport (a USB dongle, or a fake one for tests) ---- */

enum { BT_PACKET_NONE = 0, BT_PACKET_EVENT = 1, BT_PACKET_ACL = 2 };
#define BT_PACKET_MAX 1200

struct bt_transport {
    void *ctx;
    /* an HCI command packet: opcode (2), parameter length (1), parameters */
    int (*send_command)(void *ctx, const unsigned char *packet, unsigned size);
    /* an HCI ACL packet: handle and flags (2), length (2), data */
    int (*send_acl)(void *ctx, const unsigned char *packet, unsigned size);
    /* the next packet received, copied into buf (BT_PACKET_EVENT: the event
       code, the length, the parameters; BT_PACKET_ACL: the ACL packet), or
       BT_PACKET_NONE */
    int (*receive)(void *ctx, unsigned char *buf, unsigned cap, unsigned *size);
    /* runs the transport's event loop for up to this many seconds, or until
       something arrives */
    void (*wait)(void *ctx, double seconds);
    int (*failed)(void *ctx);
    void (*close)(void *ctx);
};

/* ---- the stack ---- */

#define BT_MAX_FOUND 12
#define BT_MAX_PAIRED 8
#define BT_MAX_CHANNELS 4
#define BT_COMMAND_QUEUE 64
#define BT_NAME_LENGTH 64

enum { BT_KIND_UNKNOWN, BT_KIND_DUALSENSE, BT_KIND_DUALSHOCK4 };

/* a device the inquiry found */
struct bt_found {
    unsigned char addr[6];
    unsigned cod;
    int rssi;
    unsigned page_scan_mode;
    unsigned clock_offset;
    char name[BT_NAME_LENGTH];
    int named;       /* 1 once the name request answered or failed for good */
    int name_tries;
    int gamepad;     /* its class (peripheral, joystick or game pad) or name says so */
    int kind;        /* BT_KIND_*, from the name */
    int supported;   /* kind is one the stack reads */
};

/* a pad with a stored link key */
struct bt_paired {
    unsigned char addr[6];
    unsigned char key[16];
    int key_type;
    int kind;
    char name[BT_NAME_LENGTH];
};

struct bt_touch {
    int active;
    int x, y;
};

/* what the user interface shows of the connected pad */
struct bt_pad_status {
    int connected;     /* the HID channels are open */
    int streaming;     /* full input reports are arriving */
    int kind;
    unsigned char addr[6];
    char name[BT_NAME_LENGTH];
    struct mac_gamepad_frame frame;
    unsigned extra;    /* MAC_PAD_EXTRA_* */
    struct bt_touch touch[2];
    int touch_width, touch_height;
    unsigned reports;          /* full reports since this connection */
    unsigned simple_reports;
    unsigned crc_errors;       /* full reports whose CRC-32 did not match */
    double rate;               /* full reports a second, over the last second */
    int battery_percent;       /* -1 if unknown */
};

/* what the pad is asked to show: light bar colour, player lights, motors */
#define bt_pad_output mac_pad_output /* (mac_gamepad_report.h: the system Bluetooth path builds the same reports) */

enum {
    BT_DONGLE_NONE,        /* no dongle plugged in */
    BT_DONGLE_FOUND,       /* plugged in, not opened */
    BT_DONGLE_APPLE,       /* Apple's Bluetooth driver has it */
    BT_DONGLE_BUSY,        /* another program has it open */
    BT_DONGLE_OPEN,        /* opened by us, still the ROM */
    BT_DONGLE_READY        /* opened by us, firmware loaded */
};

struct bt_channel {
    int used;
    unsigned psm, local, remote;
    int state;       /* 1 connect sent, 2 configuring, 3 open */
    int cfg_in, cfg_out;
    unsigned mtu_out;
};

struct bt_command {
    unsigned opcode;
    unsigned length;
    unsigned char params[255];
};

struct bt_stack {
    struct bt_transport tr;
    void (*log)(void *ctx, const char *line);
    void *log_ctx;

    /* settings, set before bt_stack_start */
    char firmware_path[512];
    char config_path[512];
    char keys_path[512];

    /* what the stack is doing, in words, for the user interface */
    char message[160];
    char chip[96];

    /* the controller */
    int dongle;
    unsigned hci_version, hci_revision, lmp_subversion;
    int rom_version;
    int firmware_loaded;
    unsigned char local_addr[6];
    unsigned acl_max;
    int acl_credits;
    int ncmd;
    double command_time;       /* when the last command went out */
    int failed;        /* the transport failed */

    /* the command queue */
    struct bt_command queue[BT_COMMAND_QUEUE];
    unsigned queue_head, queue_tail;
    unsigned sync_opcode;
    int sync_wait, sync_done, sync_status;
    unsigned char sync_return[300];
    unsigned sync_length;

    /* discovery */
    struct bt_found found[BT_MAX_FOUND];
    int found_count;
    int inquiry_active;
    int inquiry_cancelling;
    double search_until;       /* 0 when no search is wanted */
    double inquiry_ended;
    int name_pending;          /* index into found, or -1 */
    double name_started;

    /* the stored keys of this dongle */
    struct bt_paired paired[BT_MAX_PAIRED];
    int paired_count;

    /* the link to one pad */
    int link_up;
    unsigned link_handle;
    unsigned char link_addr[6];
    int link_incoming;
    int link_authed, link_encrypted;
    int link_used_stored_key;
    int connecting;            /* Create_Connection sent, no answer yet */
    double connect_started;
    double auth_fallback_time;
    double l2cap_time;
    int link_kind;
    char link_name[BT_NAME_LENGTH];

    /* pairing: the pad to pair with, or none; reconnect: wait for the keys' pads */
    int pairing;
    unsigned char pair_addr[6];
    double pair_deadline;
    int pair_tries;
    int pair_key_saved;
    int reconnect;             /* page scan is on: a pad with a stored key may connect */
    double page_time;          /* when a stored pad was last paged */

    /* L2CAP */
    struct bt_channel channel[BT_MAX_CHANNELS];
    unsigned next_ident;
    unsigned char l2_buf[1100];
    unsigned l2_have, l2_want, l2_cid;

    /* HID */
    int hid_ready;
    double hid_ready_time;
    int feature_sent, feature_got, feature_tries;
    double feature_time;
    unsigned feature_report;   /* 0x05 (DualSense) or 0x02 (DualShock 4) */
    unsigned output_sequence;
    int output_setup_sent;
    int output_initial_sent;
    int output_repeat;
    struct bt_pad_output output;     /* what the pad is to show */
    int output_dirty;
    double output_time;              /* when the pending output may go */
    double rumble_off_time;
    struct bt_pad_status pad;
    double rate_start;
    unsigned rate_count;
    unsigned char last_report[80];
    unsigned last_report_size;
};

/* Allocates and clears a stack. The caller sets the transport, the paths and
   the log. */
struct bt_stack *bt_stack_create(void);
void bt_stack_destroy(struct bt_stack *st);
void bt_log(struct bt_stack *st, const char *format, ...);
void bt_say(struct bt_stack *st, const char *format, ...);

/* Brings the controller up: reset, version, firmware (if it is a ROM
   RTL8761B and a firmware file is set), buffers, address, event mask, name,
   class, Secure Simple Pairing, page scan. Returns 0, or -1 with st->message
   saying why. */
int bt_stack_start(struct bt_stack *st);
/* Handles what arrived for up to this long. */
void bt_stack_pump(struct bt_stack *st, double seconds);
/* Does what is due (searches, names, paging, timeouts, output reports);
   call it every 20 ms or so. */
void bt_stack_tick(struct bt_stack *st);
/* Closes the link and the transport. */
void bt_stack_stop(struct bt_stack *st);

/* user requests (call from the thread that pumps) */
void bt_stack_search(struct bt_stack *st, double seconds);
void bt_stack_search_stop(struct bt_stack *st);
void bt_stack_pair(struct bt_stack *st, const unsigned char *addr);
void bt_stack_forget(struct bt_stack *st, const unsigned char *addr);
void bt_stack_output(struct bt_stack *st, const struct bt_pad_output *output);
void bt_stack_disconnect(struct bt_stack *st);

/* ---- bt_hci.c ---- */

void bt_hci_command(struct bt_stack *st, unsigned opcode, const unsigned char *params, unsigned count);
void bt_hci_flush(struct bt_stack *st);
/* a command that waits for its Command Complete; returns its status, or -1 on a timeout */
int bt_hci_sync(struct bt_stack *st, unsigned opcode, const unsigned char *params, unsigned count,
    unsigned char *ret, unsigned *retlen, double timeout);
void bt_hci_event(struct bt_stack *st, const unsigned char *e, unsigned size);
void bt_hci_tick(struct bt_stack *st);
void bt_hci_link_down(struct bt_stack *st);
int bt_hci_is_paired(const struct bt_stack *st, const unsigned char *addr);
struct bt_found *bt_hci_found_by_addr(struct bt_stack *st, const unsigned char *addr);

/* ---- bt_ssp.c ---- */

/* Handles the events of authentication and pairing; returns 1 if it was one. */
int bt_ssp_event(struct bt_stack *st, const unsigned char *e, unsigned size);
struct bt_paired *bt_ssp_find_key(struct bt_stack *st, const unsigned char *addr);

/* ---- bt_realtek.c ---- */

/* Builds the image a Realtek ROM takes (the patch for this ROM version, the
   firmware's version word, then the config) from the two files' contents.
   Returns the image's length (malloc'd, in *image), or 0 if the file does not
   hold a patch for the ROM. cfg may be NULL. */
unsigned bt_realtek_image(const unsigned char *fw, unsigned fw_size, const unsigned char *cfg,
    unsigned cfg_size, unsigned rom_version, unsigned char **image);
/* Downloads the image with vendor command 0xFC20, in fragments of 252 bytes. */
int bt_realtek_load(struct bt_stack *st, unsigned rom_version);

/* ---- bt_l2cap.c ---- */

struct bt_channel *bt_l2cap_by_psm(struct bt_stack *st, unsigned psm);
void bt_l2cap_connect(struct bt_stack *st, unsigned psm);
void bt_l2cap_send(struct bt_stack *st, unsigned cid, const unsigned char *data, unsigned size);
void bt_l2cap_input_acl(struct bt_stack *st, const unsigned char *p, unsigned size);
void bt_l2cap_reset(struct bt_stack *st);
void bt_l2cap_disconnect_all(struct bt_stack *st);

/* ---- bt_hid.c ---- */

/* called by L2CAP */
void bt_hid_channel_open(struct bt_stack *st, struct bt_channel *c);
void bt_hid_data(struct bt_stack *st, struct bt_channel *c, const unsigned char *d, unsigned size);
void bt_hid_reset(struct bt_stack *st);
void bt_hid_tick(struct bt_stack *st);
/* sends the pending output report now */
void bt_hid_send_output(struct bt_stack *st, int setup_only);

/* ---- bt_pad.c ---- */

/* the kind of pad a Bluetooth name says */
int bt_pad_kind_from_name(const char *name);
const char *bt_pad_kind_name(int kind);
/* Sony's output report 0x31 (DualSense) or 0x11 (DualShock 4) over Bluetooth,
   78 bytes with the CRC-32; returns 78 */
unsigned bt_pad_build_dualsense(unsigned char *r, unsigned sequence, const struct bt_pad_output *o);
unsigned bt_pad_build_dualshock4(unsigned char *r, unsigned sequence, const struct bt_pad_output *o);
/* The input report as the game's decoder (mac_gamepad_report.h) takes it:
   the DualSense's 0x31 is rewritten to the USB layout (every field one byte
   earlier), the DualShock 4's 0x11 and both pads' simple 0x01 as they are.
   Returns 1 and fills frame, extra and the touch points, or 0 if r is not a
   state report of that kind. */
int bt_pad_decode(int kind, const unsigned char *r, unsigned size, struct mac_gamepad_frame *frame,
    unsigned *extra, struct bt_touch touch[2], int *touch_width, int *touch_height, int *battery_percent);
/* whether the CRC-32 in the last four bytes of a full input report is right */
int bt_pad_input_crc_ok(const unsigned char *r, unsigned size);

/* ---- bt_keys.c ---- */

/* Loads the keys of the dongle with this address from the file; returns the count. */
int bt_keys_load(const char *path, const unsigned char *local_addr, struct bt_paired *out, int max);
/* Replaces (or adds) the key for out->addr, keeping other dongles' lines; 0 or -1. */
int bt_keys_store(const char *path, const unsigned char *local_addr, const struct bt_paired *key);
int bt_keys_remove(const char *path, const unsigned char *local_addr, const unsigned char *addr);

/* ---- bt_fake.c (tests) ---- */

/* A dongle that answers like a Realtek RTL8761BU with a DualSense in range
   (pairing mode), for tests. */
struct bt_fake;
struct bt_fake *bt_fake_create(struct bt_stack *st, int rom_only);
void bt_fake_destroy(struct bt_fake *f);
unsigned bt_fake_commands_seen(const struct bt_fake *f, unsigned opcode);
int bt_fake_output_report(const struct bt_fake *f, unsigned char *out, unsigned cap);

#ifdef __cplusplus
}
#endif
#endif
