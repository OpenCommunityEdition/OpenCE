/* The HID channels of a pad (bt_stack.h): control (PSM 0x11) and interrupt
   (PSM 0x13). HIDP headers: the high nibble of the first byte is the message
   type (0 HANDSHAKE, 4 GET_REPORT, 0xA DATA); for DATA the low two bits say
   which report type (1 input, 2 output, 3 feature). A Sony pad sends only its
   simple input report 0x01 until the host reads a feature report (DualSense 0x05,
   DualShock 4 0x02) or sends an output report; then it streams its full report
   (0x31, or 0x11) on the interrupt channel. */
#include "bt_stack.h"

#include <string.h>

void bt_hid_reset(struct bt_stack *st)
{
    st->hid_ready = 0;
    st->feature_sent = 0;
    st->feature_got = 0;
    st->feature_tries = 0;
    st->output_setup_sent = 0;
    st->output_initial_sent = 0;
    st->output_repeat = 0;
    st->output_dirty = 0;
    st->rumble_off_time = 0;
    memset(&st->pad, 0, sizeof(st->pad));
    st->pad.battery_percent = -1;
    st->rate_start = 0;
    st->rate_count = 0;
}

static void send_get_feature(struct bt_stack *st)
{
    struct bt_channel *cc = bt_l2cap_by_psm(st, 0x11);
    unsigned char m[4];
    if (!cc || cc->state != 3) return;
    st->feature_tries++;
    if (st->feature_tries == 3) st->feature_report = st->feature_report == 0x05 ? 0x02 : 0x05; /* the other pad */
    m[1] = (unsigned char)st->feature_report;
    if (st->feature_tries == 2) {
        m[0] = 0x43;      /* GET_REPORT, feature, no buffer size */
        bt_l2cap_send(st, cc->remote, m, 2);
    } else {
        m[0] = 0x4b;      /* GET_REPORT, feature, buffer size follows */
        bt_put16(m + 2, st->feature_report == 0x05 ? 64 : 37);
        bt_l2cap_send(st, cc->remote, m, 4);
    }
    bt_log(st, "HID: GET_REPORT(feature 0x%02x) sent (try %d)", st->feature_report, st->feature_tries);
    st->feature_sent = 1;
    st->feature_time = bt_now();
}

void bt_hid_channel_open(struct bt_stack *st, struct bt_channel *c)
{
    struct bt_channel *cc = bt_l2cap_by_psm(st, 0x11), *ic = bt_l2cap_by_psm(st, 0x13);
    (void)c;
    if (cc && cc->state == 3 && !ic) {
        bt_l2cap_connect(st, 0x13);
        return;
    }
    if (cc && ic && cc->state == 3 && ic->state == 3 && !st->hid_ready) {
        st->hid_ready = 1;
        st->hid_ready_time = bt_now();
        st->feature_report = st->link_kind == BT_KIND_DUALSHOCK4 ? 0x02 : 0x05;
        st->pad.connected = 1;
        st->pad.kind = st->link_kind;
        memcpy(st->pad.addr, st->link_addr, 6);
        strncpy(st->pad.name, st->link_name, sizeof(st->pad.name) - 1);
        bt_log(st, "HID: control and interrupt channels are open");
    }
}

/* the pending output report; setup_only sends the DualSense's light bar setup alone */
void bt_hid_send_output(struct bt_stack *st, int setup_only)
{
    unsigned char payload[80];
    struct bt_channel *ic = bt_l2cap_by_psm(st, 0x13);
    struct bt_pad_output o = st->output;
    if (!ic || ic->state != 3) return;
    payload[0] = 0xa2;
    if (setup_only) {
        memset(&o, 0, sizeof(o));
        o.fade_out_setup = 1;
    }
    if (st->pad.kind == BT_KIND_DUALSHOCK4)
        bt_pad_build_dualshock4(payload + 1, st->output_sequence++, &o);
    else
        bt_pad_build_dualsense(payload + 1, st->output_sequence++, &o);
    bt_l2cap_send(st, ic->remote, payload, 79);
    bt_log(st, "HID: output report 0x%02x sent (%s; light %d,%d,%d%s players 0x%02x motors %d/%d)",
        payload[1], setup_only ? "setup" : "state", o.red, o.green, o.blue, o.set_light ? "" : " (unset)",
        o.player_leds, o.strong, o.weak);
}

static void full_report(struct bt_stack *st, const unsigned char *r, unsigned rs)
{
    struct mac_gamepad_frame frame;
    unsigned extra = 0;
    struct bt_touch touch[2];
    int tw = 1920, th = 1080, battery = -1;
    double t = bt_now();
    int kind = r[0] == 0x31 ? BT_KIND_DUALSENSE : BT_KIND_DUALSHOCK4;

    if (!st->pad.streaming) {
        st->pad.streaming = 1;
        st->rate_start = t;
        st->rate_count = 0;
        st->pad.kind = kind;
        st->link_kind = kind;
        if (!st->pad.name[0]) strcpy(st->pad.name, bt_pad_kind_name(kind));
        bt_say(st, "Connected: %s, full reports flowing", bt_pad_kind_name(kind));
    }
    if (!bt_pad_input_crc_ok(r, rs)) st->pad.crc_errors++;
    if (bt_pad_decode(kind, r, rs, &frame, &extra, touch, &tw, &th, &battery)) {
        st->pad.frame = frame;
        st->pad.extra = extra;
        st->pad.touch[0] = touch[0];
        st->pad.touch[1] = touch[1];
        st->pad.touch_width = tw;
        st->pad.touch_height = th;
        if (battery >= 0) st->pad.battery_percent = battery;
    }
    st->pad.reports++;
    st->rate_count++;
    if (t - st->rate_start >= 1.0) {
        st->pad.rate = st->rate_count / (t - st->rate_start);
        st->rate_start = t;
        st->rate_count = 0;
    }
}

void bt_hid_data(struct bt_stack *st, struct bt_channel *c, const unsigned char *d, unsigned size)
{
    if (size < 1) return;
    if (c->psm == 0x11) {
        unsigned type = d[0] >> 4;
        if (type == 0) {
            bt_log(st, "HID control: HANDSHAKE result %u", d[0] & 0x0f);
            if ((d[0] & 0x0f) != 0 && st->feature_sent && !st->feature_got) st->feature_time = 0; /* try again at once */
        } else if (type == 0xa && (d[0] & 3) == 3) {
            st->feature_got = 1;
            bt_log(st, "HID control: feature report 0x%02x (%u bytes)", size > 1 ? d[1] : 0, size - 1);
        }
        return;
    }
    if (c->psm == 0x13) {
        if (d[0] == 0xa1 && size >= 2) {
            const unsigned char *r = d + 1;
            unsigned rs = size - 1;
            if ((r[0] == 0x31 || r[0] == 0x11) && rs >= 12) {
                full_report(st, r, rs);
            } else if (r[0] == 0x01 && rs >= 10) {
                struct mac_gamepad_frame frame;
                unsigned extra = 0;
                struct bt_touch touch[2];
                int tw, th, battery;
                st->pad.simple_reports++;
                if (!st->pad.streaming && bt_pad_decode(st->link_kind, r, rs, &frame, &extra, touch, &tw, &th, &battery)) {
                    st->pad.frame = frame;
                    st->pad.extra = extra;
                }
            }
        }
    }
}

void bt_hid_tick(struct bt_stack *st)
{
    double t = bt_now();
    if (!st->hid_ready) return;
    if (!st->feature_sent) send_get_feature(st);
    else if (!st->pad.streaming && t - st->feature_time > 1.5 && st->feature_tries < 5) send_get_feature(st);
    if (!st->pad.streaming) return;

    /* the first full report: show that the host has the pad (light and player light) */
    if (!st->output_initial_sent) {
        st->output_initial_sent = 1;
        if (!st->output.set_light && st->output.red == 0 && st->output.green == 0 && st->output.blue == 0) {
            st->output.green = 160;
            st->output.blue = 64;
        }
        st->output.set_light = 1;
        if (!st->output.player_leds) st->output.player_leds = 0x04;
        st->output.set_players = 1;
        st->output_dirty = 1;
    }
    if (st->rumble_off_time && t >= st->rumble_off_time) {
        st->rumble_off_time = 0;
        st->output.strong = st->output.weak = 0;
        st->output.set_rumble = 1;
        st->output_dirty = 1;
    }
    if (st->output_dirty && t >= st->output_time) {
        if (st->pad.kind == BT_KIND_DUALSENSE && !st->output_setup_sent) {
            /* the pad's own light first fades out, alone; the state follows */
            st->output_setup_sent = 1;
            bt_hid_send_output(st, 1);
            st->output_time = t + 0.08;
            return;
        }
        bt_hid_send_output(st, 0);
        st->output.set_light = st->output.set_players = st->output.set_rumble = 0;
        st->output_dirty = 0;
    }
}
