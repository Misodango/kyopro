/**
 *    author:  Misodango
 *    出展：https://github.com/Misodango/kyopro
 **/
// clang-format off
#include <bits/stdc++.h>
#include <atcoder/all>
#include <set>
using namespace std;
using namespace atcoder;
/* accelration */
// 高速バイナリ生成
#pragma GCC target("avx")
#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")
/* alias */
using ull = unsigned long long;
using ll = long long;
using vi = vector<int>;
using vl = vector<long>;
using vll = vector<long long>;
using vvi = vector<vi>;
using vvl = vector<vl>;
using vvll = vector<vll>;
using vs = vector<string>;
using pii = pair<int, int>;
using pll = pair<ll,ll>;
/* define short */
#define pb push_back
#define fi first
#define se second
#define all(obj) (obj).begin(), (obj).end()
#define rall(obj) (obj).rbegin(), (obj).rend()
#define YESNO(bool) if(bool){cout<<"YES"<<endl;}else{cout<<"NO"<<endl;}
#define yesno(bool) if(bool){cout<<"yes"<<endl;}else{cout<<"no"<<endl;}
#define YesNo(bool) if(bool){cout<<"Yes"<<endl;}else{cout<<"No"<<endl;}
/* REP macro */
#define reps(i, a, n) for (ll i = (a); i < (ll)(n); ++i)
#define rep(i, n) reps(i, 0, n)
class UnionFind{public:UnionFind()=default;explicit UnionFind(size_t n):m_parentsOrSize(n, -1){}int root(int i){if(m_parentsOrSize[i]<0){return i;}return(m_parentsOrSize[i]=root(m_parentsOrSize[i]));}void unite(int a,int b){a=root(a);b=root(b);if(a!=b){if(-m_parentsOrSize[a]<-m_parentsOrSize[b]){std::swap(a,b);}m_parentsOrSize[a]+=m_parentsOrSize[b];m_parentsOrSize[b]=a;}}bool same(int a,int b){return (root(a)==root(b));}int size(int i){return -m_parentsOrSize[root(i)];}private:std::vector<int>m_parentsOrSize;};
struct segment_tree {int n;vector<int> node_min;vector<int> node_max;segment_tree(int n): n(n), node_min(n << 1, INT_MAX), node_max(n << 1, INT_MIN) {}void set(int i, int x) {node_min[i += n] = x;node_max[i] = x;while (i >>= 1) {node_min[i] = min(node_min[i << 1], node_min[i << 1 | 1]);node_max[i] = max(node_max[i << 1], node_max[i << 1 | 1]);}}int min_fold(int l, int r) {int res = INT_MAX;for (l += n, r += n; l < r; l >>= 1, r >>= 1) {if (l & 1)res = min(res, node_min[l++]);if (r & 1)res = min(node_min[--r], res);}return res;}int max_fold(int l, int r) {int res = INT_MIN;for (l += n, r += n; l < r; l >>= 1, r >>= 1) {if (l & 1)res = max(res, node_max[l++]);if (r & 1)res = max(node_max[--r], res);}return res;}};
/* MIN MAX macro */
template<class... T>
constexpr auto min(T ... a){return min(initializer_list<common_type_t<T...>>{a...});}
template<class... T>
constexpr auto max(T ... a){return max(initializer_list<common_type_t<T...>>{a...});}
template<typename T> inline bool chmax(T &a, T b) { return ((a < b) ? (a = b, true) : (false)); }
template<typename T> inline bool chmin(T &a, T b) { return ((a > b) ? (a = b, true) : (false)); }
/* INPUT macro */
template<class... T>
void input(T&... a){(cin >> ... >> a);}
inline void scan(){}
template<class Head,class... Tail>
inline void scan(Head&head,Tail&... tail){std::cin>>head;scan(tail...);}
#define LL(...) ll __VA_ARGS__;scan(__VA_ARGS__)
#define STR(...) string __VA_ARGS__;scan(__VA_ARGS__)
#define cerr cerr << "\033[33m"
#ifdef LOCAL
#include <debug_print.hpp>
#define debug(...) debug_print::multi_print(#__VA_ARGS__, __VA_ARGS__)
#else
#define debug(...) (static_cast<void>(0))
#endif
string OP="LRUD";
// clang-format on

// 出典: https://github.com/Misodango/kyopro/blob/main/ahc/sa.cpp
// 焼きなまし (acc の main.cpp の `// clang-format on` より下に貼る)
// 例題: ランダムな点の TSP を 2-opt で解く。「問題ごと」の所を書き換える。
// 最大化前提。最小化なら score と delta の符号を反転する (例: score =
// -巡回長)。 環境変数 P_<name> があればその値、なければ def を返す
// (ジャッジ上は常に def) 例: P_T0=50 ./a.out。run.py / tune.py
// はこれで値を変えて回す
double param(const char *name, double def) {
  const char *v = getenv(("P_" + string(name)).c_str());
  return v ? atof(v) : def;
}

const double TL =
    param("TL", 1850); // ms。入出力の時間を見込んで TL より少し短く

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
int n, m, k, t;
int sy, sx;
vector<pii> g;
vector<int> gx, gy;
void input() {
  cin >> n >> m >> k >> t >> sy >> sx;
  g.resize(m);
  gx.resize(m);
  gy.resize(m);
  rep(i, m) { cin >> gy[i] >> gx[i]; }
  debug(gy, gx);
}

struct Point {
  int y = 0, x = 0;
};

struct Leg {
  int id = -1;  // -1で歩行のみ，それ以外は使う魔法陣
  Point depart; // ジャンプした時の出発店
};

// ---- 状態とスコア (問題ごと) ----
struct State {
  // vi ord;
  // // circle[from][to];
  // vvi circleRoute;
  // multiset<int> circlePosition;
  // double score; // 大きいほど良い
  vector<Point> initial;
  vector<Leg> legs; // legs[i]: 前の目的地から目的地iまで
  int cost = 0;     // 総操作数。小さいほどよい
};

constexpr int INF = 1000000000;

int evaluate(const State &s) {
  auto circles = s.initial;
  Point pos{sy, sx};
  int cost = 0;

  auto inside = [&](Point p) {
    return 0 <= p.y && p.y < n && 0 <= p.x && p.x < n;
  };

  auto walk = [&](Point q) {
    cost += abs(pos.y - q.y) + abs(pos.x - q.x);
    pos = q;
  };

  for (int i = 0; i < m; ++i) {
    const Leg &leg = s.legs[i];

    if (leg.id >= 0) {
      Point c = circles[leg.id];
      Point p = leg.depart;
      Point q{2 * c.y - p.y, 2 * c.x - p.x};

      if (!inside(p) || !inside(q))
        return INF;

      walk(p);

      ++cost;

      for (Point &v : circles) {
        if (v.y == c.y && v.x == c.x) {
          v = p;
          break;
        }
      }
      pos = q;
    }

    walk(Point{gy[i], gx[i]});
  }
  return cost;
}

void mutate(State &s) {
  auto movePoint = [&](Point &p) {
    if (rng.nextInt(8) == 0) {
      // ときどき大きく動かす
      p = Point{rng.nextInt(n), rng.nextInt(n)};
    } else {
      p.y = std::clamp(p.y + rng.nextInt(-8, 8), 0, n - 1);
      p.x = std::clamp(p.x + rng.nextInt(-8, 8), 0, n - 1);
    }
  };

  int type = rng.nextInt(3);

  if (type == 0) {
    // 初期配置を変更
    movePoint(s.initial[rng.nextInt(k)]);
  } else {
    Leg &leg = s.legs[rng.nextInt(m)];

    // 計算量K倍になるか
    // ↑K=3やった

    if (type == 1) {
      // 使用する陣を変更。-1も含めてジャンプを追加・削除
      leg.id = rng.nextInt(k + 1) - 1;
    } else {
      // ジャンプ直前の位置を変更
      if (leg.id < 0)
        leg.id = rng.nextInt(k);
      movePoint(leg.depart);
    }
  }
}

State solve() {
  State cur;
  cur.initial.resize(k);
  for (Point &c : cur.initial)
    c = Point{rng.nextInt(n), rng.nextInt(n)};

  cur.legs.resize(m);
  for (int i = 0; i < m; ++i) {
    cur.legs[i].depart = i == 0 ? Point{sy, sx} : Point{gy[i - 1], gx[i - 1]};
  }

  // 初期解で最低点をだして，焼いて改善したい
  // 経路入れ替えていいと思っていた😍
  cur.cost = evaluate(cur); // 最初は全区間を歩く
  State best = cur;

  double start = timer.ms();
  double T0 = param("T0", 30);
  double T1 = param("T1", 1);

  for (;;) {
    double now = timer.ms();
    if (now >= TL)
      break;

    double progress = (now - start) / (TL - start);
    double temp = T0 * pow(T1 / T0, progress);

    State cand = cur;
    mutate(cand);
    cand.cost = evaluate(cand);
    if (cand.cost == INF)
      continue;

    int gain = cur.cost - cand.cost;
    if (gain >= 0 || rng.nextDouble() < exp(gain / temp)) {
      cur = std::move(cand);
      if (cur.cost < best.cost)
        best = cur;
    }
  }
  return best;
}

// マンハッタン距離に変更
double dist(int a, int b) { return abs(gx[a] - gx[b]) + abs(gx[a] - gx[b]); }
// double dist(int a, int b) { return hypot(gx[a] - gx[b], gy[a] - gy[b]); }

// 回転中心の座標を返す
pair<int, int> rotation(int a, int b) {
  int ny = abs(-gx[a] + gx[b]) / 2;
  int nx = abs(-gy[a] + gy[b]) / 2;

  return {ny, nx};
}

int diff[] = {-1, 0, 1};
// 魔法陣を使う場合の距離
double distByCircle(int a, int b) {
  auto [ny, nx] = rotation(a, b);

  return abs(gx[a] - nx) + abs(gy[a] - ny);
}

bool isPointSymmetry(int a, int b) {
  if (abs(gx[a] - gx[b]) % 2 || abs(gy[a] - gy[b]) % 2)
    return false;

  // 回転
  auto [ny, nx] = rotation(a, b);

  return (ny == gy[b] && nx == gx[b]);
}

State initState() {
  State s;

  // 魔法陣の初期配置はランダム
  s.initial.resize(k);
  for (Point &c : s.initial)
    c = Point{rng.nextInt(n), rng.nextInt(n)};

  s.legs.resize(m);
  for (int i = 0; i < m; ++i) {
    s.legs[i].id = -1; // 最初は魔法陣を使わない

    // ジャンプを追加するときの出発点候補
    s.legs[i].depart = i == 0 ? Point{sy, sx} : Point{gy[i - 1], gx[i - 1]};
  }

  s.cost = evaluate(s);
  return s;
}

void moveTo(int &y, int &x, int ny, int nx) {
  int dy = ny - y;
  int dx = nx - x;

  rep(i, abs(dy)) cout << (dy < 0 ? "U\n" : "D\n");
  rep(i, abs(dx)) cout << (dx < 0 ? "L\n" : "R\n");

  y = ny;
  x = nx;
}

void output(const State &s) {
  auto circles = s.initial;

  rep(i, k) { cout << circles[i].y << " " << circles[i].x << '\n'; }

  int y = sy, x = sx;

  rep(i, m) {
    int id = s.legs[i].id;

    if (id != -1) {
      int py = s.legs[i].depart.y;
      int px = s.legs[i].depart.x;
      moveTo(y, x, py, px);

      int cy = circles[id].y;
      int cx = circles[id].x;
      cout << "J " << cy << " " << cx << '\n';

      y = 2 * cy - py;
      x = 2 * cx - px;

      // 同じ座標に陣が重なっている場合、採点器は先頭を使う
      rep(j, k) {
        if (circles[j].y == cy && circles[j].x == cx) {
          circles[j] = Point{py, px};
          break;
        }
      }
    }

    moveTo(y, x, gy[i], gx[i]);
  }
}

int main() {
  cin.tie(0)->sync_with_stdio(0);
  input();
  double initScore = initState().cost;
  State best = solve();
  output(best);
}
