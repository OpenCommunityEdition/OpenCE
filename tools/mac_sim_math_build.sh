#!/bin/sh
# Builds tools/mac_sim_math_probe.c with the game's math units (and the
# vendored musl maths) for i386 and ppc: MAC_SIM_MATH_OUT=build/mac-sim-math
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_SIM_MATH_OUT:-build/mac-sim-math}
mkdir -p "$out"
for arch in ${MAC_ARCHS:-i386 ppc}; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    for unit in port/third_party/musl-math/src/*.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu99 -O2 $fp -w -Iport/third_party/musl-math/include \
            -include port/third_party/musl-math/include/libm.h \
            -c "$unit" -o "$out/${name}_$arch.o"
        objects="$objects $out/${name}_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -c tools/mac_sim_math_stubs.c -o "$out/stubs_$arch.o"
    objects="$objects $out/stubs_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -DHALO_MAC -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
        -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/game -Isource/memory -Isource/main -Isource/shell -Isource/tag_files -Isource/objects -Isource/units -Isource/items -Isource/scenario -Isource/structures -Isource/networking -Isource/cache -Isource/ai -Isource/interface -Isource/render -Isource/sound -Isource/effects -Isource/models -Isource/text -Isource/input -Isource/camera -Isource/bitmaps -Isource/editor -Isource/rasterizer -Isource/rasterizer/xbox -Isource/cutscene -Isource/shaders -Isource/hs -Isource/bink "-Isource/saved games" \
        -Iport/mac -Iport/linux/game -Iport/linux/src -idirafter port/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h \
        -include port/include/halo_math.h \
        -include port/linux/include/halo_linux_source_fixups.h \
        -include "${MAC_RENDER_AUDIT_BASE:-build/mac-audit}/mac_engine_semantics.h" \
        ${MAC_SIM_MATH_EXTRA:-} \
        tools/mac_sim_math_probe.c source/math/real_math.c source/math/matrix_math.c \
        source/math/random_math.c source/math/geometry.c source/math/periodic_functions.c port/mac/mac_float_control.c \
        $objects -Wl,-undefined,dynamic_lookup -o "$out/mac_sim_math_probe_$arch"
done
