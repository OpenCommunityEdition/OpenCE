#!/bin/sh
# usage: measure.sh <name>   (copy at workers/K1/copies/<name>.c; run from worktree root)
K=research/compiler_application_20260925/workers/K1
O=scratch/campaign/workers/K1
n="$1"
python -B tools/campaign/gate.py source/ai/ai_communication --source $K/copies/$n.c --all --out $O/$n.obj > $O/$n.rows 2>&1
grep -v '^EXACT' $O/$n.rows
python -B $K/tools/dumpfn.py $O/$n.obj _ai_communication_event 0 0x3000 > $O/$n.txt
python -B $K/tools/normdiff.py $O/jan.txt $O/$n.txt > $O/nd_$n.txt 2> $O/nd_$n.cnt
echo "frame: $(sed -n 3p $O/$n.txt | tr -s ' ')  $(cat $O/nd_$n.cnt)  ret-at: $(grep '  ret ' $O/$n.txt | tail -1 | awk '{print $1}')"
# every other row must be unchanged vs head
diff <(grep -v '_ai_communication_event' $O/head.rows | sort) <(grep -v '_ai_communication_event' $O/$n.rows | sort) > /dev/null && echo "other rows: identical to head" || { echo "OTHER ROWS CHANGED:"; diff <(grep -v '_ai_communication_event' $O/head.rows | sort) <(grep -v '_ai_communication_event' $O/$n.rows | sort); }
