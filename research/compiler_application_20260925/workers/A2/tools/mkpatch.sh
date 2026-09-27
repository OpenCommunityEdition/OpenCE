#!/bin/sh
# usage: mkpatch.sh <out.patch> <repo-path> <modified-file> [<repo-path> <modified-file> ...]
# LF-normalised unified diff of each modified file against HEAD's blob (git index is LF).
set -e
out=$1; shift
: > "$out"
tmp=$(mktemp -d)
while [ $# -gt 0 ]; do
  p=$1; m=$2; shift 2
  git -C /c/halo-worktrees/claude-compiler-application-20260925 show "HEAD:$p" > "$tmp/a"
  tr -d '\r' < "$m" > "$tmp/b"
  diff -u --label "a/$p" --label "b/$p" "$tmp/a" "$tmp/b" >> "$out" || true
done
rm -rf "$tmp"
