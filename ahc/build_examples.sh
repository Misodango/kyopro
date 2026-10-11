#!/usr/bin/env bash
# examples を警告ゼロ (-Werror) でビルドし、スモーク条件つきで実行する。
# 使い方: bash templates/ahc/build_examples.sh
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
OUT=build
mkdir -p "$OUT"
CXX=${CXX:-g++}
WARN="-Wall -Wextra -Wshadow -Werror"
EXAMPLES=(tsp_sa tsp_sa_undo grid_beam main_template)

echo "== header self-containment =="
for h in common sa beam chokudai; do
  echo "#include \"$PWD/$h.hpp\"" | $CXX -std=c++20 $WARN -fsyntax-only -x c++ -
done

echo "== build (-O2 $WARN) =="
for ex in "${EXAMPLES[@]}"; do
  $CXX -std=c++20 -O2 $WARN "examples/$ex.cpp" -o "$OUT/$ex"
done

echo "== run =="
for ex in "${EXAMPLES[@]}"; do
  AHC_TIME_MS=${AHC_TIME_MS:-300} "$OUT/$ex" </dev/null >/dev/null
done

echo "== sanitizer build/run (-O1 asan+ubsan, short) =="
for ex in tsp_sa tsp_sa_undo grid_beam; do
  $CXX -std=c++20 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined $WARN \
    "examples/$ex.cpp" -o "$OUT/${ex}_san"
  AHC_TIME_MS=200 "$OUT/${ex}_san" </dev/null >/dev/null
done

echo "== bundle =="
python3 bundle.py examples/tsp_sa.cpp -o "$OUT/tsp_sa_submit.cpp"
if grep -nE '^\s*#\s*include\s+"' "$OUT/tsp_sa_submit.cpp"; then
  echo "ERROR: local include remains in bundled file" >&2
  exit 1
fi
if grep -nE '#\s*pragma\s+once' "$OUT/tsp_sa_submit.cpp"; then
  echo "ERROR: #pragma once remains in bundled file" >&2
  exit 1
fi
# 単一ファイルとして (元ディレクトリ非依存の場所で) コンパイルできること
mkdir -p "$OUT/bundle_check"
cp "$OUT/tsp_sa_submit.cpp" "$OUT/bundle_check/submit.cpp"
(cd "$OUT/bundle_check" && $CXX -std=c++20 -O2 $WARN submit.cpp -o submit && AHC_TIME_MS=200 ./submit </dev/null >/dev/null)

echo "build_examples: ALL OK"
