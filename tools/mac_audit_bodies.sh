#!/bin/sh
# Make every generated body the Mac session links from the current sources.
#
# The session's units include views of the game's own files and of the shared platform layer, cut by
# generators (tools/mac_*_boundary.py, tools/mac_*_view.py): game.c with its tick, the menu activity
# accessors, text_hires.c, the frame statistics counters, the bitmap sampler, the shared texture layout and
# so on. A body made once and left in build/mac-audit goes out of date when its source changes (the merge of
# upstream brought text_hires_enabled; the achievements added three lines to game.c) and the link then picks
# up a function nothing defines, a stub in its place, or an older copy that loses the new feature without a
# word. This writes every one again, each build. The generators run on Python 2.6 (c2d, the G4s) and 3 and
# take a few tenths of a second between them.
#
# One body cannot be made on c2d: mac_engine_semantics.h comes from tools/linux_msvc_semantics.py, which needs
# Python 3. It is written when this runs under Python 3, together with a stamp (a checksum of the headers under
# source/ and of the two scripts); under Python 2 the stamp is checked, and a header that no longer matches
# stops the build with the command that writes it.
#
# usage: mac_audit_bodies.sh AUDIT_DIR       (MAC_PYTHON names the interpreter; default python, else python3)
set -eu
audit=${1:?usage: mac_audit_bodies.sh AUDIT_DIR}
py=${MAC_PYTHON:-}
if [ -z "$py" ]; then
    if command -v python > /dev/null 2>&1; then py=python; else py=python3; fi
fi
mkdir -p "$audit"

# Write each body beside its place and move it over only when it differs, so a body that did not change keeps
# its time stamp (make-like tools and the graph refresh see it unchanged).
tmp="$audit/.bodies.$$"
mkdir -p "$tmp"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

$py tools/mac_game_state_boundary.py "$tmp/mac_game_state_body.h"
$py tools/mac_menu_state_boundary.py "$tmp/mac_menu_state_body.h"
$py tools/mac_text_boundary.py "$tmp"
$py tools/mac_frame_statistics_boundary.py "$tmp/mac_frame_statistics_body.h"
$py tools/mac_bitmap_sampling_boundary.py "$tmp/mac_engine_bitmaps_body.h"
$py tools/mac_resource_boundary.py "$tmp"
$py tools/mac_resource_release_body.py "$tmp/mac_resource_release_body.inc"
$py tools/mac_scenario_scripts_boundary.py "$tmp/mac_scenario_script_validators.h"
$py tools/mac_rasterizer_default_pixels_view.py "$tmp/mac_rasterizer_xbox_default_pixels.c" > /dev/null
$py tools/mac_device_state_view.py "$tmp/mac_device_state_body.inc" "$tmp/mac_device_state_defaults.inc" > /dev/null
# the names the spawn link checker (tools/mac_spawn_link_checks.sh) wrote, which do not depend on a source
cat > "$tmp/mac_spawn_witness_names.h" <<'NAMES'
#define MAC_SPAWN_JOIN_INNER(a,b) a##b
#define MAC_SPAWN_JOIN(a,b) MAC_SPAWN_JOIN_INNER(a,b)
#define verify_render_model_effect_size MAC_SPAWN_JOIN(mac_effect_witness_,__LINE__)
#define verify_rasterizer_model_begin_parameters_size MAC_SPAWN_JOIN(mac_begin_witness_,__LINE__)
#define effect_definition_size_assert MAC_SPAWN_JOIN(mac_effect_definition_witness_,__LINE__)
#define effect_event_definition_size_assert MAC_SPAWN_JOIN(mac_effect_event_witness_,__LINE__)
NAMES

# the semantics header: made under Python 3, checked under Python 2
semantics_inputs() {
    {
        find source -name '*.h' | sort | while IFS= read -r header; do
            printf '%s\n' "$header"
            cat "$header"
        done
        cat tools/mac_engine_semantics.py tools/linux_msvc_semantics.py
    } | shasum -a 256 | cut -d' ' -f1
}
semantics_stamp=$(semantics_inputs)
if $py -c 'import sys; sys.exit(sys.version_info[0] < 3)' 2> /dev/null; then
    $py tools/mac_engine_semantics.py "$tmp/mac_engine_semantics.h"
    printf '%s\n' "$semantics_stamp" > "$tmp/mac_engine_semantics.h.stamp"
elif [ ! -f "$audit/mac_engine_semantics.h" ] || [ "$(cat "$audit/mac_engine_semantics.h.stamp" 2> /dev/null)" != "$semantics_stamp" ]; then
    echo "$audit/mac_engine_semantics.h is out of date (a header under source/ or its generator changed):" >&2
    echo "on a machine with Python 3 run   MAC_PYTHON=python3 sh tools/mac_audit_bodies.sh $audit" >&2
    echo "and copy mac_engine_semantics.h and mac_engine_semantics.h.stamp from there" >&2
    exit 1
fi

changed=
for body in "$tmp"/*; do
    name=$(basename "$body")
    if [ -f "$audit/$name" ] && cmp -s "$body" "$audit/$name"; then continue; fi
    # a link into a shared directory must not be written through
    rm -f "$audit/$name"
    cp "$body" "$audit/$name"
    changed="$changed $name"
done
echo "generated bodies in $audit: ${changed:-all current}"
