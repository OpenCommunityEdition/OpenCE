#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_FLOAT_CONTROL_OUT:-build/mac-game-tick}
mkdir -p "$out"
"$mac_cc" -arch i386 -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
    -std=gnu89 -O2 -ffunction-sections -fdata-sections \
    -I. -Isource -Isource/cseries -Isource/math -Isource/physics \
    -idirafter port/linux/include -Iport/mac -Iport/include/xdk \
    -include port/mac/engine_prefix.h \
    tools/mac_float_control_probe.c source/math/real_math.c \
    port/mac/mac_float_control.c -Wl,-dead_strip \
    -o "$out/mac_float_control_probe_i386"
"$out/mac_float_control_probe_i386"
"$mac_cc" -arch ppc -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
    -std=gnu89 -O2 -mno-fused-madd -ffunction-sections -fdata-sections \
    -I. -Isource -Isource/cseries -Isource/math -Isource/physics \
    -idirafter port/linux/include -Iport/mac -Iport/include/xdk \
    -include port/mac/engine_prefix.h \
    tools/mac_float_control_ppc_probe.c source/math/real_math.c \
    port/mac/mac_float_control.c -Wl,-dead_strip \
    -o "$out/mac_float_control_probe_ppc"
if [ "${MAC_RUN_PPC:-0}" = 1 ]; then
    "$out/mac_float_control_probe_ppc"
else
    echo "PPC fenv probe built for G4 execution: $out/mac_float_control_probe_ppc"
fi
