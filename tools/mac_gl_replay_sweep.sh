#!/bin/sh
# Replays a capture with each ablation in turn (one job), writing everything to one log.
# usage: mac_gl_replay_sweep.sh gl_replay capture.glr log frames [set]
#   set 1 (default): the single ablations;  set 2: render scale, and the CPU cost under a 1x1 scissor;  set 3: repeats;
#   set 4: smaller textures (-L), sort by texture
set -u
replay=$1
capture=$2
log=$3
frames=${4:-100}
set=${5:-1}
: > "$log"
run() {
    echo "### -A '$1' -m $2" >> "$log"
    "$replay" "$capture" -n "$frames" -A "$1" -m "$2" -v $3 >> "$log" 2>&1
    echo "### exit $?" >> "$log"
}
echo "### host $(hostname) $(date)" >> "$log"
case "$set" in
1)
    run "" PFTV ""
    for a in a b c d e f F g G h x z; do run "$a" PF -q; done
    for a in ac acb acbd aceF acbeF; do run "$a" PF -q; done
    run "" PFT -q ;;
2)
    run "" PFV -q
    for a in s S d da db dc dab dac dbc dabc dx de dF df dh dg dG dabceF; do run "$a" PF -q; done ;;
3)
    for a in "" e f F g G "" e f F g G; do run "$a" PF -q; done ;;
4)
    for l in 1 2 3; do run "" PF "-q -L $l"; run d PF "-q -L $l"; done
    for a in t dt B dB dtac dbt; do run "$a" PF -q; done ;;
esac
echo "### done $(date)" >> "$log"
