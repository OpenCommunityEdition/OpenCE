#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_FILE_CHECK_DIR:-build/mac-files}
mkdir -p "$mac_dir"
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -ffunction-sections -fdata-sections $fp \
        -I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/include/xdk -Iport/linux/game -idirafter port/linux/include \
        port/mac/engine_handles.c port/mac/engine_config.c port/mac/engine_files.c tools/mac_file_probe.c \
        -Wl,-dead_strip -o "$mac_dir/mac_file_probe_$arch"
done
if [ "${MAC_RUN:-i386}" != none ];then
    run_dir=$(mktemp -d "${TMPDIR:-/tmp}/mac-file-service.XXXXXX")
    trap 'rm -rf "$run_dir"' EXIT HUP INT TERM
    HALO_MAC_SAVE_ROOT="$run_dir" "$mac_dir/mac_file_probe_${MAC_RUN:-i386}"
fi
