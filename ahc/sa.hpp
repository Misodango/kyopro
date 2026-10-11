#pragma once
// 焼きなまし (最大化前提。最小化したい場合は score と delta の符号を反転する)。
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>

#include "common.hpp"

namespace ahc {

enum class Schedule { Geometric, Linear };

struct SAConfig {
  double time_limit_ms = 1900;  // この関数に割り当てる時間 (呼び出し時点からの相対)
  double t0 = 100.0, t1 = 1.0;  // 開始/終了温度 (スコア差の単位)
  Schedule schedule = Schedule::Geometric;  // Geometric: T = t0*(t1/t0)^u, Linear: t0+(t1-t0)u
  uint64_t seed = 1;
  // >0 なら反復数で打ち切り。進捗 u も iter/max_iters で計算し、時間は安全弁のみ。
  // seed と max_iters が同じなら結果が決定的になる。
  long long max_iters = 0;
  int time_check_interval = 256;  // 何反復ごとに時計を読み、温度を更新するか
};

struct SAStats {
  long long iters = 0;     // 試行回数 (valid=false / nullopt も含む)
  long long accepted = 0;  // 受理回数
  long long improved = 0;  // best 更新回数 (on_best を呼んだ回数)
  double best_score = 0;
  double final_temp = 0;
  double elapsed_ms = 0;
};

namespace detail {

// 時計と温度の管理。温度は time_check_interval ごとにだけ再計算する。
class SAClock {
 public:
  explicit SAClock(const SAConfig& cfg)
      : cfg_(cfg), interval_(cfg.time_check_interval > 0 ? cfg.time_check_interval : 1), temp_(cfg.t0) {}

  // iter 番目の反復を実行してよいか。実行してよいなら true。
  bool step(long long iter) {
    if (cfg_.max_iters > 0 && iter >= cfg_.max_iters) return false;
    if (iter % interval_ != 0) return true;
    const double elapsed = timer_.elapsed_ms();
    if (elapsed >= cfg_.time_limit_ms) return false;
    const double u = cfg_.max_iters > 0 ? static_cast<double>(iter) / static_cast<double>(cfg_.max_iters)
                                        : elapsed / cfg_.time_limit_ms;
    temp_ = temperature(u);
    return true;
  }

  double temp() const { return temp_; }
  double elapsed_ms() const { return timer_.elapsed_ms(); }

 private:
  double temperature(double u) const {
    if (cfg_.schedule == Schedule::Geometric && cfg_.t0 > 0 && cfg_.t1 > 0) {
      return cfg_.t0 * std::pow(cfg_.t1 / cfg_.t0, u);
    }
    return cfg_.t0 + (cfg_.t1 - cfg_.t0) * u;  // Linear (t0/t1 が非正なら Geometric でもこちら)
  }

  SAConfig cfg_;
  long long interval_;
  Timer timer_;
  double temp_;
};

// 受理判定。rng.next_double() < exp(delta/T) と同値で、log(u)*T < delta と書ける。
inline bool accept(double delta, double temp, Rng& rng) {
  if (delta >= 0) return true;
  if (!(temp > 0)) return false;
  return std::log(rng.next_double()) * temp < delta;
}

}  // namespace detail

// 差分評価型。
//   propose(const State&, Rng&) -> Move   (Move は double delta, bool valid を持つ)
//   apply(State&, const Move&)            (受理時に状態へ反映)
//   on_best(const State&, double score)   (best 更新時。ユーザがコピー保存する)
// 初期解は on_best の対象外 (ユーザ側で初期 best を保存しておくこと)。
template <class State, class Propose, class Apply, class OnBest>
SAStats anneal(State& s, double cur_score, const SAConfig& cfg, Propose propose, Apply apply, OnBest on_best) {
  Rng rng(cfg.seed);
  detail::SAClock clock(cfg);
  SAStats st;
  st.best_score = cur_score;
  for (; clock.step(st.iters); ++st.iters) {
    const State& cs = s;
    auto mv = propose(cs, rng);
    if (!mv.valid) continue;
    if (!detail::accept(mv.delta, clock.temp(), rng)) continue;
    apply(s, mv);
    cur_score += mv.delta;
    ++st.accepted;
    if (cur_score > st.best_score) {
      st.best_score = cur_score;
      ++st.improved;
      on_best(cs, cur_score);
    }
  }
  st.final_temp = clock.temp();
  st.elapsed_ms = clock.elapsed_ms();
  return st;
}

// 適用・巻き戻し型。
//   try_move(State&, Rng&) -> std::optional<double>  (状態を書き換えて delta を返す。nullopt なら無変更)
//   undo(State&)                                      (直前の try_move を取り消す)
template <class State, class Try, class Undo, class OnBest>
SAStats anneal_undo(State& s, double cur_score, const SAConfig& cfg, Try try_move, Undo undo, OnBest on_best) {
  Rng rng(cfg.seed);
  detail::SAClock clock(cfg);
  SAStats st;
  st.best_score = cur_score;
  for (; clock.step(st.iters); ++st.iters) {
    const std::optional<double> d = try_move(s, rng);
    if (!d) continue;
    if (!detail::accept(*d, clock.temp(), rng)) {
      undo(s);
      continue;
    }
    cur_score += *d;
    ++st.accepted;
    if (cur_score > st.best_score) {
      st.best_score = cur_score;
      ++st.improved;
      on_best(std::as_const(s), cur_score);
    }
  }
  st.final_temp = clock.temp();
  st.elapsed_ms = clock.elapsed_ms();
  return st;
}

}  // namespace ahc
