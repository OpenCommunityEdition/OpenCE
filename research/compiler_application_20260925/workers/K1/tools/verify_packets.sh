#!/bin/sh
# Re-verify K1 packets P1/P2/P3 at the current HEAD. Run from the worktree root.
K=research/compiler_application_20260925/workers/K1
O=scratch/campaign/workers/K1
P=$K/packets
C=$K/copies
OUT=$K/packets/VERIFY_$(git rev-parse --short HEAD).txt
{
echo "HEAD $(git rev-parse HEAD)  $(date '+%Y-%m-%d %H:%M:%S %z')"
cmp -s source/ai/ai_communication.c $C/base.c && echo "unit source at HEAD == K1 base copy" || echo "UNIT SOURCE CHANGED AT HEAD"
python -B tools/campaign/gate.py source/ai/ai_communication --all --out $O/vhead.obj > $O/vhead.rows 2>&1
echo "== HEAD gate: $(grep -v '^EXACT' $O/vhead.rows | tr '\n' ' ')"
echo "== keyed_diff build/base -> HEAD compile (freshness):"; python -B scratch/campaign/keyed_diff.py build/base/source/ai/ai_communication.obj $O/vhead.obj --vs-january source/ai/ai_communication | tail -3
W3H=$(python -B $K/tools/w3.py source/ai/ai_communication $C/base.c 2>&1 | head -1)
echo "== HEAD /W3: $W3H"
for n in P1 P2 P3; do
  case $n in P1) pf=P1_polarity.patch;; P2) pf=P1+P2_cumulative.patch;; P3) pf=P1+P2+P3_cumulative.patch;; esac
  echo; echo "################ $n (cumulative patch $pf)"
  git apply --check $P/$pf && echo "git apply --check at HEAD: OK"
  python -B tools/campaign/gate.py source/ai/ai_communication --source $C/$n.c --all --out $O/v$n.obj > $O/v$n.rows 2>&1
  echo "gate --all: $(grep -v '^EXACT' $O/v$n.rows | tr '\n' ' ')"
  diff <(grep -v '_ai_communication_event' $O/vhead.rows | sort) <(grep -v '_ai_communication_event' $O/v$n.rows | sort) >/dev/null && echo "all other gate rows identical to HEAD" || echo "OTHER GATE ROWS CHANGED"
  echo "keyed_diff build/base -> $n:"; python -B scratch/campaign/keyed_diff.py build/base/source/ai/ai_communication.obj $O/v$n.obj --vs-january source/ai/ai_communication | tail -4
  echo "keyed_diff HEAD compile -> $n:"; python -B scratch/campaign/keyed_diff.py $O/vhead.obj $O/v$n.obj --vs-january source/ai/ai_communication | tail -4
  python -B $K/tools/dumpfn.py $O/v$n.obj _ai_communication_event 0 0x3000 > $O/v$n.txt
  python -B $K/tools/normdiff.py $O/jan.txt $O/v$n.txt > $O/nd_v$n.txt 2> $O/nd_v$n.cnt
  echo "event: frame $(sed -n 3p $O/v$n.txt | awk '{print $4}')  normalised $(cat $O/nd_v$n.cnt)  last ret at $(grep '  ret ' $O/v$n.txt | tail -1 | awk '{print $1}') (January 1f58)"
  echo "/W3: $(python -B $K/tools/w3.py source/ai/ai_communication $C/$n.c 2>&1 | head -1)   (HEAD: $W3H)"
  echo "fake_match_scan: $(python -B tools/fake_match_scan.py $C/$n.c 2>&1 | tail -1)"
  echo "relocations: $(python -B $K/tools/relocms.py _ai_communication_event $O/v$n.obj | tail -1 | cut -c1-160)"
  python -B scratch/campaign/review_patch.py source/ai/ai_communication $P/$pf --label K1_$n 2>&1 | grep VERDICT
done
} > $OUT 2>&1
cat $OUT
