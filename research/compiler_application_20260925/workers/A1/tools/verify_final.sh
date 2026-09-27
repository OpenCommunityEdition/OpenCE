#!/usr/bin/env bash
# Full A1 verification of finalA/finalB against the CURRENT HEAD. Writes battery/* and sweeps; read-only on the tree.
set -uo pipefail
WT=/c/halo-worktrees/claude-compiler-application-20260925
cd $WT
A1=research/compiler_application_20260925/workers/A1
R=scratch/campaign/workers/A1/roots
S=scratch/campaign/workers/A1
B=$A1/battery
P=$WT/$A1/patches
mkdir -p $B
HEADREV=$(git rev-parse --short HEAD)
echo "HEAD $HEADREV  $(date '+%Y-%m-%d %H:%M:%S %z')" > $B/00_head.txt
bash $A1/tools/build_roots.sh head npA vTU finalA finalB > /dev/null 2>&1
# 1. gate (revgate = gate.py's EXACT/residual logic from an alternate root; gate.py --source cannot see header patches)
for r in head finalA finalB; do
  python -B research/compiler_application_20260925/workers/W1/tools/revgate.py $R/$r source/game/game_engine --out $S/${r}_ge.obj > $B/01_gate_all_$r.txt
done
# 2. keyed diffs + raw disclosure diff
for r in finalA finalB; do
  python -B scratch/campaign/keyed_diff.py build/base/source/game/game_engine.obj $S/${r}_ge.obj --vs-january source/game/game_engine > $B/02_keyed_diff_$r.txt
  python -B $A1/tools/rawdiff.py build/base/source/game/game_engine.obj $S/${r}_ge.obj > $B/02_rawdiff_$r.txt
done
python -B scratch/campaign/keyed_diff.py $S/finalA_ge.obj $S/finalB_ge.obj > $B/02_keyed_diff_finalA_vs_finalB.txt
# 3. full-board sweeps
python -B $A1/tools/sweep.py $R/head $S/sweep_head --all --json $A1/sweep_control_head.json > $B/03_sweep_control_head_vs_build_base.txt 2>&1
for r in finalA finalB; do
  python -B $A1/tools/sweep.py $R/$r $S/sweep_$r --all --json $A1/sweep_$r.json > $B/03_sweep_${r}_vs_build_base.txt 2>&1
  python -B $A1/tools/sweep.py $R/$r $S/sweep_${r}_ab --all --against $S/sweep_head > $B/03_sweep_${r}_vs_head_objects.txt 2>&1
done
# 4. consumer census + per-consumer revgate A/B table
python -B $A1/tools/consumers.py $R/finalA game/game_engine.h game/player_control.h interface/hud_messaging.h sound/sound_classes.h interface/hud.h game/players.h --json $A1/consumers_finalA.json > $B/04_consumers_finalA.txt 2>&1
python -B $A1/tools/consumer_table.py $R/head $R/finalA $A1/consumers_finalA.json $A1/sweep_finalA.json $B/04_consumer_table_finalA.md > $B/04_consumer_table_summary.txt 2>&1
# 5. admission battery
for r in finalA finalB; do
  python -B scratch/tools/object_audit.py source/game/game_engine $S/${r}_ge.obj > $B/05_object_audit_$r.txt 2>&1
  python -B $A1/tools/battery_wrap.py pdb_storage source/game/game_engine $S/${r}_ge.obj > $B/05_pdb_storage_$r.txt 2>&1
  python -B $A1/tools/battery_wrap.py surplus_identity source/game/game_engine $S/${r}_ge.obj > $B/05_surplus_identity_$r.txt 2>&1
  python -B scratch/tools/provider_link.py source/game/game_engine $S/${r}_ge.obj > $B/05_provider_link_all_surplus_$r.txt 2>&1
  python -B scratch/tools/provider_link.py source/game/game_engine $S/${r}_ge.obj --baseline=build/base/source/game/game_engine.obj > $B/05_provider_link_new_surplus_$r.txt 2>&1
done
python -B scratch/tools/object_audit.py source/game/game_engine > $B/05_object_audit_production.txt 2>&1
{ echo "== finalA"; python -B scratch/tools/protoscan.py $R/finalA/source/game/game_engine.c; echo "== HEAD"; python -B scratch/tools/protoscan.py $R/head/source/game/game_engine.c; } | sed "s#$R/##g" > $B/05_protoscan.txt
python -B $A1/tools/w3census.py $R/head $R/finalA --all --out $B/05_w3_census_head_vs_finalA.txt > /dev/null 2>&1
python -B $A1/tools/w3census.py $R/head $R/finalB source/game/game_engine --out $B/05_w3_census_head_vs_finalB_game_engine.txt > /dev/null 2>&1
for r in head finalA finalB; do
  echo "== $r"; python -B tools/fake_match_scan.py $R/$r/source/game/game_engine.c $R/$r/source/game/game_engine.h $R/$r/source/game/players.h $R/$r/source/interface/hud.h $R/$r/source/game/player_control.h $R/$r/source/interface/hud_messaging.h $R/$r/source/sound/sound_classes.h $R/$r/source/interface/hud_nav_points.c 2>&1 | sed "s#$R/$r/##g"
done > $B/05_fake_match_scan.txt
echo "done $HEADREV"
# 6. objdiff 3.3.1 mini-project + strict semantic ledger (production scorer binary, frozen)
OD=$S/odproj
mkdir -p $OD
cp build/split/source/game/game_engine.obj $OD/target.obj; cp build/base/source/game/game_engine.obj $OD/prod.obj
cp $S/finalA_ge.obj $OD/finalA.obj; cp $S/finalB_ge.obj $OD/finalB.obj
printf '%s\n' '{"min_version": "2.0.0-beta.5", "build_target": false, "units": [' \
 '{"name": "prod", "target_path": "target.obj", "base_path": "prod.obj"},' \
 '{"name": "finalA", "target_path": "target.obj", "base_path": "finalA.obj"},' \
 '{"name": "finalB", "target_path": "target.obj", "base_path": "finalB.obj"}]}' > $OD/objdiff.json
build/tools/objdiff-cli.exe report generate -p $OD -o $OD/report.json > /dev/null 2>&1
python -B -m tools.audit_semantic_matches --project $OD --report report.json --output semantic_report.json --rejections "$WT/config/semantic_credit_rejections.json" > $OD/semantic_stdout.txt 2>&1
python -B $A1/tools/odsummary.py $OD > $B/06_objdiff_331_and_semantic_ledger.txt
