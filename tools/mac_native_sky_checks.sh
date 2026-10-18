#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ]; then fp=-mno-fused-madd;fi
    objects=
    for unit in tools/mac_native_sky_probe.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -fcommon \
            -I. -Isource -Isource/cseries -Isource/math -Isource/tag_files -Isource/scenario -Iport/mac \
            -include tools/mac_native_sky_prefix.h \
            -c "$unit" -o "$mac_dir/${name}_native_sky_${arch}.o"
        objects="$objects $mac_dir/${name}_native_sky_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -fno-strict-aliasing -Iport/mac -Wl,-dead_strip \
        $objects -o "$mac_dir/native_sky_$arch"
    if nm -m -u "$mac_dir/native_sky_$arch" | grep 'weak'; then
        echo 'Unresolved weak engine import' >&2;exit 1
    fi
done
"$mac_dir/native_sky_i386" "$@"
