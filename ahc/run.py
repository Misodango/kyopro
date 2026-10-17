#!/usr/bin/env python3
"""AHC の全 seed 並列実行。コンテストのディレクトリ (tools/ がある所) で動かす。
出典: https://github.com/Misodango/kyopro/blob/main/ahc/run.py

    python3 ~/kyopro/ahc/run.py                      # seed 0-99 を ./a.out で
    python3 ~/kyopro/ahc/run.py -s 0-29 -j 6 T0=50   # K=V は環境変数 P_K として渡す
    python3 ~/kyopro/ahc/run.py --save base          # 結果を runs/base.json に保存
    python3 ~/kyopro/ahc/run.py --base base          # runs/base.json と seed ごとに比べる

tools/target/release/tester があれば `tester ./a.out < in > out` (インタラクティブ)、
なければ `./a.out < in > out` のあと `vis in out` で採点する。
出力の最後の `Score = X` を拾う。out/XXXX.txt と out/XXXX.err に出力と stderr を残す。
"""
import argparse
import concurrent.futures
import json
import os
import re
import subprocess
import sys
import time

SCORE_RE = re.compile(r"Score\s*=\s*(-?\d+(?:\.\d+)?)")
TOOLS = os.path.join("tools", "target", "release")


def parse_seeds(spec):
    seeds = []
    for part in spec.split(","):
        if "-" in part:
            a, b = part.split("-")
            seeds += range(int(a), int(b) + 1)
        elif part:
            seeds.append(int(part))
    return seeds


def parse_params(kvs):
    d = {}
    for kv in kvs:
        k, v = kv.split("=", 1)
        d[k] = v
    return d


def run_one(seed, params, cmd, timeout):
    """1 ケース実行して (score, 秒) を返す。失敗したら score = None"""
    inp = os.path.join("tools", "in", "%04d.txt" % seed)
    outp = os.path.join("out", "%04d.txt" % seed)
    errp = os.path.join("out", "%04d.err" % seed)
    env = dict(os.environ)
    for k, v in params.items():
        env["P_" + k] = str(v)
    tester = os.path.join(TOOLS, "tester")
    interactive = os.path.exists(tester)
    st = time.time()
    try:
        with open(inp) as fin, open(outp, "w") as fout:
            p = subprocess.run(([tester] if interactive else []) + cmd,
                               stdin=fin, stdout=fout, stderr=subprocess.PIPE,
                               env=env, timeout=timeout, text=True, errors="replace")
        sec = time.time() - st
        log = p.stderr
        if not interactive:
            v = subprocess.run([os.path.join(TOOLS, "vis"), inp, outp],
                               capture_output=True, text=True, errors="replace")
            log += v.stdout + v.stderr
    except subprocess.TimeoutExpired:
        return None, timeout
    with open(errp, "w") as f:
        f.write(log)
    m = SCORE_RE.findall(log)
    if p.returncode != 0 or not m:
        return None, sec
    return float(m[-1]), sec


def evaluate(params, seeds, cmd, jobs, timeout=10):
    """全 seed を並列に回して {seed: (score, 秒)} を返す"""
    for s in seeds:
        if not os.path.exists(os.path.join("tools", "in", "%04d.txt" % s)):
            sys.exit("tools/in/%04d.txt がない。tools で seq 0 199 > seeds.txt && cargo run -r --bin gen seeds.txt (100-199 だけ書くと 0000 から上書きされる)" % s)
    os.makedirs("out", exist_ok=True)
    with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
        fs = {s: ex.submit(run_one, s, params, cmd, timeout) for s in seeds}
        return {s: f.result() for s, f in fs.items()}


def relative(scores, base, minimize):
    """seed ごとの base 比 (大きいほど良い) の平均。AHC の相対評価に近い"""
    rs = []
    for s, x in scores.items():
        b = base.get(s)
        if x is None:
            rs.append(0.0)
        elif not b:
            continue
        elif minimize:
            rs.append(b / x if x else 1.0)
        else:
            rs.append(x / b)
    return sum(rs) / len(rs) if rs else 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("params", nargs="*", help="K=V (環境変数 P_K になる)")
    ap.add_argument("-s", "--seeds", default="0-99")
    ap.add_argument("-j", "--jobs", type=int, default=6)
    ap.add_argument("-c", "--cmd", default="./a.out")
    ap.add_argument("--save", help="runs/NAME.json に保存")
    ap.add_argument("--base", help="runs/NAME.json と比べる")
    ap.add_argument("--min", action="store_true", help="スコアが小さいほど良い問題")
    ap.add_argument("-v", action="store_true", help="seed ごとに表示")
    a = ap.parse_args()

    seeds = parse_seeds(a.seeds)
    res = evaluate(parse_params(a.params), seeds, a.cmd.split(), a.jobs)
    scores = {s: r[0] for s, r in res.items()}
    base = None
    if a.base:
        with open(os.path.join("runs", a.base + ".json")) as f:
            base = {int(k): v for k, v in json.load(f).items()}

    for s in seeds:
        sc, sec = res[s]
        bad = sc is None
        if a.v or bad:
            line = "%04d %12s %6.2fs" % (s, "FAIL" if bad else "%.0f" % sc, sec)
            if base and base.get(s) and not bad:
                line += "  base %12.0f  %+.3f" % (base[s], sc / base[s] - 1)
            print(line)

    ok = [x for x in scores.values() if x is not None]
    print("cases %d  fail %d  mean %.1f  sum %.0f  max time %.2fs" % (
        len(seeds), len(seeds) - len(ok), sum(ok) / max(1, len(ok)), sum(ok),
        max(r[1] for r in res.values())))
    if base:
        win = lose = 0
        for s, x in scores.items():
            b = base.get(s)
            if x is None or b is None or x == b:
                continue
            if (x > b) != a.min:
                win += 1
            else:
                lose += 1
        print("vs %s: relative %.4f  win %d  lose %d" % (
            a.base, relative(scores, base, a.min), win, lose))
    if a.save:
        os.makedirs("runs", exist_ok=True)
        with open(os.path.join("runs", a.save + ".json"), "w") as f:
            json.dump(scores, f)
        print("saved runs/%s.json" % a.save)


if __name__ == "__main__":
    main()
