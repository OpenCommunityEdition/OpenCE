# OpenCE Controller Setup

A small native Mac app (Cocoa, IOKit HID Manager, Carbon's text measuring;
no nibs, no garbage collection) that PowerSpark opens on first run (and from
its Controller Setup button) to explain how to connect a modern game
controller with the 8BitDo Wireless USB Adapter 2, to show whether the pad's
input arrives, and to calibrate it. Mac OS X 10.5 PPC and 10.6 Intel; one
universal (ppc + i386) binary.

## What it does

- Instructions (plug in the adapter, its pair button, the controller's own
  pairing combination, a solid light means connected; Mac mode and firmware
  update notes; wired USB pads and the Xbox 360 pad driver are not covered).
- A live status line: "No input received yet: press a button on the
  controller.", "Receiving input", and after 4 quiet seconds "Controller found
  but sending nothing: make sure it's paired (LED solid) and press a button."
- A DualShock 4 or DualSense (including 8BitDo's adapter as one) is read from
  its USB input reports with the game's decoder (`../mac_gamepad_report.c`);
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
- There is no driver and no installer. `--package` is accepted and ignored.

## Files

- `main.m`: the window and the HID manager.
- `controller_setup_config.c/.h`: pure C config read/modify/write.
- `controller_setup_hid.c/.h`: pure C mapping of HID elements to a pad state.
- `test_config.c`: tests of both; `cc -o t test_config.c controller_setup_config.c controller_setup_hid.c && ./t`.
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
