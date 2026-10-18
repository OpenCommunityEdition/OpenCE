#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
        -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
        -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_prefix.h -include "$mac_dir/mac_engine_semantics.h" \
        port/mac/mac_input.c port/mac/engine_input.c tools/mac_engine_input_probe.c \
        -Wl,-dead_strip -framework Carbon -o "$mac_dir/engine_input_callbacks_$arch"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
        -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
        -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_prefix.h -include "$mac_dir/mac_engine_semantics.h" \
        port/mac/mac_input.c port/mac/engine_input.c tools/mac_engine_input_prepare_probe.c \
        -Wl,-dead_strip -framework Carbon -o "$mac_dir/engine_input_prepare_$arch"
done
"$mac_dir/engine_input_callbacks_i386"
"$mac_dir/engine_input_prepare_i386"
