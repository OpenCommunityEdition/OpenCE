#!/bin/sh
# The microphone for voice chat on the Mac: the ring and resampler's unit tests
# (on the machine running this), and the AUHAL capture built for ppc and i386
# in voice chat's own path.
#   MAC_VOICE_CAPTURE_SECONDS=5  also records that long from the microphone with
#   the i386 build (no window), to $out/capture.f32, and checks the signal.
#   MAC_VOICE_CAPTURE_ARCHS="i386"  builds only those.
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_VOICE_CAPTURE_OUT:-build/mac-audit/voice-capture}
archs=${MAC_VOICE_CAPTURE_ARCHS:-"ppc i386"}
mkdir -p "$out"
# the unit tests: plain C, whichever compiler this machine has
cc -std=gnu89 -O1 -Wall -Wextra -Werror -Iport/mac port/mac/mac_voice_dsp.c tools/mac_voice_dsp_probe.c \
	-lm -lpthread -o "$out/dsp_probe"
"$out/dsp_probe"
python tools/mac_voice_audio_view.py "$out/voice_audio_native.c"
for arch in $archs; do
	flags="-arch $arch -isysroot $sdk -mmacosx-version-min=10.4"
	if [ "$arch" = ppc ]; then fp=-mno-fused-madd; else fp=; fi
	common="-std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
		-Iport/linux/src -Iport/linux/include -Iport/include/xdk -Iport/third_party/opus/include \
		-include port/linux/include/halo_port_limits.h -include port/mac/engine_xdk_prefix.h"
	$cc $flags $common -Dfast_ftol=mac_voice_capture_voice_fast_ftol -c "$out/voice_audio_native.c" -o "$out/voice_$arch.o"
	$cc $flags $common -Dfast_ftol=mac_voice_capture_probe_fast_ftol -c tools/mac_voice_capture_probe.c -o "$out/probe_$arch.o"
	$cc $flags -std=gnu89 -O2 $fp -Wall -Wextra -Werror -Iport/mac -c port/mac/mac_voice_capture.c -o "$out/capture_$arch.o"
	$cc $flags -std=gnu89 -O2 $fp -Wall -Wextra -Werror -Iport/mac -c port/mac/mac_voice_dsp.c -o "$out/dsp_$arch.o"
	$cc $flags -std=gnu99 -O2 $fp -fshort-wchar -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
		-Iport/linux/include -Iport/include/xdk -include port/linux/include/halo_port_limits.h \
		-include port/mac/engine_xdk_prefix.h -c port/mac/engine_clock.c -o "$out/clock_$arch.o"
	# Opus, from the checked-in sources (shared with mac_voice_audio_checks.sh's)
	opus="$out/opus_$arch.rsp"
	: > "$opus"
	while IFS= read -r source; do
		[ -n "$source" ] || continue
		object="$out/opus_${arch}_$(basename "$source" .c).o"
		if [ ! -f "$object" ]; then
			$cc $flags -std=gnu99 -O2 $fp -DOPUS_BUILD -DVAR_ARRAYS -DHAVE_LRINTF -DHAVE_LRINT \
				-Iport/third_party/opus/include -Iport/third_party/opus/celt \
				-Iport/third_party/opus/silk -Iport/third_party/opus/silk/float \
				-c "port/third_party/opus/$source" -o "$object"
		fi
		echo "$object" >> "$opus"
	done < port/third_party/opus/sources.txt
	$cc $flags -std=gnu89 -O2 -c tools/mac_voice_probe_shims.c -o "$out/shims_$arch.o"
	$cc $flags -Wl,-dead_strip "$out/shims_$arch.o" "$out/probe_$arch.o" "$out/voice_$arch.o" "$out/capture_$arch.o" "$out/dsp_$arch.o" \
		"$out/clock_$arch.o" $(cat "$opus") -framework AudioUnit -framework AudioToolbox -framework CoreAudio \
		-framework CoreServices -lpthread -lm -o "$out/capture_probe_$arch"
	echo "built $out/capture_probe_$arch"
done
if [ -n "${MAC_VOICE_CAPTURE_SECONDS:-}" ]; then
	rm -f "$out/capture.f32"
	"$out/capture_probe_i386" "$MAC_VOICE_CAPTURE_SECONDS" "$out/capture.f32" | tee "$out/capture.txt"
	test -s "$out/capture.f32"
	grep -q 'nonzero samples [1-9]' "$out/capture.txt"
fi
