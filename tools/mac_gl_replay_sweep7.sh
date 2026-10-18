#!/bin/sh
# usage: sweep7.sh replay capture log
r=$1; c=$2; log=$3
: > "$log"
for opt in "-S" "-S -A d" "-S -A s" "-S -A b" "-S -A db"; do
  echo "### $opt" >> "$log"
  "$r" "$c" -q -n 100 -m PF -v $opt >> "$log" 2>&1
done
echo "### done" >> "$log"
