// 出典: https://github.com/Misodango/kyopro/blob/main/ahc/sa.cpp
// 焼きなまし (acc の main.cpp の `// clang-format on` より下に貼る)
// 例題: ランダムな点の TSP を 2-opt で解く。「問題ごと」の所を書き換える。
// 最大化前提。最小化なら score と delta の符号を反転する (例: score = -巡回長)。
// 環境変数 P_<name> があればその値、なければ def を返す (ジャッジ上は常に def)
// 例: P_T0=50 ./a.out。run.py / tune.py はこれで値を変えて回す
double param(const char *name, double def) {
  const char *v = getenv(("P_" + string(name)).c_str());
  return v ? atof(v) : def;
}

const double TL = param("TL", 1900); // ms。入出力の時間を見込んで TL より少し短く

struct Timer {
  chrono::steady_clock::time_point st = chrono::steady_clock::now();
  double ms() const {
    return chrono::duration<double, milli>(chrono::steady_clock::now() - st)
        .count();
  }
};

struct Rng {
  ull x = 88172645463325252ULL;
  ull next() {
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return x;
  }
  int nextInt(int n) { return next() % n; }                    // [0, n)
  int nextInt(int l, int r) { return l + nextInt(r - l + 1); } // [l, r]
  double nextDouble() { return (next() >> 11) * 0x1.0p-53; }   // [0, 1)
};

Timer timer; // グローバルに置いて起動直後から計測する
Rng rng;

// ---- 入力 (問題ごと) ----
int n;
vector<double> px, py;
void input() {
  // 例: 本番は cin で読む
  n = 200;
  px.resize(n), py.resize(n);
  rep(i, n) px[i] = rng.nextDouble() * 1000, py[i] = rng.nextDouble() * 1000;
}

// ---- 状態とスコア (問題ごと) ----
struct State {
  vi ord;
  double score; // 大きいほど良い
};

double dist(int a, int b) { return hypot(px[a] - px[b], py[a] - py[b]); }

// 一から計算するスコア。初期解と最後の検証に使う
double calcScore(const State &s) {
  double len = 0;
  rep(i, n) len += dist(s.ord[i], s.ord[(i + 1) % n]);
  return -len;
}

State initState() {
  State s;
  s.ord.resize(n);
  iota(all(s.ord), 0);
  s.score = calcScore(s);
  return s;
}

// ---- 焼きなまし ----
State solve() {
  State cur = initState(), best = cur;
  // 開始/終了温度。典型的な delta の大きさで決める。tune.py で調整できる
  const double T0 = param("T0", 100), T1 = param("T1", 1);
  const double start = timer.ms();
  double T = T0;
  ll iter = 0, accepted = 0;
  for (;; iter++) {
    // 時計を読むのと温度の更新は 256 回に 1 回にする
    if ((iter & 255) == 0) {
      double t = (timer.ms() - start) / (TL - start);
      if (t >= 1)
        break;
      T = T0 * pow(T1 / T0, t); // 幾何冷却
    }

    // 近傍 (問題ごと): 2-opt で区間 [i, j] を反転
    int i = rng.nextInt(1, n - 1), j = rng.nextInt(1, n - 1);
    if (i == j)
      continue;
    if (i > j)
      swap(i, j);
    int a = cur.ord[i - 1], b = cur.ord[i];
    int c = cur.ord[j], d = cur.ord[(j + 1) % n];
    double delta = dist(a, b) + dist(c, d) - dist(a, c) - dist(b, d);

    if (delta >= 0 || rng.nextDouble() < exp(delta / T)) {
      reverse(cur.ord.begin() + i, cur.ord.begin() + j + 1);
      cur.score += delta;
      accepted++;
      if (cur.score > best.score)
        best = cur; // コピーが重いなら best の保存を間引く
    }
    // 差分を出しにくいときは、書き換えて calcScore し直し、
    // 受理しなかったら元に戻す形にする
  }
  cerr << "iter=" << iter << " accepted=" << accepted << " T=" << T << endl;
  return best;
}

int main() {
  cin.tie(0)->sync_with_stdio(0);
  input();
  double initScore = initState().score;
  State best = solve();
  // 差分の積み重ねでずれていないか、一から計算し直して確かめる
  double score = calcScore(best);
  cerr << "init=" << initScore << " score=" << score << " (sa=" << best.score
       << ") time=" << timer.ms() << "ms" << endl;
  assert(abs(score - best.score) < 1e-6 * abs(initScore));
  rep(i, n) cout << best.ord[i] << '\n';
}
