#!/bin/sh
# usage: eval.sh <name>   (copy at workers/K2/copies/<name>.c; object at scratch/campaign/workers/K2/<name>.obj)
W=/c/halo-worktrees/claude-compiler-application-20260925
K=$W/research/compiler_application_20260925/workers/K2
O=$W/scratch/campaign/workers/K2
v=$1
cd $W
python -B tools/campaign/gate.py source/ai/ai_debug --source $K/copies/$v.c --all --out $O/$v.obj > $O/${v}_rows.txt 2>&1
grep -v "^EXACT" $O/${v}_rows.txt
grep "^EXACT" $O/head_rows.txt > $O/_a.txt; grep "^EXACT" $O/${v}_rows.txt > $O/_b.txt
cmp -s $O/_a.txt $O/_b.txt && echo SAME_EXACT_ROWS || diff $O/_a.txt $O/_b.txt
python -B $K/tools/dumpfn.py $O/$v.obj _ai_debug_render_actor 0 0x7000 > $O/$v.dis
python -B $K/tools/sdiff.py $O/jan.dis $O/$v.dis 2 > $O/sdiff_$v.txt; tail -1 $O/sdiff_$v.txt
echo "regions(<0x6000): $(awk '/^--- /{split($0,a," "); x=strtonum("0x" a[6]); if (x<0x6000) n++} END{print n}' $O/sdiff_$v.txt)"
python -B $K/tools/widthcensus.py _ai_debug_render_actor ai_debug $O/$v.obj | sed -n '1p;$p'
python -B $K/tools/metric.py $O/jan.dis $O/$v.dis
