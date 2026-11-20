"""
Splits the OR-Library MDMKP files mdmkp_ct1..9.txt into a train set and a test set.

Each input file is: a first line with the number of configurations (15), then that many
blocks of exactly 2*M+9 lines (header "N M", M knapsack rows, M bounds, M demand rows,
M bounds, 6 profit rows). The blocks are copied verbatim, so the existing reader works
unchanged on the output files (the first line is rewritten with the new block count).

For every file, n_train configurations are chosen at random (fixed seed) for train and
all the others go to test. The 6 instances of a configuration (q x coefs) always stay
together, because they share the same data.

Usage:
    python3 split_instances.py --src instancias --out . --seed 2026 --n-train 1

Output:
    <out>/train/mdmkp_ct{X}.txt
    <out>/test/mdmkp_ct{X}.txt
    <out>/split_results.csv   (split, file, N, M, original_config, new_config)
"""
import argparse
import csv
import random
import sys
from pathlib import Path


def read_blocks(path):
    lines = path.read_text().splitlines()
    while lines and not lines[-1].strip():          # trailing blank lines
        lines.pop()
    if any(not l.strip() for l in lines):
        sys.exit(f"{path}: contains blank lines, the block size would be wrong")

    n_configs = int(lines[0].split()[0])
    n_cols = lines[1].split()
    N, M = int(n_cols[0]), int(n_cols[1])
    size = 2 * M + 9
    body = lines[1:]
    if len(body) != n_configs * size:
        sys.exit(f"{path}: expected {n_configs} blocks of {size} lines "
                 f"({n_configs * size} lines), found {len(body)}")

    blocks = [body[i * size:(i + 1) * size] for i in range(n_configs)]
    for k, b in enumerate(blocks, 1):
        h = b[0].split()
        if int(h[0]) != N or int(h[1]) != M:
            sys.exit(f"{path}: configuration {k} has header {b[0]!r}, expected N={N} M={M}")
    return N, M, blocks


def write_file(path, blocks):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w") as f:
        f.write(f"{len(blocks)}\n")
        for b in blocks:
            f.write("\n".join(b) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default="instancias", help="folder with mdmkp_ct{X}.txt")
    ap.add_argument("--out", default=".", help="folder where train/ and test/ are created")
    ap.add_argument("--seed", type=int, default=2026)
    ap.add_argument("--n-train", type=int, default=1, help="configurations per file for train")
    ap.add_argument("--files", type=int, default=9, help="number of files (ct1..ctX)")
    args = ap.parse_args()

    src, out = Path(args.src), Path(args.out)
    rng = random.Random(args.seed)
    rows = []

    for x in range(1, args.files + 1):
        name = f"mdmkp_ct{x}.txt"
        path = src / name
        if not path.exists():
            sys.exit(f"{path} not found")
        N, M, blocks = read_blocks(path)
        if not 0 < args.n_train < len(blocks):
            sys.exit("--n-train must be between 1 and the number of configurations - 1")

        train_idx = sorted(rng.sample(range(len(blocks)), args.n_train))
        test_idx = [i for i in range(len(blocks)) if i not in train_idx]

        for split, idx in (("train", train_idx), ("test", test_idx)):
            write_file(out / split / name, [blocks[i] for i in idx])
            for new, old in enumerate(idx, 1):
                rows.append([split, name, N, M, old + 1, new])

    with (out / "split_results.csv").open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["split", "file", "N", "M", "original_config", "new_config"])
        w.writerows(rows)

    tr = sum(r[0] == "train" for r in rows)
    te = sum(r[0] == "test" for r in rows)
    print(f"train: {tr} configurations ({tr * 6} instances), test: {te} configurations ({te * 6} instances)")


if __name__ == "__main__":
    main()