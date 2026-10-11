// TSP を 2-opt 近傍 + 差分評価の焼きなまし (ahc::anneal) で解く例。
// 最大化前提なので score = -(巡回長)、delta = 旧距離 - 新距離。
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <vector>

#include "../sa.hpp"

namespace {

struct Tour {
  std::vector<int> order;
};

struct Move {
  double delta;
  bool valid;
  int i, j;  // 区間 [i, j] を反転
};

}  // namespace

int main() {
  const int n = 200;
  const double time_ms = ahc::env_double("AHC_TIME_MS", 1000);
  ahc::Rng gen(12345);
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

  Tour tour;
  tour.order.resize(n);
  std::iota(tour.order.begin(), tour.order.end(), 0);
  const double init_len = length(tour.order);

  Tour best = tour;
  ahc::SAConfig cfg;
  cfg.time_limit_ms = time_ms;
  cfg.t0 = 100;  // 平均辺長 ~50 程度に対し序盤は悪化を受理
  cfg.t1 = 1;
  cfg.seed = 1;

  auto propose = [&](const Tour& t, ahc::Rng& rng) {
    Move m{0, false, 0, 0};
    int i = rng.next_int(1, n - 1), j = rng.next_int(1, n - 1);
    if (i == j) return m;
    if (i > j) std::swap(i, j);
    const int a = t.order[i - 1], b = t.order[i], c = t.order[j], d = t.order[(j + 1) % n];
    m.delta = dist(a, b) + dist(c, d) - dist(a, c) - dist(b, d);
    m.valid = true;
    m.i = i;
    m.j = j;
    return m;
  };
  auto apply = [](Tour& t, const Move& m) { std::reverse(t.order.begin() + m.i, t.order.begin() + m.j + 1); };
  auto on_best = [&](const Tour& t, double) { best = t; };

  const ahc::SAStats st = ahc::anneal(tour, -init_len, cfg, propose, apply, on_best);

  // 差分評価の累積誤差を避けるため、最後は正規の評価で検証する。
  std::vector<int> sorted = best.order;
  std::sort(sorted.begin(), sorted.end());
  bool perm_ok = true;
  for (int i = 0; i < n; ++i) perm_ok = perm_ok && sorted[i] == i;
  const double final_len = length(best.order);
  std::fprintf(stderr, "[tsp_sa] init=%.1f final=%.1f ratio=%.3f iters=%lld accepted=%lld improved=%lld elapsed=%.0fms\n",
               init_len, final_len, final_len / init_len, st.iters, st.accepted, st.improved, st.elapsed_ms);
  if (!perm_ok || std::abs(final_len + st.best_score) > 1e-6 * init_len || !(final_len < 0.5 * init_len)) {
    std::fprintf(stderr, "[tsp_sa] SMOKE FAIL\n");
    return 1;
  }
  return 0;
}
