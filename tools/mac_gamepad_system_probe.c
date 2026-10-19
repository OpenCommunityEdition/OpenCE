/* The game's view of the pads (port/mac/mac_gamepad.c through mac_gamepad_poll_all) for a pad that
   the system's Bluetooth stack owns (a DualSense or DualShock 4 paired in the Bluetooth menu): prints
   the pad's slot and name, the feature-report switch to the full report (the log lines of
   mac_gamepad.c, on stderr), and the state when it changes: buttons, sticks, triggers, battery.
   The large motor rumbles with the right trigger and the small one with the left, which also sets the
   light bar (blue) and the player lights (Bluetooth output report 0x31).
   Usage: mac_gamepad_system_probe [seconds]
   Build: tools/mac_gamepad_system_probe_checks.sh */
#include "mac_gamepad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

static double now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + tv.tv_usec / 1e6;
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 30.0;
    double start = now(), last_rumble = 0;
    struct mac_gamepad_frame frames[MAC_GAMEPAD_MAXIMUM_DEVICES], previous[MAC_GAMEPAD_MAXIMUM_DEVICES];
    int slot;
    memset(previous, 0, sizeof(previous));
    if (seconds <= 0) seconds = 30.0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("polling for %.0f seconds\n", seconds);
    while (now() - start < seconds) {
        mac_gamepad_poll_all(frames);
        for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot) {
            const struct mac_gamepad_frame *f = &frames[slot];
            if (f->connected != previous[slot].connected)
                printf("%6.2f slot %d: %s (%s, playstation=%d)\n", now() - start, slot + 1,
                    f->connected ? "connected" : "gone", mac_gamepad_name_at(slot), mac_gamepad_is_playstation_at(slot));
            else if (f->connected && memcmp(f, &previous[slot], sizeof(*f)) != 0)
                printf("%6.2f slot %d: buttons %02x A%d B%d X%d Y%d W%d K%d LT%3d RT%3d L(%6d,%6d) R(%6d,%6d) battery %d%%\n",
                    now() - start, slot + 1, f->buttons, f->analog[MAC_GAMEPAD_A] != 0, f->analog[MAC_GAMEPAD_B] != 0,
                    f->analog[MAC_GAMEPAD_X] != 0, f->analog[MAC_GAMEPAD_Y] != 0, f->analog[MAC_GAMEPAD_WHITE] != 0,
                    f->analog[MAC_GAMEPAD_BLACK] != 0, f->analog[MAC_GAMEPAD_LEFT_TRIGGER], f->analog[MAC_GAMEPAD_RIGHT_TRIGGER],
                    f->left_x, f->left_y, f->right_x, f->right_y, mac_gamepad_battery_at(slot));
            if (f->connected && now() - last_rumble >= 0.05) {
                mac_gamepad_rumble(slot, f->analog[MAC_GAMEPAD_RIGHT_TRIGGER], f->analog[MAC_GAMEPAD_LEFT_TRIGGER]);
                last_rumble = now();
            }
            previous[slot] = *f;
        }
        usleep(33000);
    }
    for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; ++slot) mac_gamepad_rumble(slot, 0, 0);
    mac_gamepad_stop();
    return 0;
}
