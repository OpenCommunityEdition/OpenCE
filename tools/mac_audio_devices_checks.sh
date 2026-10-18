#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
	fp=
	if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $fp -Iport/mac port/mac/engine_audio_devices.c \
		tools/mac_audio_devices_probe.c -framework CoreAudio -framework CoreFoundation \
		-o "$mac_dir/audio_devices_$arch"
done
"$mac_dir/audio_devices_i386"
