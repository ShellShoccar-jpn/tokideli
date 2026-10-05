<p align="center">
  <img src="https://raw.githubusercontent.com/ShellShoccar-jpn/tokideli-assets/main/logo.svg" width="520" alt="tokideli">
</p>

<p align="center">
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/badge/license-public_domain-blue"></a>
  <img alt="Tested on 7 platforms" src="https://img.shields.io/badge/tested_on-Linux_Solaris_FreeBSD_NetBSD_OpenBSD_macOS_Android-informational">
</p>

# トキデリ

たとえシェルスクリプトからでも、完璧なタイミング制御を実現できる軽量POSIX準拠コマンド集
「あなたのシェルスクリプトに、正確な『時』をデリバリー」

(English version is [here](README.en.md))

## 目次

* [これは何？](#これは何)
* [Highlights](#highlights)
* [ビルド・インストール方法](#ビルドインストール方法)
* [著者・ライセンス等](#著者ライセンス等)

## これは何？

これがあればあなたのUNIXライフはより快適になるはずです。

タイミング管理について、あなたは現在POSIXで明記されいるコマンドで満足していますか？私達は満足できていません。なぜなら、それらコマンドでは秒単位よりも正確な時刻、あるいは精密な時刻に基づく動作がさせられないのです。例えば、正確に一秒間隔でデータを出力したいとします。さてあなたは、シェルスクリプトでどう書きますか？おそらく次のように書く以外に無いと思います。

```sh:
cat /PATH/TO/textdata_source |
while IFS= read -r line; do
  printf '%s\n' "$line"
  sleep 1
done
```

しかしこれでは正確な1秒間隔にはなりません。なぜなら、`while` 〜 `done` の構文や `printf` コマンド自身の処理時間が生じるため、1周に要する時間はsleepで生じる1秒を僅かに超えてしまうからです。そこで私達はこのような問題を解決するコマンドを作りました。

`valve` というコマンドを使えば上記の問題が解決できるのです。

```sh:
$ cat /PATH/TO/textdata_source | valve -l 1s
```

解決できるうえに、何とシンプルな記述なのでしょう！　実際に素朴な`while`〜`sleep`ループと`valve -l`を並べて録画したものが以下です。左（素朴なループ）は10周でおよそ0.27秒もずれてしまうのに対し、右（`valve`）のずれはわずか0.02秒程度に収まっています。

<p align="center">
  <video src="https://github.com/user-attachments/assets/3d0735d8-b737-4221-8644-9af8a19b5f25" controls muted playsinline width="520">naive while/sleep loop drifts by +0.27s over 10 iterations, while valve -l stays within +0.02s</video><br>
  <sub>この録画を再現するスクリプト: <a href="https://github.com/ShellShoccar-jpn/tokideli-assets/blob/main/demo.sh">demo.sh</a> / <a href="https://github.com/ShellShoccar-jpn/tokideli-assets/blob/main/demo.tape">demo.tape</a>（<a href="https://github.com/charmbracelet/vhs">vhs</a>の録画レシピ）</sub>
</p>

正確な時間間隔でデータ出力できるのみならず、正確な時間間隔で任意の処理もできるようになります。例えば次のようなシェルスクリプトを書けば、`curl`の処理時間が3秒より十分短い限り、正確な3秒間隔でWebページにアクセスできます。

```sh:
yes | valve -l 3s | while read dummy; do
  curl https://api.example.com/SOME/ENDPOINT
done
```

yesコマンドが、パルス発生源として新たな価値を持つようになるのも興味深くありませんか？

ただし一点注意が必要です。`valve`は自分自身の出力ペースを絶対時刻基準で維持しようとするため、`curl`の処理が一時的に3秒を超過すると、その間に溜まった分がパイプに蓄積し、処理が再開した瞬間にまとめて（ほぼ同時に）流れ出てしまいます。レートリミットを厳密に守りたい、つまりこのようなバーストを避けたい場合は、代わりに[`herewego`](manual/herewego.man.ja.md)コマンドを使うとよいでしょう。

```sh:
while herewego 3s >/dev/null; do
  curl https://api.example.com/SOME/ENDPOINT
done
```

`herewego`は、処理が一時的に長引いてキリのいい時刻を一つ逃しても、その分を後から取り戻そうとはせず、次のキリのいい時刻まで待ってから再開します。そのため、`curl`の処理時間がどれだけ揺らいでも、バーストが起きることはありません。

さて、これらのコマンドも含め、17個のコマンドを用意しましたので一覧にします。

| コマンド | 概要 |
|---|---|
| [`calclock`](manual/calclock.man.ja.md) | カレンダー時間（年月日時分秒）とUNIX時間を相互変換する |
| [`delay`](manual/delay.man.ja.md) | 標準入力から到来した各バイトを一定時間だけ遅延させて標準出力に送る |
| [`getfilets`](manual/getfilets.man.ja.md) | ファイルの mtime、ctime、atime を表示する |
| [`herewego`](manual/herewego.man.ja.md) | キリのいい時刻までsleepし、さらに目覚めた時刻を返す |
| [`linets`](manual/linets.man.ja.md) | 到来したテキストデータの各行の行頭に到来時刻付加する |
| [`oobleck`](manual/oobleck.man.ja.md) | 一定時間内に次行が到来しない場合のみ、現在保持中の行を出力する |
| [`ptw`](manual/ptw.man.ja.md) | フルバッファリングを回避するためのコマンド（[stdbuf](https://www.gnu.org/software/coreutils/manual/html_node/stdbuf-invocation.html#stdbuf-invocation)の代替品、詳細は[こちら](manual/ptw.info.ja.md)） |
| [`qvalve`](manual/qvalve.man.ja.md) | 定量弁：データを指定された時に指定された量だけ出力 |
| [`relval`](manual/relval.man.ja.md) | 逃し弁のようにして、行の転送レートを一定以下に保つ |
| [`sleep`](manual/sleep.man.ja.md) | 秒未満の指定に対応したsleepコマンド（POSIXの範囲での実装） |
| [`surgetk`](manual/surgetk.man.ja.md) | サージタンクのように、一時的なバースト入力をバッファーに吸収して平滑に出力する |
| [`tscat`](manual/tscat.man.ja.md) | 各行行頭に記された時刻に従って行毎にデータを出力する |
| [`tshead`](manual/tshead.man.ja.md) | 先頭行のタイムスタンプを基準に一定期間まで、または、指定時刻までの行を先頭から切り出す |
| [`tstail`](manual/tstail.man.ja.md) | 最終行のタイムスタンプを基準に一定期間前から、または、指定時刻以降の行を末尾まで切り出す |
| [`typeliner`](manual/typeliner.man.ja.md) | ひとまとまりのキータイプ文字列を1行にする |
| [`valve`](manual/valve.man.ja.md) | 1バイトごと、または1行ごとにデータを一定間隔で出力する |
| [`waitill`](manual/waitill.man.ja.md) | 長さではなく期限（時刻）を指定してスリープする |

各コマンドの使用法を見たい場合は、各コマンドをビルドした上で `--help` オプションを付けて実行してください。また、組み合わせ方の具体例や設計の背景について、より詳しく書かれた読み物も[`manual/`](manual/)ディレクトリーに揃えています（日英両言語）。

## Highlights

* **ナノ秒精度** — バイト単位・行単位の到来時刻をナノ秒精度で記録・比較できる。
* **7つのOSで動作確認済み** — Linux、Solaris、FreeBSD、NetBSD、OpenBSD、macOS、Android上で実機ビルド・動作確認を実施。各OS固有の癖を吸収したPOSIX.1-2008準拠のCソースを採用。
* **依存ライブラリ無し** — 1コマンド＝1つのCソースファイルで完結。ビルドに特別な外部ライブラリは不要。
* **パブリックドメイン** — CC0 / Unlicenseのいずれでも。好きなように使ってください。
* **充実した日英バイリンガルドキュメント** — 全17コマンドにマニュアルがあり、組み合わせ方や設計背景を解説する読み物記事も用意。

## ビルド・インストール方法

このリポジトリーを `git clone` してください。そして `INSTALLIN.sh` にインストール先ディレクトリー名を指定して実行してください。ビルドとインストールが対話形式で実行されます。

手短に説明すると、下記のコマンドを実行すればインストールが完了します。（"/usr/local/tokideli"は標準的なインストール先）

```sh:
$ git clone https://github.com/ShellShoccar-jpn/tokideli.git
$ su
# tokideli/INSTALLIN.sh /usr/local/tokideli
```

もしご自身のホームディレクトリー内にインストールするのであれば次のように実行してください。

```sh:
$ git clone https://github.com/ShellShoccar-jpn/tokideli.git
$ tokideli/INSTALLIN.sh $HOME/tokideli
```

`INSTALLIN.sh` を使えば、環境変数"PATH"にインストールディレクトリーを自動的に追加することもできます。（もちろんご自身で手動で追加することもできます）

## 著者・ライセンス等

製作・秘密結社シェルショッカー日本支部

ただし私達は、このリポジトリーで公開しているプログラム・ドキュメント類の一切の権利を放棄します。どうしても、ライセンスを示してくださいということであれば、[CC0](https://creativecommons.org/share-your-work/public-domain/cc0) または [the Unlicense](https://unlicense.org/) をお使いください。

とにかく、ご自由にお使いください。
