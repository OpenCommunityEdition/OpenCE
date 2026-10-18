#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
tick_out=${MAC_GAME_TICK_OUT:-build/mac-game-tick-cycle13}
out=${MAC_DSOUND_PACKET_OUT:-build/mac-audit/dsound-packet}
mkdir -p "$out"
for arch in ppc i386; do
	fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
	test -f "$tick_out/mac_dsound_native_$arch.o"
	test -f "$tick_out/engine_objects_$arch.txt"
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $fp -fms-extensions -fshort-wchar \
		-I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/linux/include \
		-Iport/include/xdk -idirafter port/linux/include \
		-include port/linux/include/halo_port_limits.h \
		-include port/linux/include/halo_port_capacity.h \
		-include port/mac/engine_xdk_prefix.h \
		-c tools/mac_dsound_packet_probe.c -o "$out/probe_$arch.o"
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -ffunction-sections -fdata-sections \
		-I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
		-Isource/memory -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
		-include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
		-include port/mac/engine_xdk_prefix.h -include build/mac-audit/mac_spawn_witness_names.h \
		-include port/include/halo_math.h -include port/linux/include/halo_linux_source_fixups.h \
		-include build/mac-audit/mac_engine_semantics.h -c port/mac/engine_services.c \
		-o "$out/services_$arch.o"
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $fp -Wl,-dead_strip "$out/probe_$arch.o" \
		"$tick_out/mac_dsound_native_$arch.o" "$tick_out/engine_handles_$arch.o" \
		"$out/services_$arch.o" "$tick_out/mac_audio_pcm_$arch.o" \
		-lpthread -o "$out/probe_$arch"
done
"$out/probe_i386"
