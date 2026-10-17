// chokudai サーチ (acc の main.cpp の `// clang-format on` より下に貼る)
// 例題は beam.cpp と同じグリッドの得点集め。「問題ごと」の所を書き換える。
// 深さごとに優先度付きキューを持ち、時間いっぱい
// 「各深さで上位 WIDTH 個を取り出して展開し、次の深さに積む」を繰り返す。
// 環境変数 P_<name> があればその値、なければ def を返す (ジャッジ上は常に def)
double param(const char *name, double def) {
  const char *v = getenv(("P_" + string(name)).c_str());
  return v ? atof(v) : def;
}

const double TL = param("TL", 1900); // ms

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
ull zobPos[N * N], zobVis[N * N], zobTurn[T + 1];
void input() {
  // 例: 本番は cin で読む
  rep(i, N * N) cell[i] = rng.nextInt(1, 99);
  rep(i, N * N) zobPos[i] = rng.next(), zobVis[i] = rng.next();
  rep(i, T + 1) zobTurn[i] = rng.next();
}

// ---- 状態 (問題ごと) ----
struct State {
  int pos = 0, turn = 0;
  ll score = 0;
  ull hash = 0; // 深さをまたいで重複を除くので手数も混ぜる
  bitset<N * N> vis;
};

State initState() {
  State s;
  s.vis[0] = 1;
  s.score = cell[0];
  s.hash = zobPos[0] ^ zobVis[0] ^ zobTurn[0];
  return s;
}

int nextPos(const State &s, int d) {
  int x = s.pos / N + dx[d], y = s.pos % N + dy[d];
  if (x < 0 || x >= N || y < 0 || y >= N)
    return -1;
  return x * N + y;
}

void applyAction(State &s, int d) {
  int np = nextPos(s, d);
  s.hash ^= zobPos[s.pos] ^ zobPos[np] ^ zobTurn[s.turn] ^ zobTurn[s.turn + 1];
  s.pos = np;
  s.turn++;
  if (!s.vis[np]) {
    s.vis[np] = 1;
    s.score += cell[np];
    s.hash ^= zobVis[np];
  }
}

// ---- chokudai サーチ ----
struct Node {
  ll eval; // 大きいほど良い
  int id;  // hist の id
  State s;
  bool operator<(const Node &o) const { return eval < o.eval; }
};

vector<pii> hist; // hist[id] = (親の id, 行動)

vi restore(int id) {
  vi acts;
  for (; hist[id].fi >= 0; id = hist[id].fi)
    acts.pb(hist[id].se);
  reverse(all(acts));
  return acts;
}

// limitMs: 探索を打ち切る時刻。ノードが増えると探索後の解放に時間がかかるので
// TL より余裕を持たせる (例題の 250 万ノードで 100ms 強、メモリ 300MB)
vi chokudaiSearch(int width, double limitMs) {
  hist = {{-1, -1}};
  vector<priority_queue<Node>> que(T + 1); // que[t]: t 手後の状態
  que[0].push({initState().score, 0, initState()});
  unordered_set<ull> done; // 展開済みの hash
  ll rounds = 0;
  while (timer.ms() < limitMs) {
    rounds++;
    rep(t, T) {
      rep(k, width) {
        // 展開済みの局面は捨てて次を見る
        while (!que[t].empty() && done.count(que[t].top().s.hash))
          que[t].pop();
        if (que[t].empty())
          break;
        Node cur = que[t].top(); // 子を作る前に取り出しておく
        que[t].pop();
        done.insert(cur.s.hash);
        // 子を列挙する (問題ごと)
        rep(d, 4) {
          if (nextPos(cur.s, d) < 0)
            continue;
          State s = cur.s;
          applyAction(s, d);
          if (done.count(s.hash))
            continue;
          hist.pb({cur.id, d});
          que[t + 1].push({s.score, (int)hist.size() - 1, s});
        }
      }
    }
  }
  cerr << "rounds=" << rounds << " nodes=" << hist.size() << endl;
  // 一番深いところまで進んだ候補のうち最良のもの
  for (int t = T; t >= 0; t--)
    if (!que[t].empty())
      return restore(que[t].top().id);
  return {};
}

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
  vi acts = chokudaiSearch(1, TL - 300);
  ll score = replay(acts);
  cerr << "chokudai=" << score << " time=" << timer.ms() << "ms" << endl;
  assert((int)acts.size() == T && score > 0);
  for (int d : acts)
    cout << "RDLU"[d];
  cout << '\n';
}
