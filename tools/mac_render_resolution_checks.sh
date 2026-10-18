#!/bin/sh
# A hidden run of the session binary in a window of 1280x800 (the MacBook's panel) at native, at
# 480 lines and at 600: the log names the size drawn, the present capture is the window's whole
# picture (HALO_MAC_PRESENT_CAPTURE), and the session's checks all pass.
# usage: mac_render_resolution_checks.sh SESSION_BINARY DATA_ROOT MAP BITMAPS SOUNDS [OUT_DIR]
set -eu
bin=$1; root=$2; map=$3; bitmaps=$4; sounds=$5; out=${6:-/tmp/render-resolution-checks}
failures=0
mkdir -p "$out"
run() { # name setting expected-pixels
	name=$1; setting=$2; expect=$3
	dir="$out/$name"; rm -rf "$dir"; mkdir -p "$dir/save"
	HALO_MAC_HIDDEN_WINDOW=1280x800 HALO_MAC_RENDER_RESOLUTION=$setting \
	HALO_MAC_RENDER_FIXED_STEP=1 HALO_MAC_RENDER_SEED=1835103336 HALO_MAC_RENDER_FRAMES=30 \
	HALO_MAC_DATA_ROOT="$root" HALO_MAC_SAVE_ROOT="$dir/save" HALO_MAC_CONFIG_DIR="$dir/cfg" \
	HALO_MAC_PRESENT_CAPTURE="$dir/present.ppm" \
		"$bin" "$map" "$bitmaps" "$sounds" --gpu-render > "$dir/out.txt" 2>&1 || true
	if grep -q "render resolution: $expect pixels for the 768x480 picture" "$dir/out.txt" &&
	   grep -q " 0 failures" "$dir/out.txt" &&
	   [ "$(head -c 20 "$dir/present.ppm" | sed -n 2p)" = "1280 800" ]; then
		echo "ok   $name: $expect drawn, 1280x800 presented"
	else
		echo "FAIL $name: expected $expect (see $dir/out.txt)"; failures=$((failures + 1))
	fi
}
run native native 1280x800
run original original 768x480
run lines600 600 960x600
echo "$failures failures"
[ "$failures" = 0 ]
