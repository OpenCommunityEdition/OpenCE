/* The Bluetooth stack's headless checks (bt_selftest.c). */
#ifndef BT_SELFTEST_H
#define BT_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* --bt-selftest [--live] [--fw DIR] [--keys FILE] [--search SECONDS] [--pair] [--reconnect SECONDS]
                 [--secs SECONDS] [--output]
   Without --live: the stack against a fake dongle (any machine). With --live: the real dongle
   (needs the firmware: --fw, or the places the panel looks). Returns 0 if all checks passed. */
int bt_selftest_main(int argc, char **argv);

#ifdef __cplusplus
}
#endif
#endif
