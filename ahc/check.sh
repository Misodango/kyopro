#!/usr/bin/env bash
# acc の C++ テンプレ (acc/cpp/main.cpp) の前半に各スニペットを貼った形でビルドし、実行する。
# 使い方: bash ahc/check.sh
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT=..
OUT=build
mkdir -p "$OUT"
FLAGS="-std=c++20 -Wall -Wextra -Werror -I $ROOT/ac-library -I $ROOT/include -DLOCAL"

for name in sa beam chokudai; do
  # テンプレの `// clang-format on` までを残して、その下にスニペットを貼る
  sed '/^\/\/ clang-format on/q' "$ROOT/acc/cpp/main.cpp" >"$OUT/$name.cpp"
  cat "$name.cpp" >>"$OUT/$name.cpp"
  echo "== $name"
  g++ $FLAGS -O2 "$OUT/$name.cpp" -o "$OUT/$name"
  "$OUT/$name" >/dev/null
  g++ $FLAGS -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
    "$OUT/$name.cpp" -o "$OUT/${name}_san"
  "$OUT/${name}_san" >/dev/null
done
echo "check: ALL OK"
