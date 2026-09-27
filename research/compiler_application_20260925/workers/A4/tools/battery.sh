#!/bin/bash
# battery.sh <unit> <candidate.obj> <candidate.c> <split_root> <root_for_w3> <label>
U=$1; OBJ=$2; SRC=$3; SPLIT=$4; ROOT=$5; L=$6
W=research/compiler_application_20260925/workers/A4
OUT=$W/logs/battery_${L}.txt
{
echo "### battery $L  $(date '+%Y-%m-%d %H:%M:%S %z')  unit=$U obj=$OBJ split=$SPLIT"
echo "## gate (per-function strict, vs $SPLIT)"; python -B $W/tools/gate_obj.py $U $OBJ $SPLIT
echo "## object_audit"; python -B $W/tools/object_audit_cand.py $U $OBJ $SPLIT
echo "## pdb_storage"; python -B $W/tools/pdb_storage_cand.py $U $OBJ $SPLIT/$U.obj
echo "## surplus identity (all candidate-only external definitions, code+data)"; python -B $W/tools/surplus_all.py $U $OBJ $SPLIT/$U.obj
echo "## surplus_identity (scratch/tools logic, .text only)"; python -B $W/tools/surplus_identity_cand.py $U $OBJ $SPLIT/$U.obj
echo "## provider_link (both orders)"; python -B scratch/tools/provider_link.py $U $OBJ
echo "## protoscan"; python -B scratch/tools/protoscan.py $SRC
echo "## /W3 /Zs"; python -B $W/tools/w3root.py $ROOT $U --show
echo "## fake_match_scan"; python -B tools/fake_match_scan.py $SRC
} > $OUT 2>&1
tail -3 $OUT
