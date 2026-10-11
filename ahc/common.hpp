#pragma once
// AHC 用共通部品: タイマー・乱数・探索結果型。標準ライブラリのみ。
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace ahc {

// マルチシード用のストライド (seed_i = base + i * kSeedStride)。
inline constexpr uint64_t kSeedStride = 0x9E3779B97F4A7C15ull;

// steady_clock ベースのタイマー。構築時 (または reset 時) からの経過 ms。
class Timer {
 public:
  Timer() { reset(); }
  void reset() { start_ = std::chrono::steady_clock::now(); }
  double elapsed_ms() const {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_).count();
  }

 private:
  std::chrono::steady_clock::time_point start_;
};

// xorshift64。seed は splitmix64 で撹拌するので seed=0 でも安全。
class Rng {
 public:
  using result_type = uint64_t;
  explicit Rng(uint64_t seed = 1) { reseed(seed); }

  void reseed(uint64_t seed) {
    uint64_t z = seed + kSeedStride;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    state_ = z != 0 ? z : 0x2545F4914F6CDD1Dull;
  }

  uint64_t next_u64() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 7;
    state_ ^= state_ << 17;
    return state_;
  }

  // [0, n) の整数 (Lemire の乗算法、棄却つきで偏りなし)。n >= 1 が前提。
  int next_int(int n) {
    const uint64_t un = static_cast<uint64_t>(n);
    __uint128_t m = static_cast<__uint128_t>(next_u64()) * un;
    uint64_t low = static_cast<uint64_t>(m);
    if (low < un) {
      const uint64_t threshold = (0 - un) % un;
      while (low < threshold) {
        m = static_cast<__uint128_t>(next_u64()) * un;
        low = static_cast<uint64_t>(m);
      }
    }
    return static_cast<int>(m >> 64);
  }

  // [lo, hi] の整数。
  int next_int(int lo, int hi) { return lo + next_int(hi - lo + 1); }

  // [0, 1) の実数 (53bit)。
  double next_double() { return static_cast<double>(next_u64() >> 11) * 0x1.0p-53; }

  // std::uniform_random_bit_generator 互換 (std::shuffle 等に渡せる)。
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return ~static_cast<result_type>(0); }
  result_type operator()() { return next_u64(); }

 private:
  uint64_t state_ = 1;
};

// 環境変数から実数を読む (未設定/不正ならデフォルト)。examples の時間短縮用。
inline double env_double(const char* name, double default_value) {
  const char* s = std::getenv(name);
  if (s == nullptr || *s == '\0') return default_value;
  char* end = nullptr;
  const double v = std::strtod(s, &end);
  return (end != s) ? v : default_value;
}

// beam / chokudai 共通の探索結果。
// State::Action は beam_search / chokudai_search が要求する型。
template <class State>
struct SearchResult {
  std::vector<typename State::Action> actions;  // 初期状態から最終状態までの行動列
  State final_state;
  double score;       // final_state.eval()
  int depth;          // actions.size()
  long long expanded; // expand() を呼んだノード数
  bool timed_out;
};

}  // namespace ahc
