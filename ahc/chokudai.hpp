#pragma once
// chokudai サーチ。深さごとの優先度付きキューを持ち、時間内に
// 「各深さで上位 width_per_iter 個を取り出して展開し、次の深さへ push」を繰り返す。
// State の要件は beam.hpp と同じ (Action / eval / hash / is_done / expand)。
#include <algorithm>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "beam.hpp"
#include "common.hpp"

namespace ahc {

struct ChokudaiConfig {
  int width_per_iter = 1;
  int max_depth = 1000;
  double time_limit_ms = 1000;  // 必須相当: 0 以下だと即終了する
  bool dedup = true;            // 一度展開した hash は再展開しない
  long long max_rounds = 0;     // >0 なら周回数で打ち切り (時間非依存の再現用)
};

template <class State>
BeamResult<State> chokudai_search(const State& init, const ChokudaiConfig& cfg) {
  using Action = typename State::Action;
  // プールには経路復元用の (親, 行動, eval) だけを置き、State はキューにのみ持たせる。
  struct PoolNode {
    int parent;
    Action act;
    double ev;
  };
  struct Entry {
    State st;
    double ev;
    int node;  // プール上の添字
  };
  // 優先度: eval 大、同値ならプール添字が小さい方 (先に作られた方)。
  auto less = [](const Entry& a, const Entry& b) {
    if (a.ev != b.ev) return a.ev < b.ev;
    return a.node > b.node;
  };

  Timer timer;
  if (init.is_done()) {
    return BeamResult<State>{{}, init, init.eval(), 0, 0, false};
  }

  const int max_depth = std::max(1, cfg.max_depth);
  const int wpi = std::max(1, cfg.width_per_iter);
  std::vector<PoolNode> pool;
  pool.push_back(PoolNode{-1, Action{}, init.eval()});
  std::vector<std::vector<Entry>> heaps(max_depth);  // heaps[d]: d 手後の状態
  heaps[0].push_back(Entry{init, pool[0].ev, 0});
  std::unordered_set<uint64_t> expanded_hash;

  // 完了ノードの最良 / 未完了ノードの (最深, eval 最大)。State のコピーは更新時のみ。
  struct Best {
    State st;
    double ev;
    int node;
    int depth;
  };
  std::optional<Best> fin, open;

  long long expanded = 0, rounds = 0;
  bool timed_out = false;
  auto time_up = [&] { return timer.elapsed_ms() >= cfg.time_limit_ms; };

  while (true) {
    if (cfg.max_rounds > 0 && rounds >= cfg.max_rounds) break;
    if (time_up()) {
      timed_out = true;
      break;
    }
    bool progressed = false;
    for (int d = 0; d < max_depth && !timed_out; ++d) {
      auto& heap = heaps[d];
      for (int taken = 0; taken < wpi && !heap.empty();) {
        if (time_up()) {
          timed_out = true;
          break;
        }
        std::pop_heap(heap.begin(), heap.end(), less);
        Entry e = std::move(heap.back());
        heap.pop_back();
        if (cfg.dedup && !expanded_hash.insert(e.st.hash()).second) continue;  // 展開済みは数えない
        ++taken;
        ++expanded;
        progressed = true;
        e.st.expand([&](Action a, State&& child) {
          const double ev = child.eval();
          const int id = static_cast<int>(pool.size());
          pool.push_back(PoolNode{e.node, a, ev});
          if (child.is_done()) {
            if (!fin || ev > fin->ev) fin.emplace(Best{std::move(child), ev, id, d + 1});
          } else if (d + 1 < max_depth) {
            if (!open || d + 1 > open->depth || (d + 1 == open->depth && ev > open->ev)) {
              open.emplace(Best{child, ev, id, d + 1});
            }
            heaps[d + 1].push_back(Entry{std::move(child), ev, id});
            std::push_heap(heaps[d + 1].begin(), heaps[d + 1].end(), less);
          } else if (!open || d + 1 > open->depth || (d + 1 == open->depth && ev > open->ev)) {
            open.emplace(Best{std::move(child), ev, id, d + 1});  // max_depth 到達の未完了ノード
          }
        });
      }
    }
    ++rounds;
    if (timed_out || !progressed) break;
  }

  auto restore = [&](int node) {
    std::vector<Action> acts;
    for (; pool[node].parent >= 0; node = pool[node].parent) acts.push_back(pool[node].act);
    std::reverse(acts.begin(), acts.end());
    return acts;
  };
  // 完了ノードがあればその最良、なければ最も深い未完了ノードの最良。
  if (fin) {
    const int d = fin->depth;
    return BeamResult<State>{restore(fin->node), std::move(fin->st), fin->ev, d, expanded, timed_out};
  }
  if (open) {
    const int d = open->depth;
    return BeamResult<State>{restore(open->node), std::move(open->st), open->ev, d, expanded, timed_out};
  }
  return BeamResult<State>{{}, init, init.eval(), 0, expanded, timed_out};
}

}  // namespace ahc
