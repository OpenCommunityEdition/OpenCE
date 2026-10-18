#!/bin/sh
# Decodes constructed DualShock 4 and DualSense reports (tools/mac_gamepad_report_probe.c)
# on the build machine, for every architecture it can run. Needs no pad.
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
        tools/mac_gamepad_report_probe.c port/mac/mac_gamepad_report.c port/mac/mac_gamepad.c \
        -framework IOKit -framework CoreFoundation -o "$mac_dir/gamepad_report_$arch"
done
# (a PowerPC binary runs here only under Rosetta, which is not evidence of its byte order)
"$mac_dir/gamepad_report_i386"
