#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XAPI_OUT:-build/mac-xapi}
mkdir -p "$out"
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    common="-arch $arch -isysroot $mac_sdk -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -ffunction-sections -fdata-sections"
    "$mac_cc" $common -Iport/third_party/mbedtls/include -Iport/third_party/mbedtls/library \
        -c port/third_party/mbedtls/library/sha1.c -o "$out/sha1_$arch.o"
    "$mac_cc" $common -DMBEDTLS_PLATFORM_MS_TIME_ALT -Iport/third_party/mbedtls/include -Iport/third_party/mbedtls/library \
        -c port/third_party/mbedtls/library/platform_util.c -o "$out/platform_util_$arch.o"
    "$mac_cc" $common -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
        -Iport/include/xdk -Iport/third_party/mbedtls/include -idirafter port/linux/include \
        -c port/mac/engine_xapi.c -o "$out/engine_xapi_$arch.o"
    "$mac_cc" $common -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
        -Iport/include/xdk -Iport/third_party/mbedtls/include -idirafter port/linux/include \
        port/mac/engine_handles.c port/mac/engine_config.c port/mac/engine_files.c \
        tools/mac_xapi_probe.c "$out/engine_xapi_$arch.o" "$out/sha1_$arch.o" "$out/platform_util_$arch.o" \
        -Wl,-dead_strip -o "$out/probe_$arch"
done
if [ "${MAC_RUN:-i386}" != none ];then
    save_root=$(mktemp -d "${TMPDIR:-/tmp}/mac-xapi.XXXXXX")
    trap 'rm -rf "$save_root"' EXIT HUP INT TERM
    HALO_MAC_SAVE_ROOT="$save_root" "$out/probe_${MAC_RUN:-i386}"
fi
