#!/bin/sh
set -eu
out=${MAC_PCM_OUT:-build/mac-audio-pcm}
mkdir -p "$out"
for arch in ppc i386; do
    gcc-4.0 -arch "$arch" -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac \
        port/mac/mac_audio_pcm.c tools/mac_audio_pcm_probe.c -o "$out/pcm_$arch"
done
"$out/pcm_i386"
