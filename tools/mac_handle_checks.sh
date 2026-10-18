#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac -c port/mac/engine_handles.c -o "$mac_dir/engine_handles_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac tools/mac_handle_probe.c "$mac_dir/engine_handles_${arch}.o" -o "$mac_dir/handle_$arch"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -fms-extensions -fshort-wchar -I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/include/xdk -Iport/linux/game -idirafter port/linux/include -c tools/mac_platform_handles_abi.c -o "$mac_dir/platform_handles_abi_${arch}.o"
done
"$mac_dir/handle_i386"
