#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XNET_STATUS_OUT:-build/mac-audit/xnet-status}
mkdir -p "$out"
for arch in ppc i386; do
	flags="-arch $arch -isysroot $sdk -mmacosx-version-min=10.4 -std=gnu89 -O2"
	if [ "$arch" = ppc ]; then fp=-mno-fused-madd; else fp=; fi
	$cc $flags $fp \
		-Iport/mac \
		-c port/mac/engine_xnet_status.c -o "$out/status_$arch.o"
	$cc $flags $fp \
		-Iport/mac \
		-c tools/mac_xnet_status_probe.c -o "$out/probe_$arch.o"
	$cc -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -Wl,-dead_strip \
		"$out/status_$arch.o" "$out/probe_$arch.o" -o "$out/probe_$arch"
done
"$out/probe_i386"
