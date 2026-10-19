#!/bin/sh
# Builds the Bluetooth input probe (tools/mac_bt_input_probe.c): the game's pad poll with the Bluetooth stack linked
# in. It is built, not run: run it with the dongle plugged in, its firmware in place and a pad paired in Controller
# Setup (usage in the probe's header).
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ${MAC_ARCHS:-ppc i386}; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra $mac_fp -Iport/mac -Iport/linux/src \
        tools/mac_bt_input_probe.c port/mac/mac_bt_input.c port/mac/mac_gamepad.c port/mac/mac_gamepad_report.c \
        port/mac/bt/bt_hci.c port/mac/bt/bt_hid.c port/mac/bt/bt_keys.c port/mac/bt/bt_l2cap.c port/mac/bt/bt_pad.c \
        port/mac/bt/bt_realtek.c port/mac/bt/bt_session.c port/mac/bt/bt_ssp.c port/mac/bt/bt_usb_iokit.c \
        port/mac/bt/bt_util.c -framework IOKit -framework CoreFoundation -o "$mac_dir/bt_input_probe_$arch"
done
