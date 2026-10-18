#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
base=build/mac-audit
out=${MAC_CACHE_VERTEX_OUT:-build/mac-audit/cache-vertex}
mkdir -p "$out"
python tools/mac_device_state_view.py "$base/mac_device_state_body.inc" "$base/mac_device_state_defaults.inc"
python tools/mac_xgpu_texture_decode_view.py "$base/mac_xgpu_texture_decode_view.inc"
python tools/mac_resource_boundary.py
python tools/mac_cache_vertex_view.py source/cache/cache_files_windows.c "$out/mac_cache_vertex_native.c"
for arch in ppc i386; do
	fp=; [ "$arch" != ppc ] || fp=-mno-fused-madd
	objects=
	for unit in port/mac/engine_device_state.c port/mac/mac_gpu_textures.c port/mac/mac_shader_source_cache.c port/mac/mac_bone_gather.c port/mac/mac_gl_visibility.c port/mac/mac_gl_program_cache.c port/mac/mac_render_timing.c port/mac/mac_vertex_upload.c port/mac/mac_index_decode_cache.c port/mac/bsp_decode_cache.c port/mac/engine_physical_memory.c port/mac/engine_cache_resources.c "$out/mac_cache_vertex_native.c" tools/mac_cache_vertex_probe.c; do
		name=$(basename "$unit" .c)
		if [ "$unit" = port/mac/mac_render_timing.c ] || [ "$unit" = port/mac/mac_vertex_upload.c ] || [ "$unit" = port/mac/mac_index_decode_cache.c ] || [ "$unit" = port/mac/bsp_decode_cache.c ] || [ "$unit" = port/mac/mac_bone_gather.c ]; then
			"$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
				-Iport/mac -c "$unit" -o "$out/${name}_$arch.o"
			objects="$objects $out/${name}_$arch.o"
			continue
		fi
		"$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
			-fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections \
			-I"$base" -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
			-Isource/scenario -Isource/structures -I"source/saved games" -Isource/units -Isource/render \
			-Isource/sound -Isource/main -Isource/interface -Isource/rasterizer -Isource/cutscene \
			-Isource/ai -Isource/cache -Isource/camera -Isource/bitmaps -Isource/models -Isource/items \
			-Isource/effects -Isource/memory -Isource/game -Isource/input -Isource/objects \
			-Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
			-include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
			-include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h \
			-include "$base/mac_engine_semantics.h" -c "$unit" -o "$out/${name}_$arch.o"
		objects="$objects $out/${name}_$arch.o"
	done
	"$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
		-fno-strict-aliasing -Iport/mac -Iport/include/xdk -Wl,-dead_strip $objects \
		"$base/shared_d3d_resources_cpu_resources_${arch}.o" \
		"$base/shared_texture_layout_cpu_resources_${arch}.o" \
		"$base/native_resource_memory_cpu_resources_${arch}.o" \
		-o "$out/probe_$arch"
done
ulimit -c 0
for arch in ppc i386; do
	"$out/probe_$arch"
	if "$out/probe_$arch" outside >"$out/outside_$arch.txt" 2>&1; then
		echo 'cache resource operation unexpectedly accepted outside headless device' >&2
		exit 1
	fi
	grep -q 'requires active headless device' "$out/outside_$arch.txt"
	for operation in busy kick; do
		if "$out/probe_$arch" "outside-device-$operation" >"$out/outside-device-$operation-$arch.txt" 2>&1; then
			echo "D3D $operation unexpectedly accepted without headless device" >&2
			exit 1
		fi
		grep -q 'explicit native headless device' "$out/outside-device-$operation-$arch.txt"
	done
done
