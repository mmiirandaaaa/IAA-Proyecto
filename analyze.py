#!/usr/bin/env python3
"""
Analysis of results/runs.csv (from run_experiments.py).

Usage (from ProyectoIAA/):  python3 analyze.py [--out results]

Writes to results/:
  summary.csv / summary.tex  best, mean, std of the gap per problem and variant,
                             feasibility rate, mean first-feasible generation, mean generations
  tests.csv                  Mann-Whitney per problem (mig vs nomig), Holm-adjusted p
  wilcoxon.txt               one Wilcoxon signed-rank over the per-problem mean gaps
  fig_gap_ct1.pdf, fig_gap_ct9.pdf, fig_feas_ct9.pdf, fig_conv.pdf

Gap reference: best feasible profit found by ANY run (either variant) on that problem.
Say so in the paper: it is a gap to the best known in this study, not to the optimum.

Infeasible runs (decided before looking at results):
  - excluded from best/mean/std gap, reported through the feasibility rate;
  - Mann-Whitney: ranked below every feasible run;
  - Wilcoxon: an infeasible run counts as gap = 100 % in the per-problem mean.
"""
import argparse
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.stats import mannwhitneyu, wilcoxon

KEY = ["file", "inst", "q", "coefs"]
VARS = ["mig", "nomig"]
LABEL = {"mig": "with migration", "nomig": "no migration (mu = 0)"}
COLOR = {"mig": "#2a6fdb", "nomig": "#e07b39"}
INST_ORIG = {1: 1, 5: 6, 10: 11}  # test-file config -> original OR-Library config


def pname(r):
    return f"{INST_ORIG.get(r['inst'], r['inst'])}-q{r['q']}-{'m' if r['coefs'] == 'mixed' else 'p'}"


def holm(p):
    p = np.asarray(p, float)
    ok = ~np.isnan(p)
    adj = np.full_like(p, np.nan)
    idx = np.where(ok)[0][np.argsort(p[ok])]
    m, run = len(idx), 0.0
    for rank, i in enumerate(idx):
        run = max(run, min(1.0, (m - rank) * p[i]))
        adj[i] = run
    return adj


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="results")
    args = ap.parse_args()
    out = Path(args.out)

    df = pd.read_csv(out / "runs.csv")
    df["feasible"] = df["feasible"].astype(int)
    best = (df[df.feasible == 1].groupby(KEY)["profit"].max().rename("best_known"))
    df = df.join(best, on=KEY)
    df["gap"] = np.where(df.feasible == 1, 100 * (df.best_known - df.profit) / df.best_known, np.nan)
    df["gap_pess"] = df["gap"].fillna(100.0)
    df["score"] = np.where(df.feasible == 1, df.profit, -1e15)  # infeasible below every feasible
    df["problem"] = df.apply(pname, axis=1)

    # ---- summary table
    g = df.groupby(KEY + ["variant"])
    summ = pd.DataFrame({
        "best_known": g["best_known"].first(),
        "runs": g.size(),
        "gap_best": g["gap"].min(),
        "gap_mean": g["gap"].mean(),
        "gap_std": g["gap"].std(),
        "feas_rate": g["feasible"].mean(),
        "first_feas_gen": g["first_feasible_gen"].mean(),
        "generations": g["generations"].mean(),
    }).reset_index()
    summ.to_csv(out / "summary.csv", index=False, float_format="%.4f")

    wide = summ.pivot_table(index=KEY, columns="variant",
                            values=["gap_mean", "gap_std", "gap_best", "feas_rate"])
    wide.columns = [f"{a}_{b}" for a, b in wide.columns]
    wide = wide.reset_index()
    wide["problem"] = wide.apply(pname, axis=1)

    # ---- per-problem Mann-Whitney
    rows = []
    for k, sub in df.groupby(KEY):
        a = sub[sub.variant == "mig"]["score"].values
        b = sub[sub.variant == "nomig"]["score"].values
        try:
            p = mannwhitneyu(a, b, alternative="two-sided").pvalue
        except ValueError:
            p = np.nan
        rows.append(dict(zip(KEY, k), mean_gap_mig=sub[sub.variant == "mig"]["gap_pess"].mean(),
                         mean_gap_nomig=sub[sub.variant == "nomig"]["gap_pess"].mean(), p_mwu=p))
    tests = pd.DataFrame(rows)
    tests["p_holm"] = holm(tests["p_mwu"])
    tests["winner"] = np.where(tests.p_holm < 0.05,
                               np.where(tests.mean_gap_mig < tests.mean_gap_nomig, "mig", "nomig"), "-")
    tests.to_csv(out / "tests.csv", index=False, float_format="%.4g")

    # ---- Wilcoxon over per-problem means
    d = tests.mean_gap_mig - tests.mean_gap_nomig
    try:
        w = wilcoxon(tests.mean_gap_mig, tests.mean_gap_nomig)
        wtxt = f"W = {w.statistic:.1f}, p = {w.pvalue:.4g}"
    except ValueError as e:
        wtxt = f"not computable ({e})"
    with open(out / "wilcoxon.txt", "w") as f:
        f.write(f"Wilcoxon signed-rank, {len(tests)} problems, per-problem mean gap "
                f"(infeasible = 100 %), mig vs nomig: {wtxt}\n")
        f.write(f"mig better on {(d < 0).sum()}, nomig better on {(d > 0).sum()}, ties {(d == 0).sum()}\n")
        f.write(f"MWU significant after Holm: mig {sum(tests.winner == 'mig')}, "
                f"nomig {sum(tests.winner == 'nomig')}, none {sum(tests.winner == '-')}\n")

    # ---- LaTeX table
    t = wide.merge(tests[KEY + ["p_holm"]], on=KEY).sort_values(KEY)
    lines = [r"\begin{tabular}{llrrrrrrr}", r"\hline",
             r"File & Problem & \multicolumn{3}{c}{With migration} & \multicolumn{3}{c}{No migration} & $p_{Holm}$\\",
             r" & & best & mean (sd) & feas & best & mean (sd) & feas & \\", r"\hline"]
    fmt = lambda x: "--" if pd.isna(x) else f"{x:.2f}"
    for _, r in t.iterrows():
        lines.append(
            f"{r.file} & {r.problem} & {fmt(r.gap_best_mig)} & {fmt(r.gap_mean_mig)} ({fmt(r.gap_std_mig)}) & "
            f"{r.feas_rate_mig:.2f} & {fmt(r.gap_best_nomig)} & {fmt(r.gap_mean_nomig)} ({fmt(r.gap_std_nomig)}) & "
            f"{r.feas_rate_nomig:.2f} & {'--' if pd.isna(r.p_holm) else f'{r.p_holm:.3g}'}\\\\")
    lines += [r"\hline", r"\end{tabular}"]
    (out / "summary.tex").write_text("\n".join(lines) + "\n")

    plt.rcParams.update({"font.size": 8, "axes.spines.top": False, "axes.spines.right": False})

    # ---- gap boxplots, one per file
    for fkey in sorted(df.file.unique()):
        sub = df[(df.file == fkey) & (df.feasible == 1)]
        probs = sorted(df[df.file == fkey].problem.unique(),
                       key=lambda s: (int(s.split("-")[0]), s.split("-")[1], s.split("-")[2]))
        fig, ax = plt.subplots(figsize=(7, 2.6))
        for j, v in enumerate(VARS):
            data = [sub[(sub.problem == p) & (sub.variant == v)]["gap"].values for p in probs]
            pos = np.arange(len(probs)) + (j - 0.5) * 0.36
            bp = ax.boxplot([x if len(x) else [np.nan] for x in data], positions=pos, widths=0.32,
                            patch_artist=True, showfliers=True, flierprops={"markersize": 2})
            for b in bp["boxes"]:
                b.set(facecolor=COLOR[v], alpha=0.6, linewidth=0.6)
            ax.plot([], [], "s", color=COLOR[v], alpha=0.6, label=LABEL[v])
        ax.set_xticks(range(len(probs)), probs, rotation=60)
        ax.set_ylabel("gap to best known (%)")
        ax.set_title(f"{fkey}: gap of feasible runs (config-q-coefs; m = mixed, p = positive)")
        ax.legend(frameon=False)
        fig.tight_layout()
        fig.savefig(out / f"fig_gap_{fkey}.pdf")
        plt.close(fig)

    # ---- feasibility rate by q for ct9
    if "ct9" in df.file.values:
        fr = df[df.file == "ct9"].groupby(["q", "variant"])["feasible"].mean().unstack()
        fig, ax = plt.subplots(figsize=(3.4, 2.4))
        x = np.arange(len(fr.index))
        for j, v in enumerate(VARS):
            if v in fr:
                ax.bar(x + (j - 0.5) * 0.38, fr[v].values, 0.36, color=COLOR[v], label=LABEL[v])
        ax.set_xticks(x, [f"q option {q}" for q in fr.index])
        ax.set_ylim(0, 1.05)
        ax.set_ylabel("feasibility rate")
        ax.set_title("ct9")
        ax.legend(frameon=False, fontsize=7)
        fig.tight_layout()
        fig.savefig(out / "fig_feas_ct9.pdf")
        plt.close(fig)

    # ---- convergence curves (seed 1 traces)
    cases = [("ct1", 1, 1, "mixed"), ("ct9", 1, 1, "mixed"), ("ct9", 1, 3, "mixed")]
    fig, axes = plt.subplots(1, 3, figsize=(7, 2.3))
    for ax, (fk, i, q, c) in zip(axes, cases):
        for v in VARS:
            tr = out / "traces" / f"{v}_{fk}_i{i}_q{q}_{c}_s1.csv"
            if not tr.exists():
                continue
            t = pd.read_csv(tr)
            y = t.profit.where(t.feasible == 1)
            ax.plot(t.generation, y, color=COLOR[v], lw=1, label=LABEL[v])
            if (t.feasible == 1).any():
                ax.axvline(t.generation[t.feasible == 1].iloc[0], color=COLOR[v], lw=0.6, ls=":")
        ax.set_title(f"{fk}, config {INST_ORIG[i]}, q option {q}, {c}", fontsize=7)
        ax.set_xlabel("generation")
    axes[0].set_ylabel("profit of x* (feasible only)")
    axes[0].legend(frameon=False, fontsize=6)
    fig.tight_layout()
    fig.savefig(out / "fig_conv.pdf")
    plt.close(fig)

    print((out / "wilcoxon.txt").read_text())
    print(f"written to {out}/")


if __name__ == "__main__":
    main()