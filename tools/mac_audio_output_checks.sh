#!/bin/sh
set -eu
out=${MAC_AUDIO_OUT:-build/mac-audio-output}
mkdir -p "$out"
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    gcc-4.0 -arch "$arch" -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror $fp -Iport/mac \
        port/mac/mac_audio_output.c tools/mac_audio_output_probe.c \
        -framework AudioUnit -framework AudioToolbox -framework CoreAudio -framework Carbon \
        -o "$out/audio_output_$arch"
done
"$out/audio_output_i386"
