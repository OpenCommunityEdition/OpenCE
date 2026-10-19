#!/bin/sh
# The loading screen fills the window. A hidden run in a window of 1280x800 enters a level through the menus
# (the title map, Campaign, Load Level, as mac_menu_b30_checks.sh does) and writes the loading frames
# (HALO_MAC_LOADING_SHOTS) at the render resolution of the setting given: each must be the window's size, and have
# light in the upper right of the window, not only in the lower left 640x480 (the bug where the loading screen
# was drawn with the game's own picture size, in the corner of a larger framebuffer). Run once with
# mac.loading_indicator on too: its label must be drawn in the middle of the window.
# usage: sh tools/mac_loading_checks.sh SESSION_BINARY DATA_ROOT UI_MAP OUTDIR [RENDER_RESOLUTION [WINDOW]]
#   (RENDER_RESOLUTION native|original|<lines>, default native; WINDOW default 1280x800)
# HALO_MAC_MENU_ART_DIR must name port/assets/menus outside a kit. Prints "checks, N failures".
set -u
binary=$1; root=$2; ui=$3; out=$4; setting=${5:-native}; window=${6:-1280x800}
width=${window%x*}; height=${window#*x}
here=$(CDPATH= cd "$(dirname "$0")" && pwd)
wait8=wait,wait,wait,wait,wait,wait,wait,wait,
long=""; i=0; while [ $i -lt 300 ]; do long="${long}wait,"; i=$((i+1)); done
checks=0; failures=0
check() { checks=$((checks+1)); if [ "$1" != 1 ]; then failures=$((failures+1)); echo "FAIL: $2"; fi; }
run() { # name indicator
	dir="$out/$1"; rm -rf "$dir"; mkdir -p "$dir/save" "$dir/cache"
	( cd "$dir" && env HALO_MAC_MENU=1 HALO_START_IN_MENU=1 HALO_MAC_RENDER_FIXED_STEP=1 HALO_MAC_RENDER_SEED=1835103336 \
	    HALO_MAC_RENDER_FRAMES=${FRAMES:-900} HALO_MAC_DATA_ROOT="$root" HALO_MAC_MAP_CACHE="$dir/cache" \
	    HALO_MAC_SAVE_ROOT="$dir/save" HALO_MAC_CONFIG_DIR="$dir/cfg" HALO_MAC_HIDDEN_WINDOW=$window \
	    HALO_MAC_RENDER_RESOLUTION=$setting HALO_MAC_LOADING_INDICATOR=$2 HALO_MAC_LOADING_SHOTS="$dir/shot:5" \
	    HALO_MAC_MENU_KEY_FRAMES=4 \
	    HALO_MAC_MENU_KEYS="${wait8}return,${wait8}return,${wait8}down,down,down,wait,wait,return,${wait8}return,${long}end" \
	    "$binary" "$ui" --gpu-render > "$dir/out.txt" 2>&1 )
	check $(grep -a -q "loading: screen up" "$dir/out.txt" && echo 1 || echo 0) "$1: the loading screen comes up"
	check $(ls "$dir"/shot-*.ppm >/dev/null 2>&1 && echo 1 || echo 0) "$1: loading frames were written"
	check $(grep -a -q " 0 failures" "$dir/out.txt" && echo 1 || echo 0) "$1: the session's checks pass"
	for shot in "$dir"/shot-*.ppm; do
		[ -f "$shot" ] || continue
		size=$(head -c 20 "$shot" | sed -n 2p)
		check $([ "$size" = "$width $height" ] && echo 1 || echo 0) "$1: $(basename "$shot") is the window's size ($size)"
	done
}
run plain 0
run indicator 1
# light in the upper right of the window (the band lights the whole height; a picture in the corner leaves it black),
# and, with the indicator, in the middle
for name in plain indicator; do
	last=$(ls "$out/$name"/shot-*.ppm 2>/dev/null | tail -1)
	[ -n "$last" ] || continue
	python -E "$here/mac_loading_checks.py" "$last" > "$out/$name/analysis.txt" 2>&1
	upper=$(sed -n 's/^upper //p' "$out/$name/analysis.txt"); middle=$(sed -n 's/^middle //p' "$out/$name/analysis.txt")
	check $([ "${upper:-0}" -gt 0 ] && echo 1 || echo 0) "$name: the upper right of the window is lit (${upper:-none})"
	[ "$name" = indicator ] && check $([ "${middle:-0}" -gt 50 ] && echo 1 || echo 0) "indicator: the label is in the middle (${middle:-none})"
done
echo "loading screen: $checks checks, $failures failures"
[ $failures = 0 ]
