#!/bin/sh
# Host-side checks of the NV2A to ARBvp1.0 translator (port/mac/mac_nv2a_arb.c):
# retail vertex shader table entries 31 and 50, random programs against a
# reference interpreter of the GLSL translator's semantics, refusal cases.
# Needs no GPU and no game data (the shader table is in the sources). Run
# from the repository root. MAC_NV2A_ARB_CC selects the compiler (default cc);
# with MAC_NV2A_ARB_LEGACY=1 the translator is also compiled with the legacy
# toolchain (MAC_CC, MAC_SDK as the other Mac checks) for i386 and ppc.
set -eu
cc=${MAC_NV2A_ARB_CC:-cc}
out=${MAC_NV2A_ARB_OUT:-build/mac-audit/nv2a-arb}
mkdir -p "$out"
# no fused multiply-add: the reference must round as the GPU programs do
"$cc" -std=gnu89 -O1 -ffp-contract=off -Wall -Wextra -Wno-deprecated-declarations \
    -Iport/mac tools/mac_nv2a_arb_probe.c port/mac/mac_nv2a_arb.c \
    port/mac/mac_bone_gather.c -lm -o "$out/mac_nv2a_arb_probe"
for seed in 12345 1 2 3; do
    "$out/mac_nv2a_arb_probe" "$seed" >"$out/run-$seed.txt" 2>&1 || {
        cat "$out/run-$seed.txt"; exit 1; }
done
cat "$out/run-12345.txt"
if [ "${MAC_NV2A_ARB_LEGACY:-0}" = 1 ]; then
    legacy=${MAC_CC:-gcc-4.0}
    sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
    for arch in i386 ppc; do
        "$legacy" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 -Wall -Wextra -Iport/mac \
            -c port/mac/mac_nv2a_arb.c -o "$out/mac_nv2a_arb_$arch.o"
    done
fi
