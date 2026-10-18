#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_RENDER_SERVICES_OUT:-build/mac-audit/render-services}
mkdir -p "$out"
for arch in ppc i386; do
	mac_fp=
	if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar \
		-fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Isource/math \
		-Isource/input -Iport/mac -Iport/linux/include -Iport/include/xdk \
		-include port/linux/include/halo_port_limits.h \
		-include port/linux/include/halo_port_capacity.h \
		port/mac/engine_render_services.c tools/mac_render_services_probe.c \
		-Wl,-dead_strip -o "$out/render_services_$arch"
done
"$out/render_services_i386"
"$out/render_services_ppc"
