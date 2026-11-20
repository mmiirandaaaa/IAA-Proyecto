# Tuning the island GA and TSTS with irace

Both algorithms are tuned with the same instances, the same time limit per run, the same
number of runs (maxExperiments), the same parallelism and the same irace seed. The only
differences between the two scenarios are the parameter file, the default configuration and
the binary that the target runner calls.

## Layout

All paths are derived from the location of `common.sh`, so the scripts work from any folder.

```
ProyectoIAA/
  mdmkp/mdmkp          island GA binary   (cd mdmkp && make)
  tabusearch/tsts      TSTS binary        (compile command at the top of TSTS.cpp)
  train/  test/        instance files     (split_instances.py)
  tuning/
    common.sh            paths, time limit, cost: shared by both runners
    make_instances.sh    builds the irace instance list from train/ or test/
    ga/    scenario.txt  parameters.txt  default.txt  target-runner
    tsts/  scenario.txt  parameters.txt  default.txt  target-runner
```

If a binary is missing, the runner stops and prints the command that builds it.

## Steps

```
cd ProyectoIAA/tuning
chmod +x make_instances.sh ga/target-runner tsts/target-runner
./make_instances.sh train > instances-train.txt

# smoke test, one call each (prints a single number, the cost)
./ga/target-runner 1 1 42 train/mdmkp_ct1.txt,1,1,mixed
./tsts/target-runner 1 1 42 train/mdmkp_ct1.txt,1,1,mixed

cd ga   && irace --scenario scenario.txt --check && irace --scenario scenario.txt
cd ../tsts && irace --scenario scenario.txt --check && irace --scenario scenario.txt
```

The instance token (`train/mdmkp_ct1.txt,1,1,mixed`) is relative to `ProyectoIAA/`.

Run the two tunings one after the other, not at the same time, on the charger, with Low
Power Mode off and nothing else heavy running. Set `parallel` (in both scenarios) to the
number of performance cores: `sysctl -n hw.perflevel0.physicalcpu`.

The best configuration is printed at the end under "Best configurations as commandlines"
(the first line is the one to use). `irace.Rdata` in each folder keeps the full log.

## What is fixed and why

- Time limit: `tmax = n * TIME_FACTOR` seconds, computed in `common.sh` for both. With
  `TIME_FACTOR = 0.24` that is 24 / 60 / 120 s for n = 100 / 250 / 500. The GA gets
  `--gmax 2000000000`, so it also stops only by time. Use the same rule in the final
  experiments.
- Cost: `-profit` if the solution is feasible, `10^9` if not. irace's default F-test works on
  ranks per instance, so the different profit scales of the instances do not matter.
- Budget: `maxExperiments = 1000` for both. Since every run uses its time limit, that is the
  same CPU time for both tunings (TSTS may stop early when its second stage has no improving
  swap left; report the time actually used).
- Starting points: each algorithm starts from its own default (`default.txt`): the values
  of the GA README, and the paper values for TSTS (alpha = 10^4, lambda = 100).

## What to report in the paper

Training instances (the train split, how it was drawn), parameter ranges (`parameters.txt`),
the time limit rule, maxExperiments, the irace version and the final values for both
algorithms.