#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_PHYSICAL_RANGE_OUT:-build/mac-audit/physical-range}
base=build/mac-audit
mkdir -p "$out"
for arch in ppc i386; do
	fp=; [ "$arch" != ppc ] || fp=-mno-fused-madd
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
		-I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
		-Isource/memory -Isource/input -Isource/game -Isource/objects -Iport/mac \
		-Iport/linux/game -Iport/linux/include -Iport/include/xdk \
		-include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
		-include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h \
		-include "$base/mac_engine_semantics.h" \
		port/mac/engine_physical_memory.c tools/mac_physical_range_probe.c \
		-Wl,-dead_strip -o "$out/probe_$arch"
done
"$out/probe_i386"
