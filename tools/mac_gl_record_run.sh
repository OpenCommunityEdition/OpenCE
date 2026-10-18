#!/bin/sh
# Runs the renderer with the GL recorder injected and writes one captured frame.
# usage: mac_gl_record_run.sh run_dir capture.glr [frame]
# Environment: GAME_DIR (required; holds run_bsp.sh and the binary),
#   BINARY (default $GAME_DIR/mac_rendered_session_ppc), DATA_ROOT, MAPS (required; directory of the three maps),
#   MAP_NAMES (bloodgulch bitmaps sounds file names), RECORDER (the dylib).
set -eu
run_dir=$1
capture=$2
frame=${3:-20}
game=${GAME_DIR:?set GAME_DIR to the directory with run_bsp.sh and the binary}
binary=${BINARY:-$game/mac_rendered_session_ppc}
data_root=${DATA_ROOT:-/tmp/opence-session-data}
maps=${MAPS:?set MAPS to the directory of the three maps}
names=${MAP_NAMES:-"bloodgulch-v7.map bitmaps-v7.map sounds-v7.map"}
recorder=${RECORDER:-$(dirname "$0")/gl_record.dylib}
set -- $names
export HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1 HALO_MAC_LAZY_PRESENT=1 HALO_MAC_ARB_VERTEX=all
export HALO_MAC_PROJ_FETCH=1 HALO_MAC_ARB_FRAGMENT=1
# DEFAULT_DETAIL=1: the default detail (no low-preset variables)
if [ -z "${DEFAULT_DETAIL:-}" ]; then
    export HALO_MAC_TEXTURE_SKIP_LEVELS=3 HALO_SPECULAR=false HALO_SHADOWS=false HALO_DECALS=false HALO_PARTICLES=low HALO_SCENERY_MINIMUM_PIXELS=16
fi
export GLREC_OUT="$capture" GLREC_FRAME="$frame" GLREC_EXIT=1
# (only the renderer gets the recorder: a wrapper script sets it, so the shell
# and tools run_bsp.sh starts are left alone)
mkdir -p "$run_dir"
wrapper="$run_dir/inject.sh"
printf '#!/bin/sh\nDYLD_INSERT_LIBRARIES=%s exec %s "$@"\n' "$recorder" "$binary" > "$wrapper"
chmod +x "$wrapper"
binary=$wrapper
cd "$game"
sh run_bsp.sh "$run_dir" "$data_root" "$maps/$1" "$maps/$2" "$maps/$3" "$binary" 0 30 --gpu-render
