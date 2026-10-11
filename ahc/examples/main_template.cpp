// AHC 用の雛形。提出時は `python3 bundle.py main_template.cpp -o submit.cpp` で単一ファイル化する。
#include <iostream>
#include <vector>

#include "../sa.hpp"

// ---- 入力 (問題ごとに書き換える) ----
struct Input {
  int n = 10;
};

Input read_input() {
  Input in;
  if (!(std::cin >> in.n)) in.n = 10;  // 入力がなくても動くようにしている (雛形用)
  return in;
}

// ---- 解の状態 (問題ごとに書き換える) ----
struct State {
  std::vector<int> x;  // 例: 0/1 の列
};

// スコア (大きいほど良い)。最小化問題なら符号を反転する。
double full_score(const Input&, const State& s) {
  double sc = 0;
  for (int v : s.x) sc += v;  // 例: 1 の個数を最大化
  return sc;
}

// ---- SA の近傍 ----
struct Move {
  double delta;
  bool valid;
  int idx;
};

int main() {
  ahc::Timer timer;  // 起動直後に開始
  const Input in = read_input();
  const double time_limit_ms = ahc::env_double("AHC_TIME_MS", 1900);  // 本番は TL - 余裕

  State cur;
  cur.x.assign(in.n, 0);
  State best = cur;
  const double init_score = full_score(in, cur);

  ahc::SAConfig cfg;
  cfg.time_limit_ms = time_limit_ms - timer.elapsed_ms();
  cfg.t0 = 1.0;
  cfg.t1 = 0.1;
  cfg.seed = 1;

  auto propose = [&](const State& s, ahc::Rng& rng) {
    const int i = rng.next_int(in.n);
    // 1 反転の差分: 0 -> 1 なら +1、1 -> 0 なら -1
    return Move{s.x[i] == 0 ? 1.0 : -1.0, true, i};
  };
  auto apply = [](State& s, const Move& m) { s.x[m.idx] ^= 1; };
  auto on_best = [&](const State& s, double) { best = s; };

  const ahc::SAStats st = ahc::anneal(cur, init_score, cfg, propose, apply, on_best);

  // 最後に正規のスコアで検証する
  std::cerr << "score=" << full_score(in, best) << " (SA best=" << st.best_score << ") iters=" << st.iters << "\n";
  for (int v : best.x) std::cout << v << "\n";
  return 0;
}
