#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -fexceptions -Wall -Wextra -Werror -Iport/mac \
        port/mac/engine_stack.c tools/mac_engine_stack_probe.c -o "$mac_dir/engine_stack_$arch"
done
"$mac_dir/engine_stack_i386"
