/* The game's view of a Bluetooth pad (port/mac/mac_bt_input.c through mac_gamepad_poll_all): turns
   input.bluetooth on, polls the pads as the game does every 33 ms for seconds (default 60), and prints
   the dongle's and the pad's state when it changes, the slot the pad takes and its buttons, sticks and
   triggers. The large motor rumbles with the right trigger, the small one with the left, so the game's
   rumble path (mac_gamepad_rumble) can be felt. Pair the pad in Controller Setup first; then press PS.
   Quit Controller Setup's window first, or the dongle is busy (the probe says so and tries again).
   Usage: mac_bt_input_probe [seconds]
   Build: tools/mac_bt_input_checks.sh */
#include "mac_bt_input.h"
#include "mac_gamepad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

/* the settings the game's table would hold */
int config_boolean(const char *name) { return strcmp(name, "input.bluetooth") == 0; }
const char *config_string(const char *name) { (void)name; return ""; }
unsigned long config_changes(void) { return 1; }

static double now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + tv.tv_usec / 1e6;
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 60.0;
    double start = now(), last_rumble = 0;
    struct mac_gamepad_frame frames[MAC_GAMEPAD_MAXIMUM_DEVICES], previous[MAC_GAMEPAD_MAXIMUM_DEVICES];
    int slot, ever = 0;
    memset(previous, 0, sizeof(previous));
    if (seconds <= 0) seconds = 60.0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("polling for %.0f seconds; press PS on a paired controller\n", seconds);
    while (now() - start < seconds) {
        mac_gamepad_poll_all(frames);
        for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot) {
            const struct mac_gamepad_frame *f = &frames[slot];
            if (f->connected != previous[slot].connected)
                printf("%6.2f slot %d: %s (%s, playstation=%d)\n", now() - start, slot + 1,
                    f->connected ? "connected" : "gone", mac_gamepad_name_at(slot), mac_gamepad_is_playstation_at(slot));
            else if (f->connected && memcmp(f, &previous[slot], sizeof(*f)) != 0)
                printf("%6.2f slot %d: buttons %02x A%d B%d X%d Y%d W%d K%d LT%3d RT%3d L(%6d,%6d) R(%6d,%6d)\n", now() - start,
                    slot + 1, f->buttons, f->analog[MAC_GAMEPAD_A] != 0, f->analog[MAC_GAMEPAD_B] != 0,
                    f->analog[MAC_GAMEPAD_X] != 0, f->analog[MAC_GAMEPAD_Y] != 0, f->analog[MAC_GAMEPAD_WHITE] != 0,
                    f->analog[MAC_GAMEPAD_BLACK] != 0, f->analog[MAC_GAMEPAD_LEFT_TRIGGER], f->analog[MAC_GAMEPAD_RIGHT_TRIGGER],
                    f->left_x, f->left_y, f->right_x, f->right_y);
            if (f->connected) {
                ever = 1;
                if (now() - last_rumble >= 0.05) {
                    mac_gamepad_rumble(slot, f->analog[MAC_GAMEPAD_RIGHT_TRIGGER], f->analog[MAC_GAMEPAD_LEFT_TRIGGER]);
                    last_rumble = now();
                }
            }
            previous[slot] = *f;
        }
        usleep(33000);
    }
    mac_gamepad_stop();
    printf("%s\n", ever ? "a pad was connected" : "no pad connected");
    return 0;
}
