#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_VOICE_AUDIO_OUT:-build/mac-audit/voice-audio}
mkdir -p "$out"
python tools/mac_voice_audio_view.py "$out/voice_audio_native.c"
for arch in ppc i386; do
	flags="-arch $arch -isysroot $sdk -mmacosx-version-min=10.4"
	if [ "$arch" = ppc ]; then fp=-mno-fused-madd; else fp=; fi
	# Keep the game-facing adapter under the same clean XDK declarations as the
	# game. Opus itself is built from the checked-in portable upstream sources.
	$cc $flags -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar \
		-Dfast_ftol=mac_voice_audio_fast_ftol \
		-I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/linux/src \
		-Iport/linux/include -Iport/include/xdk -Iport/third_party/opus/include \
		-Iport/third_party/opus/celt -Iport/third_party/opus/silk -Iport/third_party/opus/silk/float \
		-include port/linux/include/halo_port_limits.h -include port/mac/engine_xdk_prefix.h \
		-c "$out/voice_audio_native.c" -o "$out/voice_$arch.o"
	$cc $flags -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar \
		-Dfast_ftol=mac_voice_probe_fast_ftol \
		-I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/linux/src \
		-Iport/linux/include -Iport/include/xdk -Iport/third_party/opus/include \
		-include port/linux/include/halo_port_limits.h -include port/mac/engine_xdk_prefix.h \
		-c tools/mac_voice_audio_probe.c -o "$out/probe_$arch.o"
	$cc $flags -std=gnu99 -O2 $fp -fshort-wchar -ffunction-sections -fdata-sections \
		-I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/linux/include -Iport/include/xdk \
		-include port/linux/include/halo_port_limits.h -include port/mac/engine_xdk_prefix.h \
		-c port/mac/engine_clock.c -o "$out/clock_$arch.o"
	: > "$out/opus_$arch.rsp"
	while IFS= read -r source; do
		[ -n "$source" ] || continue
		object="$out/opus_${arch}_$(basename "$source" .c).o"
		$cc $flags -std=gnu99 -O2 $fp -DOPUS_BUILD -DVAR_ARRAYS -DHAVE_LRINTF -DHAVE_LRINT \
			-Iport/third_party/opus/include -Iport/third_party/opus/celt \
			-Iport/third_party/opus/silk -Iport/third_party/opus/silk/float \
			-c "port/third_party/opus/$source" -o "$object"
		echo "$object" >> "$out/opus_$arch.rsp"
	done < port/third_party/opus/sources.txt
	$cc $flags -Wl,-dead_strip "$out/probe_$arch.o" "$out/voice_$arch.o" \
		"$out/clock_$arch.o" \
		$(cat "$out/opus_$arch.rsp") -lpthread -lm -o "$out/probe_$arch"
done
"$out/probe_i386"
