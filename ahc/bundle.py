#!/usr/bin/env python3
"""ローカル include ("...") を再帰展開して単一ファイルにまとめるバンドラ。

使い方: python3 bundle.py main.cpp -o submit.cpp [-I dir ...]
- #include "xxx.hpp" を include 元ファイルのディレクトリ -> -I の順で解決して展開
- #pragma once を持つファイルは 2 回目以降スキップ (展開時に #pragma once 行は除去)
- #include <...> はそのまま残す
"""
import argparse
import os
import re
import sys

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
PRAGMA_ONCE_RE = re.compile(r'^\s*#\s*pragma\s+once\b')


class Bundler:
    def __init__(self, include_dirs):
        self.include_dirs = include_dirs
        self.once_done = set()  # 展開済みの #pragma once ファイル (realpath)
        self.stack = []         # 循環 include 検出用

    def resolve(self, name, from_dir):
        for d in [from_dir] + self.include_dirs:
            cand = os.path.join(d, name)
            if os.path.isfile(cand):
                return os.path.realpath(cand)
        raise FileNotFoundError('include not found: "%s" (searched %s)' % (name, [from_dir] + self.include_dirs))

    def expand(self, path, is_main=False):
        path = os.path.realpath(path)
        with open(path, encoding="utf-8") as f:
            lines = f.read().splitlines()
        has_once = any(PRAGMA_ONCE_RE.match(l) for l in lines)
        if has_once and not is_main:
            if path in self.once_done:
                return []
            self.once_done.add(path)
        if path in self.stack:
            raise RuntimeError("circular include: " + " -> ".join(self.stack + [path]))
        self.stack.append(path)
        out = []
        if not is_main:
            out.append("// ---- begin %s ----" % os.path.basename(path))
        base = os.path.dirname(path)
        for line in lines:
            if not is_main and PRAGMA_ONCE_RE.match(line):
                continue
            m = INCLUDE_RE.match(line)
            if m:
                out.extend(self.expand(self.resolve(m.group(1), base)))
            else:
                out.append(line)
        if not is_main:
            out.append("// ---- end %s ----" % os.path.basename(path))
        self.stack.pop()
        return out


def main():
    ap = argparse.ArgumentParser(description="Bundle local includes into a single C++ file.")
    ap.add_argument("input")
    ap.add_argument("-o", "--output", help="output file (default: stdout)")
    ap.add_argument("-I", dest="include_dirs", action="append", default=[], help="extra include dir")
    args = ap.parse_args()
    try:
        out = Bundler(args.include_dirs).expand(args.input, is_main=True)
    except (OSError, RuntimeError) as e:
        print("bundle.py: error: %s" % e, file=sys.stderr)
        return 1
    text = "\n".join(out) + "\n"
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
