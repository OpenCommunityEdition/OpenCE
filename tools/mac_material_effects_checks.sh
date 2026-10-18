#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_CHECK_DIR:-build/mac-audit/material-effects-endian}
mkdir -p "$out"
python tools/mac_material_effects_view.py "$out/mac_material_effects_native.c" \
    "$out/mac_material_effects_endian_helper.c"
for arch in ppc i386; do
    fp=
    if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    flags="-arch $arch -isysroot $mac_sdk -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections"
    includes="-I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory -Isource/objects -Isource/effects -Isource/game -Isource/input -Isource/networking -Isource/render -Isource/scenario -Isource/sound -Iport/mac -Iport/linux/include -Iport/include/xdk"
    "$mac_cc" $flags $includes -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h \
        -c "$out/mac_material_effects_native.c" -o "$out/material_effects_$arch.o"
    "$mac_cc" $flags $includes -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h \
        -c "$out/mac_material_effects_endian_helper.c" -o "$out/helper_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -c tools/mac_material_effects_endian_probe.c \
        -o "$out/probe_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -Wl,-dead_strip "$out/helper_$arch.o" "$out/probe_$arch.o" \
        -o "$out/material_effects_$arch"
done
"$out/material_effects_i386"
