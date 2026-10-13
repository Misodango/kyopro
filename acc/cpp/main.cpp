/**
 *    author:  Misodango
 *    cd `acc config-dir`
 *    この問題を A☆C して †予選突破† させていただきますぞ!wwww
 **/
// clang-format off
#include <bits/stdc++.h>
#include <atcoder/all>
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
// clang-format on
int main() { 
  cin.tie(0)->sync_with_stdio(0); 
}
