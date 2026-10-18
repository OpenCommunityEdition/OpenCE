# OpenCE Controller Setup

A small native Mac app (Cocoa, IOKit HID Manager; no nibs, no garbage
collection) that the OpenCE launcher opens on first run to install and
calibrate an Xbox One S Bluetooth controller. Mac OS X 10.5 PPC and 10.6
Intel; one universal (ppc + i386) binary.

The controller driver (`xboxd`, `XboxOnePad.kext`, `XboxOneController-1.0.pkg`)
is a separate project and is not part of this repository. This app only
looks for its files and process, and opens its package with the system
Installer. It never runs `sudo`, loads a kext or handles a password.

## What it does

- Status: driver installed (`/usr/local/bin/xboxd`,
  `/usr/local/share/XboxOnePad/XboxOnePad.kext`,
  `/Library/LaunchDaemons/net.xboxcontroller.xboxd.plist`), `xboxd` running,
  the virtual pad visible (HID VID 045E PID 028E), the raw controller
  (PID 02FD) connected. Checked every 2 seconds.
- Install Driver: opens the package with `/usr/bin/open`, so Installer asks
  for the administrator password. The package is `--package PATH`, or a
  `XboxOneController*.pkg` next to the app.
- Calibrate: live sticks with their dead-zone rings, triggers, d-pad and 12
  button lights; dead zones 0..16000 (9000 is the stock game's); invert look.
- Another HID game pad or joystick is shown too (X/Y, Rx/Ry or Z/Rz, hat,
  buttons) and can be saved.
- Save writes `input.gamepad_left_dead_zone`, `input.gamepad_right_dead_zone`,
  `input.gamepad_invert_look` and `input.gamepad_setup_done = true` to the
  `--config PATH` file (created if missing; other lines kept; missing keys go
  before the first `[section]`). Skip writes only `input.gamepad_setup_done`.
  Both exit with status 0. Closing the window exits with status 1 and writes
  nothing. Existing values in the file preset the controls.

The physical pad must be paired over Bluetooth first (Xbox One S, model 1708).
Never update its firmware.

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

    open -W -a "OpenCE Controller Setup.app" --args --config <kit>/config.toml --package <kit>/Driver/XboxOneController-1.0.pkg

`open --args` needs 10.6, so on 10.5 run the binary inside the bundle
directly (it stays in the foreground until the window ends):

    "<app>/Contents/MacOS/OpenCE Controller Setup" --config <kit>/config.toml --package <kit>/Driver/XboxOneController-1.0.pkg

Read `input.gamepad_setup_done` in the config to decide whether to open it
again.
