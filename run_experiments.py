#!/usr/bin/env python3
"""
Final runs for the island GA: with migration vs without migration (--mu 0).

Test set: configurations 1, 6, 11 of ct1 and ct9 (original numbering), which are
configurations 1, 5, 10 of test/mdmkp_ct1.txt and test/mdmkp_ct9.txt after
split_instances.py (seed 2026). q = 1,2,3 x mixed/positive -> 36 problems.

Usage (from ProyectoIAA/):
    python3 run_experiments.py --params "--pc 0.9 --pm 0.004 ..." --seeds 10

Paste the irace best configuration into --params exactly as irace prints it.
Results are appended to results/runs.csv as runs finish, and runs already in that
file are skipped, so you can stop it and start it again.
"""
import argparse
import concurrent.futures as cf
import csv
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
EXE = ROOT / "mdmkp" / "mdmkp"
FILES = {"ct1": "test/mdmkp_ct1.txt", "ct9": "test/mdmkp_ct9.txt"}
CONFIGS = [1, 5, 10]                    # = original 1, 6, 11
QS = [1, 2, 3]
COEFS = ["mixed", "positive"]
VARIANTS = {"mig": [], "nomig": ["--mu", "0"]}   # --mu 0 comes last, so it overrides --params
FIELDS = ["variant", "file", "inst", "q", "coefs", "seed", "profit", "feasible",
          "generations", "time", "first_feasible_gen", "tmax"]


def n_items(path):
    with open(path) as f:
        f.readline()
        return int(f.readline().split()[0])


def first_feasible(trace):
    with open(trace) as f:
        next(f)
        for line in f:
            p = line.rstrip().split(",")
            if p[2] == "1":
                return int(p[0])
    return ""


def run_one(job, params, factor, trace_dir, keep_trace):
    variant, key, inst, q, coefs, seed = job
    path = ROOT / FILES[key]
    tmax = n_items(path) * factor
    trace = trace_dir / f"{variant}_{key}_i{inst}_q{q}_{coefs}_s{seed}.csv"
    cmd = [str(EXE), "--q", str(q), "--coefs", coefs, "--instancia", str(inst),
           "--seed", str(seed), "--tmax", str(tmax), "--gmax", "2000000000",
           *params, *VARIANTS[variant], "--csv", str(trace)]
    with open(path) as fin:
        out = subprocess.run(cmd, stdin=fin, capture_output=True, text=True).stdout

    m = re.search(r"^Profit:\s*(-?[\d.eE+]+)", out, re.M)
    if not m:
        raise RuntimeError(f"no Profit line for {job}:\n{out[-500:]}")
    feasible = 0 if "No se encontr" in out else 1
    gens = re.search(r"^Generaciones:\s*(\d+)", out, re.M)
    secs = re.search(r"^Tiempo:\s*([\d.eE+-]+)", out, re.M)
    ffg = first_feasible(trace) if trace.exists() else ""
    if not keep_trace and trace.exists():
        trace.unlink()
    return dict(variant=variant, file=key, inst=inst, q=q, coefs=coefs, seed=seed,
                profit=m.group(1), feasible=feasible,
                generations=gens.group(1) if gens else "",
                time=secs.group(1) if secs else "", first_feasible_gen=ffg, tmax=tmax)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--params", required=True, help="tuned parameters, as irace prints them")
    ap.add_argument("--seeds", type=int, default=10)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--time-factor", type=float, default=0.02, help="tmax = n * factor (same as tuning)")
    ap.add_argument("--out", default="results")
    args = ap.parse_args()

    if not os.access(EXE, os.X_OK):
        sys.exit(f"{EXE} not found or not executable (cd mdmkp && make)")
    for f in FILES.values():
        if not (ROOT / f).exists():
            sys.exit(f"{ROOT / f} not found (run split_instances.py)")

    params = shlex.split(args.params)
    out = ROOT / args.out
    trace_dir = out / "traces"
    trace_dir.mkdir(parents=True, exist_ok=True)
    runs_csv = out / "runs.csv"

    done = set()
    if runs_csv.exists():
        with open(runs_csv) as f:
            for r in csv.DictReader(f):
                done.add((r["variant"], r["file"], int(r["inst"]), int(r["q"]), r["coefs"], int(r["seed"])))

    jobs = [(v, k, i, q, c, s)
            for s in range(1, args.seeds + 1)
            for v in VARIANTS for k in FILES for i in CONFIGS for q in QS for c in COEFS]
    jobs = [j for j in jobs if j not in done]
    print(f"{len(jobs)} runs to do ({len(done)} already in {runs_csv}), {args.jobs} in parallel")

    new_file = not runs_csv.exists()
    with open(runs_csv, "a", newline="") as f, cf.ThreadPoolExecutor(args.jobs) as ex:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        if new_file:
            w.writeheader()
        futs = {ex.submit(run_one, j, params, args.time_factor, trace_dir, j[5] == 1): j for j in jobs}
        for n, fut in enumerate(cf.as_completed(futs), 1):
            try:
                w.writerow(fut.result())
                f.flush()
            except Exception as e:
                print(f"FAILED {futs[fut]}: {e}", file=sys.stderr)
            if n % 20 == 0 or n == len(jobs):
                print(f"  {n}/{len(jobs)}", flush=True)


if __name__ == "__main__":
    main()