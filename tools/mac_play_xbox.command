#!/bin/sh
# Plays from the Xbox disc image's maps alone: the title menu (ui.map), then the
# campaign and multiplayer games from the same folder. No MacSoft Halo.app is used.
# The data root is the folder that holds maps/ (OpenCE Setup extracts it to
# XboxData beside the binary); HALO_MAC_DATA_ROOT overrides it.
set -eu

launcher_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
binary="$launcher_dir/OpenCE-BloodGulch"
data_root=${HALO_MAC_DATA_ROOT:-$launcher_dir/XboxData}
state_root="$HOME/Library/Application Support/OpenCE Xbox"

if [ ! -x "$binary" ]; then
    echo "OpenCE-BloodGulch must be beside this launcher."
    exit 1
fi
if [ ! -f "$data_root/maps/ui.map" ]; then
    echo "Missing $data_root/maps/ui.map (run OpenCE Setup to extract the maps from your Halo disc image)."
    exit 1
fi

mkdir -p "$state_root/saves"
cd "$state_root"
export HALO_MAC_DATA_ROOT="$data_root"
export HALO_MAC_SAVE_ROOT="$state_root/saves"
# the inflated maps are kept beside the maps (HALO_MAC_MAP_CACHE=off turns that off)
export HALO_MAC_MAP_CACHE=${HALO_MAC_MAP_CACHE:-$state_root/map-cache}
export HALO_START_IN_MENU=1
export HALO_MAC_BONE_GATHER=${HALO_MAC_BONE_GATHER:-1}
unset HALO_MAC_RENDER_FRAMES HALO_MAC_RENDER_TRACE HALO_MAC_SESSION_TRACE
unset HALO_MAC_RENDER_FIXED_STEP HALO_MAC_RENDER_SEED
echo "Loading OpenCE with the Halo disc image's maps. The first start of each map unpacks it into $HALO_MAC_MAP_CACHE."
echo "Menus: arrow keys, Return to choose, Backspace to go back. In a game: WASD, mouse look, Escape for the pause menu."
exec "$binary" "$data_root/maps/ui.map" --gpu-play
