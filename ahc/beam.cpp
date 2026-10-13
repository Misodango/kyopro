// ビームサーチ (acc の main.cpp の `// clang-format on` より下に貼る)
// 例題: N x N グリッドの (0, 0) から T 手で上下左右に動き、
//       初めて踏んだマスの得点を集める。「問題ごと」の所を書き換える。
// 候補は (評価値, hash, 親, 行動) だけで作り、残ったものだけ状態を作る。
constexpr double TL = 1900; // ms

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

Timer timer;
Rng rng;

// ---- 入力 (問題ごと) ----
constexpr int N = 20, T = 100;
constexpr int dx[4] = {0, 1, 0, -1}, dy[4] = {1, 0, -1, 0};
int cell[N * N];
ull zobPos[N * N], zobVis[N * N]; // Zobrist hash 用の乱数
void input() {
  // 例: 本番は cin で読む
  rep(i, N * N) cell[i] = rng.nextInt(1, 99);
  rep(i, N * N) zobPos[i] = rng.next(), zobVis[i] = rng.next();
}

// ---- 状態 (問題ごと) ----
struct State {
  int pos = 0;
  ll score = 0;
  ull hash = 0; // 同じ局面を同じ値にする (Zobrist)
  bitset<N * N> vis;
};

State initState() {
  State s;
  s.vis[0] = 1;
  s.score = cell[0];
  s.hash = zobPos[0] ^ zobVis[0];
  return s;
}

// s から行動 d を取れるならその先のマス、取れないなら -1
int nextPos(const State &s, int d) {
  int x = s.pos / N + dx[d], y = s.pos % N + dy[d];
  if (x < 0 || x >= N || y < 0 || y >= N)
    return -1;
  return x * N + y;
}

void applyAction(State &s, int d) {
  int np = nextPos(s, d);
  s.hash ^= zobPos[s.pos] ^ zobPos[np];
  s.pos = np;
  if (!s.vis[np]) {
    s.vis[np] = 1;
    s.score += cell[np];
    s.hash ^= zobVis[np];
  }
}

// ---- ビームサーチ ----
struct Cand {
  ll eval; // ビームで比べる値 (大きいほど良い)
  ull hash;
  int parent; // cur での添字
  int act;
};

vector<pii> hist; // hist[id] = (親の id, 行動)。行動列の復元用

vi restore(int id) {
  vi acts;
  for (; hist[id].fi >= 0; id = hist[id].fi)
    acts.pb(hist[id].se);
  reverse(all(acts));
  return acts;
}

vi beamSearch(int width) {
  hist = {{-1, -1}};
  vector<pair<State, int>> cur = {{initState(), 0}}; // (状態, hist の id)
  rep(turn, T) {
    // 候補を列挙する (問題ごと)。状態はまだ作らない
    vector<Cand> cand;
    rep(i, cur.size()) {
      const State &s = cur[i].fi;
      rep(d, 4) {
        int np = nextPos(s, d);
        if (np < 0)
          continue;
        ll eval = s.score;
        ull hash = s.hash ^ zobPos[s.pos] ^ zobPos[np];
        if (!s.vis[np])
          eval += cell[np], hash ^= zobVis[np];
        cand.pb({eval, hash, (int)i, (int)d});
      }
    }
    if (cand.empty())
      break;

    // 評価値の降順に、同じ hash を除きながら width 個まで残す
    sort(all(cand),
         [](const Cand &a, const Cand &b) { return a.eval > b.eval; });
    unordered_set<ull> seen;
    vector<pair<State, int>> nxt;
    for (const Cand &c : cand) {
      if ((int)nxt.size() >= width)
        break;
      if (!seen.insert(c.hash).se)
        continue;
      State s = cur[c.parent].fi;
      applyAction(s, c.act);
      hist.pb({cur[c.parent].se, c.act});
      nxt.pb({s, (int)hist.size() - 1});
    }
    cur.swap(nxt);
    if (timer.ms() > TL) // 時間切れならこの層の最良を返す
      break;
  }
  int best = 0;
  rep(i, cur.size()) if (cur[i].fi.score > cur[best].fi.score) best = i;
  return restore(cur[best].se);
}

// 行動列を最初からなぞってスコアを出す (探索とは別に検証する)
ll replay(const vi &acts) {
  State s = initState();
  for (int d : acts) {
    if (nextPos(s, d) < 0)
      return -1;
    applyAction(s, d);
  }
  return s.score;
}

int main() {
  cin.tie(0)->sync_with_stdio(0);
  input();
  ll greedy = replay(beamSearch(1)); // width = 1 は貪欲
  vi acts = beamSearch(500);
  ll score = replay(acts);
  cerr << "greedy=" << greedy << " beam=" << score << " time=" << timer.ms()
       << "ms" << endl;
  assert((int)acts.size() == T && score >= greedy);
  for (int d : acts)
    cout << "RDLU"[d];
  cout << '\n';
}
