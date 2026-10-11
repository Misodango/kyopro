# templates/ahc — AtCoder Heuristic Contest 用テンプレート

AHC でそのまま使える header-only の C++20 テンプレート集（焼きなまし / ビームサーチ / chokudai サーチ）。
procon37 の solver core・ヘッダ・CMake には一切依存しない（このディレクトリだけで完結する）。
標準ライブラリのみ、名前空間は `ahc`、`g++ -std=c++20 -O2 -Wall -Wextra -Wshadow` で警告ゼロ。

## ファイル一覧

| ファイル | 内容 |
|---|---|
| `common.hpp` | `Timer`, `Rng`（xorshift64 + splitmix64 初期化）, `kSeedStride`, `env_double`, `SearchResult` |
| `sa.hpp` | 焼きなまし `anneal`（差分評価型）/ `anneal_undo`（適用・巻き戻し型）。**最大化前提** |
| `beam.hpp` | ビームサーチ `beam_search`（行動履歴から復元、dedup、group_cap） |
| `chokudai.hpp` | chokudai サーチ `chokudai_search`（深さ別 priority_queue） |
| `bundle.py` | `#include "..."` を展開して単一ファイル化するバンドラ |
| `examples/` | `tsp_sa`, `tsp_sa_undo`, `grid_beam`, `main_template`（雛形） |
| `build_examples.sh` | 全 examples を `-Werror` でビルド・実行・スモーク検証 + sanitizer + bundle 検証 |

```bash
bash templates/ahc/build_examples.sh      # 出力は templates/ahc/build/（.gitignore 済み）
AHC_TIME_MS=200 ./build/tsp_sa            # examples は AHC_TIME_MS で時間を短縮できる
```

## 焼きなまし (`sa.hpp`)

最大化前提。最小化問題は **score と delta の符号を反転** して渡す（例: score = -巡回長、delta = 旧 - 新）。

```cpp
#include "sa.hpp"

struct Move { double delta; bool valid; int i, j; };   // delta と valid は必須

ahc::SAConfig cfg;
cfg.time_limit_ms = 1800;          // この呼び出しに割り当てる時間（呼び出し時点から）
cfg.t0 = 100; cfg.t1 = 1;          // 開始/終了温度（スコア差の単位）
cfg.seed = 1;
// cfg.max_iters = 1'000'000;      // >0 なら反復数で打ち切り + 決定的

State best = cur;
auto propose = [&](const State& s, ahc::Rng& rng) -> Move { /* 近傍を選び delta を計算 */ };
auto apply   = [](State& s, const Move& m) { /* 状態へ反映 */ };
auto on_best = [&](const State& s, double score) { best = s; };
ahc::SAStats st = ahc::anneal(cur, cur_score, cfg, propose, apply, on_best);
```

- `valid=false` は反復数に数えるが受理判定はしない（rng も消費しない）。
- 受理: `delta >= 0 || rng.next_double() < exp(delta / T)`。実装は同値の `log(u) * T < delta`。
- 温度は `time_check_interval`（既定 256）反復ごとに時計を読んだ時だけ再計算する。
- `on_best` は best 更新時のみ（初期解は対象外。初期 best はユーザ側で保存しておく）。
- `SAStats::improved` は best 更新回数。

適用・巻き戻し型は、状態を直接書き換えた方が書きやすい / 差分計算が難しい問題向け。

```cpp
auto try_move = [&](State& s, ahc::Rng& rng) -> std::optional<double> { /* 書き換えて delta を返す。無変更なら nullopt */ };
auto undo     = [&](State& s) { /* 直前の try_move を取り消す */ };
ahc::anneal_undo(cur, cur_score, cfg, try_move, undo, on_best);
```

## ビームサーチ (`beam.hpp`)

State に要求する仕様（concept ではなく duck typing）:

```cpp
struct State {
  using Action = int;                 // デフォルト構築可能
  double eval() const;                // 大きいほど良い
  uint64_t hash() const;              // 同一局面の重複除去用（Zobrist 推奨）
  bool is_done() const;               // 終端
  template <class Emit> void expand(Emit&& emit) const;  // emit(Action, State&&) で子を列挙
};

ahc::BeamConfig cfg;
cfg.width = 1000; cfg.max_depth = 100;
cfg.time_limit_ms = 1800;             // >0 なら時間切れで打ち切り、その層のベストを返す
cfg.dedup = true;                     // 同一 hash は eval 最大の 1 つだけ残す
cfg.group_cap = 50;                   // >0 なら group_key ごとに最大 50 件
auto res = ahc::beam_search(init, cfg, [](const State& s) { return (uint64_t)s.pos; });  // group_key は省略可
// res.actions / res.final_state / res.score / res.depth / res.expanded / res.timed_out
```

- 終端に達した子は「完了候補」として別途保持され、ビーム枠を消費しない。最終的に eval 最大の完了ノード（なければ最終層の最良ノード）を返す。
- 履歴は層ごとの `(parent, action)` だけを保持し、最後に逆順で復元する。State 全体は現在層にしか持たない。
- `group_cap` 有効時は eval 降順に貪欲に詰める。`group_key` を省略した場合 `group_cap` は無視される。
- 同値 eval の並びは候補の生成順で決まる（決定的）。

## chokudai サーチ (`chokudai.hpp`)

State 要件は beam と同じ。結果型も `BeamResult`（= `SearchResult`）と同じ。

```cpp
ahc::ChokudaiConfig cfg;
cfg.width_per_iter = 1;       // 1 周あたり各深さで展開する個数
cfg.max_depth = 100;
cfg.time_limit_ms = 1800;     // 0 以下だと即終了する
// cfg.max_rounds = 1000;     // >0 なら周回数で打ち切り（時間非依存で再現したいとき）
auto res = ahc::chokudai_search(init, cfg);
```

- 深さごとに eval 最大優先のヒープを持ち、`dedup` 時は一度展開した hash を再展開しない（`hash()` に手数を含めること）。
- プールには `(parent, action, eval)` のみ、State はヒープ内にだけ持つ。ノード数が増えるとメモリを食う（枝刈りは未実装）。

## バンドル (`bundle.py`)

AtCoder は単一ファイル提出なので、ローカル include を展開する。

```bash
python3 templates/ahc/bundle.py main.cpp -o submit.cpp [-I dir ...]
```

- `#include "xxx.hpp"` を include 元のディレクトリ → `-I` の順に解決して再帰展開。
- `#pragma once` を持つファイルは 2 回目以降スキップし、展開時に `#pragma once` 行を除く。
- `#include <...>` はそのまま残す（重複行が残ってよい）。見つからない include / 循環 include はエラー終了。

## procon37 から持ち込んだ教訓

- **幾何冷却をデフォルト**にする: 線形より終盤の低温域に長く滞在でき、局所改善の仕上げに時間を使える。`Schedule::Linear` は比較用。
- **t0 / t1 は典型的な delta の大きさから決める**: t0 ≈ 序盤に悪化を受理したい delta 幅、t1 ≈ 最小改善単位程度。まず近傍の delta をログに出して分布を見る。
- **seed 注入と `max_iters` で再現性**を確保する。時間打ち切りだと環境差で結果が変わるので、比較・回帰テストは `max_iters` で行う。
- **時計読みは間引く**（`time_check_interval`）。温度更新もそのタイミングだけ。毎反復 `pow` / `steady_clock::now` を呼ばない。
- **内部の高速評価で選んだ best は、最後に正規の評価で検証する**（`examples/tsp_sa.cpp` は最終長を一から再計算し、`best_score` と突き合わせている）。差分更新の累積誤差やバグをここで捕まえる。
- **ビームは dedup と group_cap で多様性を確保**する。procon37 のビームは local / potential / diversity の 3 レーンに 1/2・1/4・1/4 の枠を配分している（同様のレーン分けは、eval を複数種類用意して各レーンの上位を連結すれば実現できる）。
- **時間制限には余裕を持たせる**（例: TL - 100ms）。入力読み込み・出力・最後の検証の時間を差し引いて `time_limit_ms` を渡す。
- 設定の採否は 1 試合ではなく、多数ケース（可能なら seed を `base + i * kSeedStride` で振る）の平均と勝敗数で判定する。

## 今後の拡張候補

- 差分更新ビーム（State をコピーせず、操作の適用/巻き戻しで DFS 的に展開）/ Euler tour による木ビーム
- 多点スタート（複数 seed で SA を回して最良を採用、または途中で best から再開）
- 並列シード（AHC では不可な場合が多いが、ローカル検証用に複数 seed を並列実行）
- chokudai のプールの枝刈り・State のコンパクト保持
- ビーム選別の複数レーン化（local / potential / diversity）を `beam_search` の設定として組み込む
