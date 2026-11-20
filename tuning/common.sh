# Shared by both target runners. Everything that makes the comparison fair lives here,
# so the instance format, the time limit and the cost are literally the same for both.

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJ="$HERE"          # folder that contains train/ and test/ (from split_instances.py)
GA_EXE="$PROJ/mdmkp"                 # island GA binary (make in mdmkp)
TSTS_EXE="$HERE/tabusearch"        # TSTS binary (compile command at the top of TSTS.cpp)

# Time limit per run, in seconds, depending only on the number of items n of the instance:
#   tmax = n * TIME_FACTOR
# Both algorithms get exactly this limit. Change it here and nowhere else.
TIME_FACTOR=${TIME_FACTOR:-0.24} # because we want 120 s for n=500

tmax_for() {
	local n
	n=$(sed -n 2p "$1" | awk '{print $1}')
	awk -v n="$n" -v f="$TIME_FACTOR" 'BEGIN { print n * f }'
}

# irace minimizes the last number printed by the target runner.
# Feasible solution: cost = -profit. No feasible solution: a constant worse than any feasible cost.
INFEASIBLE_COST=1000000000

cost_from_output() {
	local out="$1" z
	z=$(printf '%s\n' "$out" | grep '^Profit:' | tail -1 | awk '{print $2}')
	if [ -z "$z" ]; then
		echo "target-runner: no 'Profit:' line in the output. Last lines:" >&2
		printf '%s\n' "$out" | tail -5 >&2
		exit 1
	fi
	if printf '%s\n' "$out" | grep -q 'No se encontr'; then
		echo "$INFEASIBLE_COST"
	else
		echo $(( -z ))
	fi
}

# An irace instance is one token: <file relative to ProyectoIAA>,<instancia>,<q>,<coefs>
# e.g. train/mdmkp_ct9.txt,1,3,mixed
split_instance() {
	IFS=',' read -r FILE INST Q COEFS <<< "$1"
	if [ ! -f "$PROJ/$FILE" ]; then
		echo "target-runner: instance file $PROJ/$FILE not found" >&2
		exit 1
	fi
}
