#!/bin/sh
# Builds the system Bluetooth pad probe (tools/mac_gamepad_system_probe.c) for the Mac. It is
# built, not run: run it with a pad paired in the Bluetooth menu (usage in the probe's header).
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ${MAC_ARCHS:-ppc i386}; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra $mac_fp -Iport/mac \
        tools/mac_gamepad_system_probe.c port/mac/mac_gamepad.c port/mac/mac_gamepad_report.c \
        -framework IOKit -framework CoreFoundation -o "$mac_dir/gamepad_system_probe_$arch"
done
