# ahc

AHC 用の貼り付けテンプレート。パラメータ調整の使い方は `usage.html` をブラウザで開く。

acc で作った main.cpp の `// clang-format on` より下を消して、ファイルの中身をそのまま貼る。
1 行目の `// 出典: https://github.com/Misodango/kyopro/...` は消さずに一緒に貼る (コンテスト前に AI で用意したコードは出典を書く必要がある)。
どれも例題つきでそのまま動くので、「問題ごと」と書いた所を書き換えて使う。

| ファイル | 内容 |
|---|---|
| `sa.cpp` | 焼きなまし。例題は TSP (2-opt)。幾何冷却、時計読みは 256 回に 1 回 |
| `beam.cpp` | ビームサーチ。例題はグリッドの得点集め。候補は (評価値, hash, 親, 行動) だけで作り、残ったものだけ状態を作る。hash で重複除去 |
| `chokudai.cpp` | chokudai サーチ。例題は beam.cpp と同じ。深さごとの priority_queue |
| `check.sh` | acc テンプレ + 各ファイルを結合してビルド・実行する (サニタイザ込み) |
| `run.py` | 全 seed を並列に実行して平均・失敗数・最大時間を出す。`--save` / `--base` で seed ごとの勝敗を比べる |
| `tune.py` | `param(...)` の値を自動で探す (ランダム + 最良の近くを探す)。依存なし |

```bash
bash ahc/check.sh
```

どのファイルにも `param(name, 既定値)` が入っていて、環境変数 `P_<name>` があればその値を使う (ジャッジには無いので既定値になる)。
今入っているのは sa.cpp の `TL` `T0` `T1`、beam.cpp の `TL` `W1` `WIDTH`、chokudai.cpp の `TL`。

どのファイルにも `Timer` (`timer.ms()`) と `Rng` (`nextInt(n)`, `nextInt(l, r)`, `nextDouble()`) が入っている。

## パラメータ調整の手順

コンテストのディレクトリ (acc で作った `a/`) に公式ツールを `tools/` として展開してから:

```bash
cd tools && cargo build -r && cd ..          # tester (インタラクティブ) か vis ができる
g++ -std=c++20 -O2 -I ~/kyopro/ac-library main.cpp   # ./a.out
python3 ~/kyopro/ahc/run.py -s 0-49 --save base      # 基準を保存
python3 ~/kyopro/ahc/run.py -s 0-49 --base base T0=30   # 比べる (relative, win/lose)
python3 ~/kyopro/ahc/tune.py T0=1:1000:log T1=0.01:10:log TL=500 -s 0-29 -n 50
```

- `K=lo:hi` は一様、`K=lo:hi:log` は対数 (温度・係数など正の値)、`K=lo:hi:int` は整数、`K=V` は固定。
- tune.py は最初にコードの既定値で回して基準にし、seed ごとの基準比の平均 (relative) が最大のものを探す。Ctrl-C で止めても最良を表示する。結果は `runs/tune.tsv` に追記。
- 最小化の問題は run.py / tune.py に `--min` を付ける。
- 調整中は `TL=500` などで 1 ケースを短くすると試行回数を稼げる。ただし温度の最適値は TL で少し変わるので、最後は元の TL で run.py を回して確かめる。
- `-j` は 6 前後 (既定)。時間で打ち切る解法は並列を増やしすぎると 1 ケースあたりの計算量が減ってぶれる。
- 同じ seed の組で選び続けると、その seed に合わせ込んだ値になりやすい。見つけた値は別の seed (`-s 100-199`) でも勝ち越すか確かめる seed 100 以降は tools で `seq 0 199 > seeds.txt && cargo run -r --bin gen seeds.txt` として作る。gen はファイル名を seed ではなく行番号で付けるので、100〜199 だけを書くと `in/0000` から上書きされる。
- ビームの評価関数で迷ったら、特徴量を `W1 * f1 + W2 * f2 + ...` と足して重みを tune.py に任せる。効かない特徴量は重みが 0 付近に寄る。
- 新しい問題のコードでも、調整したい値を `const double X = param("X", 既定値);` と書けば同じように使える。
- 出力と stderr は `out/XXXX.txt` と `out/XXXX.err` に残る。失敗した seed は FAIL と表示される。

## 注意

- 標準ライブラリの `std::apply` と名前がぶつかるので、状態を進める関数は `applyAction` にしている (`using namespace std` 下で `apply(s, d)` と書くと `d` の型次第で `std::apply` が選ばれてコンパイルエラーになる)。
- chokudai はノードを全部持つのでメモリを食う (例題で 250 万ノード、300MB 弱)。探索後の解放にも 100ms ほどかかるので、打ち切り時刻を TL より 300ms 早めている。

## procon37 で得た教訓

- 冷却は幾何冷却をデフォルトにする。線形より低温の時間が長く、終盤の仕上げに時間を使える。
- t0 / t1 は典型的な delta の大きさから決める。t0 は序盤に受理したい悪化幅、t1 は最小の改善幅くらい。まず delta をログに出して分布を見る。
- 時計読みと温度更新は間引く。毎反復 `pow` や `steady_clock::now` を呼ばない。
- 差分で更新したスコアは、最後に一から計算し直して突き合わせる。累積誤差や差分のバグがここで見つかる。
- ビームは hash の重複除去で多様性を確保する。procon37 では local / potential / diversity の 3 種類の評価で枠を 1/2・1/4・1/4 に分けていた。
- 設定の良し悪しは 1 ケースではなく多数ケースの平均と勝敗数で決める。

## 方針・過去問の参照資料

[AHC 方針の引き出し — 最初に開くページ](docs/README.md)

貪欲の判断基準、ビームの状態・評価、焼きなましの近傍を設計するための資料。短期11回・長期9回の過去AHCを、探索変数と部分問題の構造から引ける形でまとめた。

- [初期解・状態・近傍の設計手順](docs/01-design.md)
- [過去AHCの20事例と出典](docs/02-patterns.md)
- [4時間の進め方・診断表・学習の位置づけ](docs/03-execution.md)
- [このテンプレとの対応・移植時の注意点](docs/04-template-map.md)

