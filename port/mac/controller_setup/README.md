# OpenCE Controller Setup

A small native Mac app (Cocoa, IOKit HID Manager, Carbon's text measuring;
no nibs, no garbage collection) that PowerSpark opens on first run (and from
its Controller Setup button) to explain how to connect a modern game
controller with the 8BitDo Wireless USB Adapter 2, to show whether the pad's
input arrives, and to calibrate it. Mac OS X 10.5 PPC and 10.6 Intel; one
universal (ppc + i386) binary.

## What it does

- Instructions (plug in the adapter, with a USB data cable if possible: a
  charge-only cable is not seen at all; its pair button, the controller's own
  pairing combination, Dinput mode (Create + D-pad Left for 3 seconds) as the
  best on these Macs, a solid light means connected; the adapter detaches and
  attaches again by itself while it pairs; firmware update notes; wired USB
  pads and the Xbox 360 pad driver are not covered).
- A live status line: "No input received yet: press a button on the
  controller.", "Receiving input", and after 4 quiet seconds "Controller found
  but sending nothing: make sure it's paired (LED solid) and press a button."
- While only the adapter's waiting identity (2DC8:3107 "IDLE", a vendor-defined
  page, not a game pad) is present, the status line says "Adapter found,
  waiting for a controller". A device that goes is dropped and the next game
  pad that appears (the adapter attaches again as 2DC8:3105 in Dinput mode, or
  as a DualShock 4) is chosen at once.
- A DualShock 4 or DualSense (including 8BitDo's adapter as one) and the
  adapter in Dinput mode (2DC8:3105) are read from their USB input reports
  with the game's decoder (`../mac_gamepad_report.c`; analog triggers and the
  touchpad click come from the report);
  every other HID pad from its elements.
- Calibrate: live sticks with their dead-zone rings, triggers, d-pad and 12
  button lights; dead zones 0..16000 (9000 is the stock game's); invert look.
- Save writes `input.gamepad_left_dead_zone`, `input.gamepad_right_dead_zone`,
  `input.gamepad_invert_look` and `input.gamepad_setup_done = true` to the
  `--config PATH` file (created if missing; other lines kept; missing keys go
  before the first `[section]`). Skip writes only `input.gamepad_setup_done`.
  Both exit with status 0. Closing the window exits with status 1 and writes
  nothing. Existing values in the file preset the controls.
- `--dump-text` prints the window's texts and control sizes, with the size
  each text needs (`GetThemeTextDimensions`), and marks any that is cut off;
  it opens no window.
- The Bluetooth (experimental) tab, in the same window as the Controllers tab (one window, a tab view), is a test area for pairing and live-testing a
  DualSense or DualShock 4 over PowerSpark's own Bluetooth stack (`../bt/`), through a
  TP-Link UB500 (Realtek RTL8761BU) dongle. It shows the dongle's state (not found,
  found, in use by Apple's Bluetooth, firmware loaded), searches and pairs, lists
  paired pads (press PS to reconnect, Forget), shows the pad live (sticks, analog
  triggers, buttons, touchpad), tests the light bar, player lights and rumble, and
  shows the report rate. The stack starts only when Start is pressed, and stops when the tab is left or the
  window closes. The Realtek firmware (`rtl8761bu_fw.bin`,
  `rtl8761bu_config.bin`) is not shipped: it is looked for in
  `input.bluetooth_firmware_dir`, `~/Library/Application Support/OpenCE/Bluetooth/` and
  `/Library/Application Support/RealtekBTFirmware/`. If Apple's driver takes the dongle,
  `sudo nvram bluetoothHostControllerSwitchBehavior=never` (undo with
  `sudo nvram -d bluetoothHostControllerSwitchBehavior`) and replugging keeps it off.
  Link keys are in `~/Library/Application Support/OpenCE/Bluetooth/keys.txt` (mode 0600),
  the log in `bluetooth.log` beside it.
- `--bt-selftest` runs the stack with no window: against a fake dongle (CRCs, output
  report layout, firmware image, key file, a whole search, pairing and input session),
  or with `--live [--fw DIR] [--search SECONDS] [--pair] [--reconnect SECONDS]
  [--secs N] [--output]` on the real dongle.
- Only one Controller Setup runs at a time. The first holds a lock (`flock` on
  `~/Library/Application Support/OpenCE/controller_setup.lock`, dropped by the system when the
  process ends). One started later, by whatever route (the launcher's button, its first-run hook,
  `--controller-setup`, Finder, `open`), asks the first, by a distributed notification carrying its
  `--config` path, to come to the front (and take the path if it has none), waits for the first to
  finish and quits without a window with status 1, so the launcher carries on only when the first is
  done. `--hold-test` holds the lock for 6 seconds with no window and prints each request to come to the
  front (a test).
- There is no driver and no installer. `--package` is accepted and ignored.

## Files

- `main.m`: the window and the HID manager.
- `controller_setup_config.c/.h`: pure C config read/modify/write.
- `controller_setup_hid.c/.h`: pure C mapping of HID elements to a pad state.
- `test_config.c`: tests of both; `cc -o t test_config.c controller_setup_config.c controller_setup_hid.c && ./t`.
- `bt_panel.m`, `controller_setup_ui.h`: the Bluetooth tab.
- `../bt/`: the Bluetooth stack (see `bt_stack.h`).
- `build.sh`, `Info.plist`.

## Building

On a Mac with Xcode 3.x (gcc-4.2, `/Developer/SDKs/MacOSX10.5.sdk`), run
`./build.sh` in this folder. It runs the tests, then builds
`OpenCE Controller Setup.app` with `-arch ppc -arch i386
-mmacosx-version-min=10.5 -fno-objc-gc`. `CC` and `SDK` can be overridden.

## Launcher

On 10.6 and later:

    open -W -a "OpenCE Controller Setup.app" --args --config <kit>/config.toml

`open --args` needs 10.6, so on 10.5 run the binary inside the bundle
directly (it stays in the foreground until the window ends):

    "<app>/Contents/MacOS/OpenCE Controller Setup" --config <kit>/config.toml

Read `input.gamepad_setup_done` in the config to decide whether to open it
again.
