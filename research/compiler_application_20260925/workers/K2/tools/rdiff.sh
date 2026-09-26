#!/bin/sh
# usage: rdiff.sh <a> <b> : compare sdiff region lists (J offset, delta) of two evaluated variants
O=/c/halo-worktrees/claude-compiler-application-20260925/scratch/campaign/workers/K2
f(){ awk '/^--- /{split($0,a," "); x=strtonum("0x" a[6]); if (x>=0x6000) exit; print a[6], a[9]}' $O/sdiff_$1.txt | grep -v "^$" ; }
diff <(f $1) <(f $2)
