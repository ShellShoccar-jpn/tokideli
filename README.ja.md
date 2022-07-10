# トキデリ

あなたのUNIXコンピューターに高品質な「時」をデリバリー！

(English version is [here](README.en.md))

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

解決できるうえに、何とシンプルな記述なのでしょう！

これも含めて、いくつかのコマンドを用意しました。

* [`calclock`](bin/calclock) ..... カレンダー時間（年月日時分秒）とUNIX時間を相互変換する
* [`getfilets`](c_src/getfilets.c) ファイルの mtime、ctime、atime を表示する
* [`herewego`](c_src/herewego.c) . キリのいい時刻までsleepし、さらに目覚めた時刻を返す
* [`linets`](c_src/linets.c) ..... 到来したテキストデータの各行の行頭に到来時刻付加する
* [`ptw`](c_src/ptw.c) ........... フルバッファリングを回避するためのコマンド（[stdbuf](https://www.gnu.org/software/coreutils/manual/html_node/stdbuf-invocation.html#stdbuf-invocation)の代替品、詳細は[こちら](https://github.com/ShellShoccar-jpn/tokideli/blob/main/manual/ptw.info.ja.md)）
* [`sleep`](c_src/sleep.c) ....... 秒未満の指定に対応したsleepコマンド（POSIXの範囲での実装）
* [`tscat`](c_src/tscat.c) ....... 各行行頭に記された時刻に従って行毎にデータを出力する
* [`typeliner`](c_src/typeliner.c) ひとまとまりのキータイプ文字列を1行にする
* [`valve`](c_src/valve.c) ....... 1バイトごと、または1行ごとにデータを一定間隔で出力する

各コマンドの使用法を見たい場合は、各コマンドをビルドした上で `--help` オプションを付けて実行してください。

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
