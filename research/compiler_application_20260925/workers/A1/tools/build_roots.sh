#!/usr/bin/env bash
# Reproducible A1 alternate roots from the CURRENT HEAD. Usage: build_roots.sh <name>...
#   head   : git archive HEAD source (+ xbox junction)
#   npA/npB: head + fifty-objects pick_game_engine variant_no_single_consumer_headers (01A alias / 01B view copy) + 02..05
#   vTU    : npA + nav-point trio in the genuine hud.h (spec_vTU)
#   finalA : vTU + R1R2 + R3A + R4 + R5 + R6 + R8 + Q3D + R9 + R10 + R12 + R11 + notes   (Q1 = alias)
#   finalB : finalA + spec_viewcopy                                                       (Q1 = /Od view copy)
set -euo pipefail
WT=/c/halo-worktrees/claude-compiler-application-20260925
A1=$WT/research/compiler_application_20260925/workers/A1
R=$WT/scratch/campaign/workers/A1/roots
NP=$WT/research/fifty_objects_20260925/w/owner_queue/pick_game_engine/patches/variant_no_single_consumer_headers
export GIT_CEILING_DIRECTORIES=$R
py() { python -B "$@"; }
chain="R1R2 R3A R4 R5 R6 R8 Q3D R9 R10 R12 R11 R13a R13b R14a R14b notes"
for name in "$@"; do
  case $name in
    head) py $A1/tools/mkroot.py export $R/head ;;
    npA)  py $A1/tools/mkroot.py clone $R/head $R/npA
          (cd $R/npA && git apply $NP/01A_F01R_NP_game_engine_c_alias.patch $NP/02_F02_NP_hud_nav_points_h.patch \
             $NP/03_F03aR_shared_header_game_engine_h.patch $NP/04_F03b_shared_header_prototypes.patch \
             $NP/05_F04_hud_nav_points_owner_include.patch) ;;
    npB)  py $A1/tools/mkroot.py clone $R/head $R/npB
          (cd $R/npB && git apply $NP/01B_F01RB_NP_game_engine_c_view_copy.patch $NP/02_F02_NP_hud_nav_points_h.patch \
             $NP/03_F03aR_shared_header_game_engine_h.patch $NP/04_F03b_shared_header_prototypes.patch \
             $NP/05_F04_hud_nav_points_owner_include.patch) ;;
    vTU)  py $A1/tools/mkroot.py clone $R/npA $R/vTU
          py $A1/tools/edit.py $R/vTU $A1/tools/spec_vTU.py ;;
    finalA) py $A1/tools/mkroot.py clone $R/vTU $R/finalA
          for s in $chain; do py $A1/tools/edit.py $R/finalA $A1/tools/spec_$s.py > /dev/null; done ;;
    finalB) py $A1/tools/mkroot.py clone $R/finalA $R/finalB
          py $A1/tools/edit.py $R/finalB $A1/tools/spec_viewcopy.py ;;
    *) echo "unknown root $name"; exit 1 ;;
  esac
  echo "built $name"
done
