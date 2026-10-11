#pragma once
// ビームサーチ (行動履歴から復元する方式)。
//
// State に要求する仕様 (concept ではなく duck typing):
//   using Action = ...;                       // デフォルト構築可能であること
//   double   eval() const;                    // 大きいほど良い (ビーム選別用)
//   uint64_t hash() const;                    // 同一局面の重複除去用 (dedup=true のとき使う)
//   bool     is_done() const;                 // 終端か
//   template <class Emit> void expand(Emit&& emit) const;  // emit(Action, State&&) で子を列挙
// State はムーブ構築可能であること。終端でない子だけが次の層に残る。
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common.hpp"

namespace ahc {

struct BeamConfig {
  int width = 1000;
  int max_depth = 1000;
  double time_limit_ms = 0;  // >0 なら時間切れ時点で打ち切り、その層のベストを返す
  bool dedup = true;         // 同一 hash は eval 最大の 1 つだけ残す
  int group_cap = 0;         // >0 なら group_key(state) ごとに最大 group_cap 件 (多様性確保、group_key 指定時のみ)
};

template <class State>
using BeamResult = SearchResult<State>;

// group_key を渡さないとき用のダミー。この型のときは group_cap を無視する。
struct NoGroupKey {
  template <class S>
  uint64_t operator()(const S&) const {
    return 0;
  }
};

template <class State, class GroupKey = NoGroupKey>
BeamResult<State> beam_search(const State& init, const BeamConfig& cfg, GroupKey group_key = {}) {
  using Action = typename State::Action;
  struct Node {
    State st;
    int parent;  // 1 つ前の層での添字
    Action act;
    double ev;
    uint64_t h;
  };
  struct Hist {
    int parent;
    Action act;
  };

  Timer timer;
  const int width = std::max(1, cfg.width);
  const bool timed = cfg.time_limit_ms > 0;

  if (init.is_done()) {
    return BeamResult<State>{{}, init, init.eval(), 0, 0, false};
  }

  std::vector<Node> cur;
  cur.push_back(Node{init, -1, Action{}, init.eval(), 0});
  // hist[d][i] = d 層目 i 番目ノードの (親, 行動)。hist[0] は根のダミー。
  std::vector<std::vector<Hist>> hist;
  hist.push_back({Hist{-1, Action{}}});

  // 終端に達したノードのうち eval 最大のもの (親の位置と最後の行動で復元する)。
  struct Finished {
    State st;
    double ev;
    int parent_depth;
    int parent_idx;
    Action act;
  };
  std::optional<Finished> fin;

  auto restore = [&](int depth, int idx) {
    std::vector<Action> acts;
    for (int d = depth; d >= 1; --d) {
      acts.push_back(hist[d][idx].act);
      idx = hist[d][idx].parent;
    }
    std::reverse(acts.begin(), acts.end());
    return acts;
  };

  long long expanded = 0;
  bool timed_out = false;
  int depth = 0;  // cur が何層目か (= そこまでの行動数)
  std::vector<Node> cand;

  for (; depth < cfg.max_depth && !cur.empty(); ++depth) {
    cand.clear();
    for (int i = 0; i < static_cast<int>(cur.size()); ++i) {
      if (timed && timer.elapsed_ms() >= cfg.time_limit_ms) {
        timed_out = true;
        break;
      }
      ++expanded;
      cur[i].st.expand([&](Action a, State&& child) {
        const double ev = child.eval();
        if (child.is_done()) {
          if (!fin || ev > fin->ev) fin.emplace(Finished{std::move(child), ev, depth, i, a});
          return;
        }
        const uint64_t h = cfg.dedup ? child.hash() : 0;
        cand.push_back(Node{std::move(child), i, a, ev, h});
      });
    }
    if (timed_out) break;
    if (cand.empty()) break;  // 子なし / 全て終端: cur を最終層として残す

    // 選別: dedup -> (group_cap | nth_element) -> 上位 width。
    std::vector<int> idx;
    idx.reserve(cand.size());
    if (cfg.dedup) {
      std::unordered_map<uint64_t, int> best_of;  // hash -> 暫定で残す候補の添字
      best_of.reserve(cand.size() * 2);
      std::vector<char> dead(cand.size(), 0);
      for (int i = 0; i < static_cast<int>(cand.size()); ++i) {
        auto [it, inserted] = best_of.emplace(cand[i].h, i);
        if (inserted) continue;
        if (cand[i].ev > cand[it->second].ev) {
          dead[it->second] = 1;
          it->second = i;
        } else {
          dead[i] = 1;
        }
      }
      for (int i = 0; i < static_cast<int>(cand.size()); ++i) {
        if (!dead[i]) idx.push_back(i);
      }
    } else {
      idx.resize(cand.size());
      std::iota(idx.begin(), idx.end(), 0);
    }

    // eval 降順 (同値は添字昇順で決定的に)
    auto better = [&](int a, int b) {
      if (cand[a].ev != cand[b].ev) return cand[a].ev > cand[b].ev;
      return a < b;
    };
    constexpr bool has_group_key = !std::is_same_v<GroupKey, NoGroupKey>;
    if (has_group_key && cfg.group_cap > 0) {
      std::sort(idx.begin(), idx.end(), better);
      std::unordered_map<uint64_t, int> cnt;
      std::vector<int> picked;
      picked.reserve(std::min<size_t>(idx.size(), width));
      for (int i : idx) {
        if (static_cast<int>(picked.size()) >= width) break;
        int& c = cnt[static_cast<uint64_t>(group_key(cand[i].st))];
        if (c >= cfg.group_cap) continue;
        ++c;
        picked.push_back(i);
      }
      idx.swap(picked);
    } else if (static_cast<int>(idx.size()) > width) {
      std::nth_element(idx.begin(), idx.begin() + width, idx.end(), better);
      idx.resize(width);
      std::sort(idx.begin(), idx.end(), better);
    } else {
      std::sort(idx.begin(), idx.end(), better);
    }

    std::vector<Node> next;
    next.reserve(idx.size());
    std::vector<Hist> layer;
    layer.reserve(idx.size());
    for (int i : idx) {
      layer.push_back(Hist{cand[i].parent, cand[i].act});
      next.push_back(std::move(cand[i]));
    }
    hist.push_back(std::move(layer));
    cur.swap(next);
  }

  // 結果の決定: 完了ノードがあれば eval 最大のもの、なければ最終層の最良ノード。
  if (fin) {
    std::vector<Action> acts = restore(fin->parent_depth, fin->parent_idx);
    acts.push_back(fin->act);
    const int d = static_cast<int>(acts.size());
    const double score = fin->ev;
    return BeamResult<State>{std::move(acts), std::move(fin->st), score, d, expanded, timed_out};
  }
  int best = 0;
  for (int i = 1; i < static_cast<int>(cur.size()); ++i) {
    if (cur[i].ev > cur[best].ev) best = i;
  }
  const int d = static_cast<int>(hist.size()) - 1;
  const double score = cur[best].ev;
  return BeamResult<State>{restore(d, best), std::move(cur[best].st), score, d, expanded, timed_out};
}

}  // namespace ahc
