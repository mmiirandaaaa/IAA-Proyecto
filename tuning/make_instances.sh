#!/bin/bash
# Writes the irace instance list for one split, one instance per line:
#   <file relative to ProyectoIAA>,<instancia>,<q>,<coefs>
# Every configuration of every file gives 6 instances (q = 1,2,3 x mixed/positive).
#
# Usage (from any folder):
#   ./make_instances.sh train > instances-train.txt
#   ./make_instances.sh test  > instances-test.txt

source "$(dirname "$0")/common.sh"
SPLIT=${1:-train}

if [ ! -d "$PROJ/$SPLIT" ]; then
	echo "make_instances: folder $PROJ/$SPLIT not found" >&2
	exit 1
fi

for f in "$PROJ/$SPLIT"/mdmkp_ct*.txt; do
	T=$(head -1 "$f" | awk '{print $1}')
	for i in $(seq 1 "$T"); do
		for q in 1 2 3; do
			for c in mixed positive; do
				echo "$SPLIT/$(basename "$f"),$i,$q,$c"
			done
		done
	done
done