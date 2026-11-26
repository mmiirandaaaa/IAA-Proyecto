source "$(dirname "$0")/common.sh"

ALGO=$1
OUT=$2
shift 2 || { echo "usage: $0 <ga|tsts> <out.csv> [algorithm parameters...]" >&2; exit 1; }

case "$ALGO" in
	ga)   EXE="$GA_EXE";   EXTRA=(--gmax 2000000000) ;;
	tsts) EXE="$TSTS_EXE"; EXTRA=() ;;
	*) echo "first argument must be ga or tsts" >&2; exit 1 ;;
esac
check_exe "$EXE"

SEEDS=${SEEDS:-10}
JOBS=${JOBS:-4}
PARAMS=("$@")

mkdir -p "$(dirname "$OUT")"
[ -f "$OUT" ] || echo "algo,file,instancia,q,coefs,seed,status,profit,tmax,seconds" > "$OUT"

# One run. Called by xargs with: file instancia q coefs seed
one_run() {
	local file=$1 inst=$2 q=$3 coefs=$4 seed=$5
	local tmax out status profit secs
	tmax=$(tmax_for "$PROJ/$file")
	out=$("$EXE" --q "$q" --coefs "$coefs" --instancia "$inst" --seed "$seed" \
		--tmax "$tmax" "${EXTRA[@]}" "${PARAMS[@]}" < "$PROJ/$file" 2>&1)
	profit=$(printf '%s\n' "$out" | grep '^Profit:' | tail -1 | awk '{print $2}')
	secs=$(printf '%s\n' "$out" | grep '^Tiempo:' | tail -1 | awk '{print $2}')
	if [ -z "$profit" ]; then
		echo "run_final: no 'Profit:' for $file,$inst,$q,$coefs seed $seed" >&2
		return
	fi
	if printf '%s\n' "$out" | grep -q 'No se encontr'; then status=infeasible; else status=feasible; fi
	echo "$ALGO,$file,$inst,$q,$coefs,$seed,$status,$profit,$tmax,$secs" >> "$OUT"
}
export -f one_run tmax_for
export ALGO EXE PROJ OUT TIME_FACTOR
export PARAMS_STR="${PARAMS[*]}"

# bash arrays do not survive xargs, so rebuild them inside the worker shell
worker() {
	read -r -a PARAMS <<< "$PARAMS_STR"
	if [ "$ALGO" = ga ]; then EXTRA=(--gmax 2000000000); else EXTRA=(); fi
	one_run "$@"
}
export -f worker

# Job list = 2 files x 3 instances x 3 q x 2 coefs x SEEDS, minus the runs already in the CSV
for file in test/mdmkp_ct1.txt test/mdmkp_ct9.txt; do
	for inst in 1 5 10; do
		for q in 1 2 3; do
			for coefs in mixed positive; do
				for seed in $(seq 1 "$SEEDS"); do
					grep -q "^$ALGO,$file,$inst,$q,$coefs,$seed," "$OUT" || echo "$file $inst $q $coefs $seed"
				done
			done
		done
	done
done > /tmp/run_final_jobs.$$

TOTAL=$(wc -l < /tmp/run_final_jobs.$$)
echo "run_final: $TOTAL runs to do ($ALGO, TIME_FACTOR=$TIME_FACTOR, $JOBS in parallel), output $OUT"
if [ "$TOTAL" -gt 0 ]; then
	xargs -a /tmp/run_final_jobs.$$ -P "$JOBS" -L 1 bash -c 'worker "$@"' _
fi
rm -f /tmp/run_final_jobs.$$
echo "run_final: done, $(($(wc -l < "$OUT") - 1)) rows in $OUT"