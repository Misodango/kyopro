#!/usr/bin/env python3
"""パラメータの自動調整。コンテストのディレクトリで動かす (run.py と同じ前提)。

    python3 ~/kyopro/ahc/tune.py T0=1:1000:log T1=0.01:10:log -s 0-29 -n 60
    python3 ~/kyopro/ahc/tune.py W1=-2:2 WIDTH=100:2000:int TL=500 -n 40

K=lo:hi      一様に探す           K=lo:hi:log  対数で探す (温度など正の値)
K=lo:hi:int  整数                 K=V          固定 (TL=500 で 1 ケースを短くできる)

最初にコードの既定値 (固定以外は P_ を渡さない) で回して基準にし、
seed ごとの基準比の平均 (relative) が一番高いものを探す。
試行は「ランダムに 1 点」か「今の最良を少しずらす」のどちらか。seed の組は全試行で同じ。
Ctrl-C で止めても、それまでの最良を表示する。全試行は runs/tune.tsv に追記する。
"""
import argparse
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run import evaluate, parse_seeds, relative  # noqa: E402


def parse_space(specs):
    space, fixed = {}, {}
    for kv in specs:
        k, v = kv.split("=", 1)
        if ":" not in v:
            fixed[k] = v
            continue
        p = v.split(":")
        lo, hi = float(p[0]), float(p[1])
        kind = p[2] if len(p) > 2 else "lin"
        space[k] = (lo, hi, kind)
    return space, fixed


# 範囲を [0, 1] に正規化してから動かす
def to_unit(x, lo, hi, kind):
    if kind == "log":
        return (math.log(x) - math.log(lo)) / (math.log(hi) - math.log(lo))
    return (x - lo) / (hi - lo)


def from_unit(u, lo, hi, kind):
    u = min(1.0, max(0.0, u))
    if kind == "log":
        return math.exp(math.log(lo) + u * (math.log(hi) - math.log(lo)))
    x = lo + u * (hi - lo)
    return int(round(x)) if kind == "int" else x


def fmt(params):
    return " ".join("%s=%.6g" % (k, v) if isinstance(v, float) else "%s=%s" % (k, v)
                    for k, v in params.items())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("specs", nargs="+")
    ap.add_argument("-s", "--seeds", default="0-29")
    ap.add_argument("-j", "--jobs", type=int, default=6)
    ap.add_argument("-c", "--cmd", default="./a.out")
    ap.add_argument("-n", "--trials", type=int, default=50)
    ap.add_argument("--min", action="store_true", help="スコアが小さいほど良い問題")
    a = ap.parse_args()

    space, fixed = parse_space(a.specs)
    seeds = parse_seeds(a.seeds)
    cmd = a.cmd.split()
    os.makedirs("runs", exist_ok=True)
    log = open(os.path.join("runs", "tune.tsv"), "a")

    def score_of(res):
        return {s: r[0] for s, r in res.items()}

    print("base (code defaults) %s" % fmt(fixed))
    base = score_of(evaluate(fixed, seeds, cmd, a.jobs))
    if any(v is None for v in base.values()):
        print("warn: base で失敗した seed があります", file=sys.stderr)

    best, best_rel = None, 1.0  # None = コードの既定値
    best_u = {k: random.random() for k in space}
    try:
        for t in range(a.trials):
            # 序盤は広く、終盤は最良の近くを細かく
            sigma = 0.3 * (1 - t / a.trials) + 0.05
            if best is None or random.random() < 0.25:
                u = {k: random.random() for k in space}
            else:
                u = dict(best_u)
                keys = [k for k in space if random.random() < 0.5] or [random.choice(list(space))]
                for k in keys:
                    u[k] = min(1.0, max(0.0, u[k] + random.gauss(0, sigma)))
            params = {k: from_unit(u[k], *space[k]) for k in space}
            sc = score_of(evaluate({**fixed, **params}, seeds, cmd, a.jobs))
            rel = relative(sc, base, a.min)
            fail = sum(v is None for v in sc.values())
            mark = ""
            if rel > best_rel:
                best, best_rel, best_u, mark = params, rel, u, "  *best*"
            print("[%3d/%d] rel %.4f fail %d  %s%s" % (t + 1, a.trials, rel, fail, fmt(params), mark))
            log.write("%.5f\t%d\t%s\n" % (rel, fail, fmt({**fixed, **params})))
            log.flush()
    except KeyboardInterrupt:
        print("\n中断")

    print("=" * 60)
    if best is None:
        print("既定値より良いものは見つからなかった")
        return
    print("best rel %.4f (base 比)" % best_rel)
    print("別の seed でも確認する (seed 100-199 の基準は 1 回だけ保存すればよい):")
    print("  python3 ~/kyopro/ahc/run.py -s 100-199 --save def")
    print("  python3 ~/kyopro/ahc/run.py -s 100-199 --base def %s" % fmt(best))
    print("コードの既定値に書き戻す:")
    for k, v in best.items():
        print('  param("%s", %.6g)' % (k, v))


if __name__ == "__main__":
    main()
