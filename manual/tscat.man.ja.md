# TSCAT(1)

## 名前

tscat - タイムスタンプ対応版"cat"コマンド

## 書式

```sh:
tscat [-c|-e|-I|-z] [-Z] [-1kuy] [-p n] [file [...]]
```

## 説明

cat(1)が与えられたファイルをできる限り速く標準出力に書き出すのに対し、本コマンドは与えられたfileの中の各列の行頭（第1列）にある数値をタイムスタンプと見なし、それが示す時刻が到来した瞬間にその行を出力します。ただし、タイムスタンプ文字列の直後には半角空白<0x20>または水平タブ<0x09>が1文字置かれていなければならず、出力時にはその部分が取り除かれます。これはちょうど、[linets(1)](linets.man.ja)コマンドと正反対の動作です。

例えば次のようなデータを本コマンドに読み込ませると、

```text:
20220801000000.000 1st_line
20220801000000.500 2nd_line
20220801000001.000 3rd_line
         :             :
```

次のような内容、かつタイミングで標準出力に出力されます。

```text:
1st_line       ← 2022-08-01 00:00:00.0に出力
2nd_line       ← 2022-08-01 00:00:00.5に出力
3rd_line       ← 2022-08-01 00:00:01.0に出力
   :
```

しかしながら、ほとんどの場合、[linets(1)](linets.man.ja)コマンドで付加したタイムスタンプは既に過去の日時を指しており、これを本コマンドに与えればすべての行が直ちに出力されるため、当時のタイミングは再現されません。そこで、この問題を解決するために-Zオプション（後述）を用意しました。-Zオプションが指定された場合、各行タイムスタンプと1行目のタイムスタンプの時間差を計算し、その時間が経過したタイミングで各行を出力します。これにより、当時のデータの入力タイミングが行単位で再現できるようになります。

各種引数・オプションについては別途説明します。

## 引数の説明

### file

データ源となるファイル。省略したり、`-`を指定した場合には標準出力と見なされます。複数指定することも可能です。

## オプション

### -c, -e, -I, -z

タイムスタンプ文字列（第1列）はどのフォーマットで記録されているとみなすかを指定するためのオプションです。-c、-e、-zはそれぞれ、カレンダー日時（"YYYYMMDDhhmmzz"の14桁整数部を持つ）、UNIX時間（UTCタイムゾーンにおける1970-01-01T00:00:00からの経過秒数）、タイムスタンプ付データ作成開始時刻からの経過秒数を意味し、それぞれのオプションは互いに排他的です。これらがいずれも指定されなかった場合には-cが指定されたものと見なします。以下に、それぞれのオプションにおけるフォーマットの詳細を記します。

* -c: カレンダー日時
  * `YYYYMMDDhhmmss.ddddddddd`
* -e: UNIX時間
  * `[+|-]n.ddddddddd`
* -I: 拡張ISO 8601形式
  * `YYYY-MM-DDThh:mm:ss,ddddddddd{+|-}hh:mm`
  * `YYYY-MM-DDThh:mm:ss,dddddddddZ`
* -z: データ作成開始時刻からの経過秒数
  * `[+|-]N.ddddddddd`

*YYYYMMDDhhmmss*は年月日時分秒を並べた14桁の整数、*n*はUTCタイムゾーンにおける1970-01-01T00:00:00からの経過秒数、*N*はタイムスタンプ付データ作成開始時刻からの経過秒数を意味します。また、いずれの場合も秒未満の時刻を示すために最大9桁の小数部（*ddddddddd*）を付けることもできます。

なお、14桁整数値はOSに設定されているタイムゾーンに依存します。明示的にタイムゾーンを指定したい場合には環境変数TZで設定してください。（「使用例」のセクションを参照）

-zオプションが指定されている場合に負の値を与えると、それ以降に到来する経過秒数にとっての起点時刻が、その負の値を与えられた時刻に変更されます。

### -Z

与えられたタイムスタンプの時刻が訪れた瞬間に各行を出力するのではなく、1行目のタイムスタンプとの時間差を計算し、その時間が経過したタイミングで各行を出力します。このオプションを利用すれば、[linets(1)](linets.man.ja)コマンドで記録したデータの到来タイミングをいつでも再現できます。

### -1

コマンド起動時、標準入力からのデータとは関係無の無い1行（LF）を出力します。このオプションの目的は、AWKやシェルスクリプトからこのコマンドと双方向パイプを構築する際、デッドロックが起こるのを防ぐためです。（[使用例](#使用例)の項にあるAWKスクリプトでの使用例を参照）

### -k

入力テキストデータの先頭に付加されていたタイムスタンプ列を除去せず、そのまま標準出力へ送ります。

### -u

タイムゾーンをUTCに設定します。これは環境変数TZに`UTC+0`を設定する場合と同じです。このオプションは、-cオプション指定時の表示内容に影響します。

### -y

「タイピングモード」にします。通常はタイムスタンプ付きデータの各行を行として（つまり改行コードが付いた状態で）出力します。しかしこのモードでは、空行以外の行については改行コードを出力しません。

例えば次のように、1文字目"H"をタイプした時点からの経過時間をタイムスタンプに持つファイル（mytyping.txt）があるとします。

```text:mytyping.txt
0 H
0.250 e
0.437 l
0.550 l
0.705 o
0.971 
1.855 w
1.969 o
2.104 r
2.198 l
2.302 d
2.835 !
3.113 
```

これに対して、`tscat -zy mytyping.txt`のように、-zと-yオプションを付けた状態で本コマンドを実行すると、次の内容の文字列が当時のタイピングを再現しながら画面に表示されます。

```text:
Hello
world!
```

なお、このmytyping.txtは、[typeliner(1)](typeliner.man.ja.md)コマンドと[linets(1)](linets.man.ja.md)コマンドを組み合わせ、次のようにして作成できます。

```sh:
$ typeliner -e | linets -3Z > mytyping.txt
Hello⏎          ← コマンドを実行したら
world!⏎         ← このようにタイプした後、[Ctrl]+[D]を押す
```

詳細は、[typeliner(1)](typeliner.man.ja.md)コマンドのマニュアルを参照してください。

### -p *n*

（_POSIX_PRIORITY_SCHEDULINGをサポートしているOS限定）プロセス優先度設定。データ転送レート調整に用いるnanosleep()関数の動作精度を上げるため、このオプションの*n*値を2または3にすれば、プロセス優先度が上がります。*n*の範囲は0から3の4段階で、1がデフォルトです。

なお、このオプションを使うには管理者権限が必要な環境があるかもしれません。
## 戻り値

指定されたすべてのファイルを正常に処理できた場合にのみ0を戻し、引数・オプションが不正であった場合や一つでもファイル処理に失敗した場合には0以外を戻します。

## 使用例

[linets(1)](linets.man.ja.md)コマンドを使ってping(8)コマンドの動作内容をミリ秒単位でタイミングごと記録した後、それを再生する。

```sh:
$ ping -c 10 example.cpm | linets -3 > ping_result.log
$ tscat -Z ping_result.log
```

[typeliner(1)](typeliner.man.ja.md)コマンドと[linets(1)](linets.man.ja.md)コマンドを使って自分のタイピングを記録した後、再生する。（なお、タイムスタンプはカレンダー日時形式とし、その際のタイムゾーンは`JST-9`とする）

```sh:
$ export TZ=JST-9
$ typeliner -e | linets -c3 > mytyping.txt
（文字をタイプした後、最後に[Ctrl]+[D]を押す）
$ tscat -cy mytyping.txt
（先程のタイピングの様子が画面に再現される）
```

AWKスクリプトの中で、外部コマンドとしてのsleep(1)を使用するよりもより正確な時間間隔かつ軽量に、スリープを行う。（この使用方法においては、-1オプションと名前付きパイプファイルが一つ必要）

```awk:
BEGIN {
  cmd_wait="tscat -1z named_pipe";

  cmd_wait | getline dummy;  # Set the timer to 0
  print "The time is set 0. Please wait until the time will be 1...";

  print "1 " > "named_pipe"; fflush();
  cmd_wait | gettime dummy;  # Wait for about 1 sec

  print "Now the time is 1. Please wait until the time will be 2...";

  print "2 " > "named_pipe"; fflush();
  cmd_wait | gettime dummy;  # Wait for about 1 sec again

  print "Finish";
  close("named_pipe");
  close(cmd_wait);
}
```

## バグ

コマンド自体はナノ秒単位までのタイムスタンプ指定ができますが、実際にその精度で再現できるとは限りません。どれくらい高精度な再現ができるかはOSの状態やハードウェアの性能に依存します。

-pオプションを使えば精度を上げられるかもしれません。

## 規格への準拠

このコマンドのソースコードはC99、IEEE Std 1003.1-2001（“POSIX.1”）に準拠させてあります。

## 関連項目

[linets(1)](linets.man.ja.md)、[LINETS & TSCATチュートリアル](linets_and_tscat.ja.md)、[typeliner(1)](typeliner.man.ja.md)
