// グリッド上の得点回収問題で、貪欲 (width=1 のビーム) / ビーム / chokudai を比較する例。
// N x N グリッドの (0,0) から T 手、上下左右に移動し、初回訪問マスの得点を獲得。総得点最大化。
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "../beam.hpp"
#include "../chokudai.hpp"

namespace {

constexpr int kN = 20;
constexpr int kT = 100;
constexpr int kDx[4] = {0, 1, 0, -1};
constexpr int kDy[4] = {1, 0, -1, 0};

struct Problem {
  int cell[kN * kN];
  uint64_t zob_pos[kN * kN];
  uint64_t zob_visit[kN * kN];
  uint64_t zob_turn[kT + 1];
};

Problem make_problem(uint64_t seed) {
  ahc::Rng rng(seed);
  Problem p{};
  for (int i = 0; i < kN * kN; ++i) {
    p.cell[i] = rng.next_int(1, 99);
    p.zob_pos[i] = rng.next_u64();
    p.zob_visit[i] = rng.next_u64();
  }
  for (int t = 0; t <= kT; ++t) p.zob_turn[t] = rng.next_u64();
  return p;
}

const Problem* g_prob = nullptr;

struct State {
  using Action = int;  // 移動方向 0..3
  int pos = 0;
  int turn = 0;
  int score = 0;
  uint64_t h = 0;  // Zobrist: 位置 ^ 訪問集合 ^ 手数
  std::bitset<kN * kN> visited;

  static State initial() {
    State s;
    s.visited.set(0);
    s.score = g_prob->cell[0];
    s.h = g_prob->zob_pos[0] ^ g_prob->zob_visit[0] ^ g_prob->zob_turn[0];
    return s;
  }
  double eval() const { return score; }
  uint64_t hash() const { return h; }
  bool is_done() const { return turn >= kT; }

  template <class Emit>
  void expand(Emit&& emit) const {
    const int x = pos / kN, y = pos % kN;
    for (int d = 0; d < 4; ++d) {
      const int nx = x + kDx[d], ny = y + kDy[d];
      if (nx < 0 || nx >= kN || ny < 0 || ny >= kN) continue;
      const int np = nx * kN + ny;
      State c = *this;
      c.h ^= g_prob->zob_pos[pos] ^ g_prob->zob_pos[np] ^ g_prob->zob_turn[turn] ^ g_prob->zob_turn[turn + 1];
      c.pos = np;
      c.turn = turn + 1;
      if (!c.visited[np]) {
        c.visited.set(np);
        c.score += g_prob->cell[np];
        c.h ^= g_prob->zob_visit[np];
      }
      emit(d, std::move(c));
    }
  }
};

// 行動列をリプレイして得点を再計算 (探索内部の eval とは独立した検証)。
int replay_score(const std::vector<int>& acts) {
  std::bitset<kN * kN> vis;
  int x = 0, y = 0, sum = g_prob->cell[0];
  vis.set(0);
  for (int d : acts) {
    x += kDx[d];
    y += kDy[d];
    if (x < 0 || x >= kN || y < 0 || y >= kN) return -1;
    if (!vis[x * kN + y]) {
      vis.set(x * kN + y);
      sum += g_prob->cell[x * kN + y];
    }
  }
  return sum;
}

}  // namespace

int main() {
  const Problem prob = make_problem(2024);
  g_prob = &prob;
  const State init = State::initial();

  ahc::BeamConfig greedy_cfg;
  greedy_cfg.width = 1;
  greedy_cfg.max_depth = kT;
  const auto greedy = ahc::beam_search(init, greedy_cfg);

  ahc::BeamConfig beam_cfg;
  beam_cfg.width = 500;
  beam_cfg.max_depth = kT;
  ahc::Timer timer;
  const auto beam = ahc::beam_search(init, beam_cfg);
  const double beam_ms = timer.elapsed_ms();

  // group_cap: 現在位置ごとに最大 20 件 (多様性確保)。group_key は state を受けて整数を返す関数。
  ahc::BeamConfig grp_cfg = beam_cfg;
  grp_cfg.group_cap = 20;
  const auto grp = ahc::beam_search(init, grp_cfg, [](const State& s) { return static_cast<uint64_t>(s.pos); });

  // chokudai はビームと同程度の時間 (AHC_TIME_MS で上書き可)。
  ahc::ChokudaiConfig ck_cfg;
  ck_cfg.max_depth = kT;
  ck_cfg.width_per_iter = 1;
  ck_cfg.time_limit_ms = ahc::env_double("AHC_TIME_MS", beam_ms < 50 ? 50 : beam_ms);
  const auto ck = ahc::chokudai_search(init, ck_cfg);

  const int g_re = replay_score(greedy.actions), b_re = replay_score(beam.actions), c_re = replay_score(ck.actions);
  const int p_re = replay_score(grp.actions);
  std::fprintf(stderr, "[grid_beam] greedy=%d beam(w=500)=%d (%.0fms) beam+group_cap(20/pos)=%d chokudai=%d (%.0fms, expanded=%lld)\n",
               g_re, b_re, beam_ms, p_re, c_re, ck_cfg.time_limit_ms, ck.expanded);
  const bool consistent = g_re == static_cast<int>(greedy.score) && b_re == static_cast<int>(beam.score) &&
                          c_re == static_cast<int>(ck.score) &&
                          p_re == static_cast<int>(grp.score) && grp.depth == kT && greedy.depth == kT && beam.depth == kT && ck.depth == kT;
  if (!consistent || b_re < g_re) {
    std::fprintf(stderr, "[grid_beam] SMOKE FAIL\n");
    return 1;
  }
  return 0;
}
