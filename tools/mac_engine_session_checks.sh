#!/bin/sh
# Run two real native v7 session ownership cycles against installed retail
# files using the current broad provider manifest. This builds the PPC and
# i386 variants independently, then executes the native architecture only.
set -eu
if [ "$#" -ne 3 ];then
    echo "usage: $0 bloodgulch.map bitmaps.map sounds.map" >&2
    exit 2
fi
# An absolute path under an installed maps/ directory identifies its data root.
# Keep explicit user overrides; the platform file layer uses this root for
# Windows-style D:\ maps paths created during game_initialize.
if [ -z "${HALO_MAC_DATA_ROOT:-}" ]; then
    map_dir=$(dirname "$1")
    map_leaf=$(basename "$map_dir" | tr '[:upper:]' '[:lower:]')
    if [ "$map_leaf" = maps ]; then
        candidate_root=$(dirname "$map_dir")
        if [ -d "$candidate_root/maps" ] || [ -d "$candidate_root/Maps" ]; then
            HALO_MAC_DATA_ROOT=$candidate_root
            export HALO_MAC_DATA_ROOT
        fi
    fi
fi
# Session startup can initialize saved-game services. Give it an isolated
# per-run root unless the caller deliberately supplied a persistent one.
created_save_root=
if [ -z "${HALO_MAC_SAVE_ROOT:-}" ]; then
    created_save_root=$(mktemp -d "${TMPDIR:-/tmp}/opence-session-save.XXXXXX")
    HALO_MAC_SAVE_ROOT=$created_save_root
    export HALO_MAC_SAVE_ROOT
fi
cleanup_session_save_root() {
    if [ -n "$created_save_root" ]; then rm -rf "$created_save_root"; fi
}
trap cleanup_session_save_root EXIT HUP INT TERM
export MAC_ARCHS=${MAC_ARCHS:-ppc i386}
export MAC_GAME_TICK_PROBE=tools/mac_engine_session_probe.c
export MAC_GAME_TICK_OUT=${MAC_GAME_TICK_OUT:-build/mac-engine-session-tick}
sh tools/mac_game_tick_checks.sh
if [ "${MAC_SESSION_BUILD_ONLY:-0}" = 1 ];then
    exit 0
fi
machine=$(uname -m)
case "$machine" in
    ppc|Power\ Macintosh) arch=ppc ;;
    i386|x86_64) arch=i386 ;;
    *) echo "unsupported native architecture for session run: $machine" >&2; exit 2 ;;
esac
test -x "$MAC_GAME_TICK_OUT/game_tick_link_$arch"
"$MAC_GAME_TICK_OUT/game_tick_link_$arch" "$1" "$2" "$3"
