// TSP を適用・巻き戻し型 (ahc::anneal_undo) で解く例。
// 近傍は swap と 2-opt の混合。どちらも自己逆写像なので undo は同じ操作をもう一度行うだけ。
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <optional>
#include <vector>

#include "../sa.hpp"

int main() {
  const int n = 200;
  const double time_ms = ahc::env_double("AHC_TIME_MS", 1000);
  ahc::Rng gen(777);
  std::vector<double> x(n), y(n);
  for (int i = 0; i < n; ++i) {
    x[i] = gen.next_double() * 1000;
    y[i] = gen.next_double() * 1000;
  }
  auto dist = [&](int a, int b) { return std::hypot(x[a] - x[b], y[a] - y[b]); };
  auto length = [&](const std::vector<int>& o) {
    double s = 0;
    for (int k = 0; k < n; ++k) s += dist(o[k], o[(k + 1) % n]);
    return s;
  };
  // 位置 k と k+1 を結ぶ辺 k のうち、重複を除いた集合の長さの和。
  auto edge_sum = [&](const std::vector<int>& o, const int* edges, int cnt) {
    double s = 0;
    for (int e = 0; e < cnt; ++e) s += dist(o[edges[e]], o[(edges[e] + 1) % n]);
    return s;
  };

  std::vector<int> order(n);
  std::iota(order.begin(), order.end(), 0);
  const double init_len = length(order);
  std::vector<int> best = order;

  struct Last {
    bool is_swap;
    int i, j;
  } last{true, 0, 0};

  auto flip = [&](std::vector<int>& o, const Last& m) {
    if (m.is_swap) {
      std::swap(o[m.i], o[m.j]);
    } else {
      std::reverse(o.begin() + m.i, o.begin() + m.j + 1);
    }
  };
  auto try_move = [&](std::vector<int>& o, ahc::Rng& rng) -> std::optional<double> {
    int i = rng.next_int(0, n - 1), j = rng.next_int(0, n - 1);
    if (i == j) return std::nullopt;
    if (i > j) std::swap(i, j);
    const bool is_swap = rng.next_int(2) == 0;
    if (!is_swap && i == 0) i = 1;  // 反転は位置 0 を動かさない
    if (!is_swap && i >= j) return std::nullopt;
    int edges[4];
    int cnt = 0;
    auto add = [&](int e) {
      e = (e + n) % n;
      for (int k = 0; k < cnt; ++k) {
        if (edges[k] == e) return;
      }
      edges[cnt++] = e;
    };
    if (is_swap) {
      add(i - 1), add(i), add(j - 1), add(j);
    } else {
      add(i - 1), add(j);
    }
    const double before = edge_sum(o, edges, cnt);
    last = {is_swap, i, j};
    flip(o, last);
    return before - edge_sum(o, edges, cnt);
  };
  auto undo = [&](std::vector<int>& o) { flip(o, last); };
  auto on_best = [&](const std::vector<int>& o, double) { best = o; };

  ahc::SAConfig cfg;
  cfg.time_limit_ms = time_ms;
  cfg.t0 = 100;
  cfg.t1 = 1;
  cfg.seed = 2;
  const ahc::SAStats st = ahc::anneal_undo(order, -init_len, cfg, try_move, undo, on_best);

  std::vector<int> sorted = best;
  std::sort(sorted.begin(), sorted.end());
  bool perm_ok = true;
  for (int i = 0; i < n; ++i) perm_ok = perm_ok && sorted[i] == i;
  const double final_len = length(best);
  std::fprintf(stderr, "[tsp_sa_undo] init=%.1f final=%.1f ratio=%.3f iters=%lld accepted=%lld improved=%lld elapsed=%.0fms\n",
               init_len, final_len, final_len / init_len, st.iters, st.accepted, st.improved, st.elapsed_ms);
  if (!perm_ok || std::abs(final_len + st.best_score) > 1e-6 * init_len || !(final_len < 0.5 * init_len)) {
    std::fprintf(stderr, "[tsp_sa_undo] SMOKE FAIL\n");
    return 1;
  }
  return 0;
}
