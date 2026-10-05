# c_src コーディングスタイルガイド

このディレクトリのCソースは「Shell-Shoccar Japan」による統一テンプレートに極めて忠実に書かれている。
新しいコマンドのソースコードをここに追加するときは、以下の特徴を必ず模倣すること。
最小サンプルとして `sleep.c`（getopt無し）、getopt・構造体・複数エラー関数を使う中規模サンプルとして `getfilets.c` を参照するとよい。

## 1. 全体方針

- 1コマンド = 1ソースファイル。共通処理（`print_usage_and_exit`, `error_exit`, `warning`, `tmsp`型のtypedef等）を共有する `.h` は作らず、各ファイルに複製して実装する。
- `.h` ファイルは存在しない。ビルドは `MAKE.sh` が `c_src/` 内の `.c` を1つずつ個別コンパイルする方式（依存関係やリンクを考えない1ファイル1バイナリ）。

## 2. ファイル冒頭バナー

`#` で始まる行を `/* ... */` で囲む、シェルスクリプト風のブロックコメント。フィールド順は固定。

```c
/*####################################################################
#
# SLEEP - Sleep Command Which Supported Non-Integer Numbers
#
# USAGE   : sleep seconds
# Args    : seconds ... The number of second to sleep for. You can
#                       give not only an integer number but also a
#                       non-integer number here.
# Retuen  : Return 0 only when succeeded to sleep
#
# How to compile : cc -O3 -o __CMDNAME__ __SRCNAME__
#
# Written by Shell-Shoccar Japan (@shellshoccarjpn) on 2024-06-23
#
# This is a public-domain software (CC0). It means that all of the
# people can use this for any purposes with no restrictions at all.
# By the way, We are fed up with the side effects which are brought
# about by the major licenses.
#
# The latest version is distributed at the following page.
# https://github.com/ShellShoccar-jpn/tokideli
#
####################################################################*/
```
（`sleep.c:1-23`）

- フィールド順: コマンド名+一行概要 → `USAGE` → `Args`/`Options`/`Output` → `Retuen`（既存ファイル群で定着している表記。新規ファイルでもこの表記に合わせる） → `How to compile`（`__CMDNAME__`/`__SRCNAME__` プレースホルダを `MAKE.sh` が実際のコマンド名・ソース名に置換する。`-pthread`や`-lrt`が必要な場合は複数行に分けてフォールバック用の代替コンパイルコマンドも書く） → `Written by Shell-Shoccar Japan (@shellshoccarjpn) on YYYY-MM-DD` → CC0宣言の定型文 → GitHubリポジトリURL（`https://github.com/ShellShoccar-jpn/tokideli`）。

## 3. セクションコメントの3階層バナー

```c
/*####################################################################
# Main
####################################################################*/

/*=== Initial Setting ==============================================*/

/*--- Variables ----------------------------------------------------*/
```

- 大見出し: `/*####...####*/` + `# 見出し` （ファイル全体の大区分。`Initial Configuration`, `Main` など）
- 中見出し: `/*=== 見出し =...=*/`（`=`で右端まで埋める。`Initial Setting`, `Parse options`, `Try to make me a realtime process`, `Finish` などが頻出）
- 小見出し: `/*--- 見出し -...-*/`（`-`で右端まで埋める。`macro constants`, `headers`, `global variables`, `Variables`, `prototype functions` などが頻出）

## 4. 命名規則（ハンガリアン記法）

- 関数名: `snake_case`（例: `print_usage_and_exit`, `error_exit`, `parse_abstime`）
- マクロ/定数: `UPPER_SNAKE_CASE`（例: `LINE_BUF`, `BUFSIZE`, `RINGBUF_NUM_MAX`）
- 変数名は「型プレフィックス + PascalCase」:

| プレフィックス | 意味 | 例 |
|---|---|---|
| `i` | int | `iRet`, `iStatus`, `iNerror` |
| `i8` | int64_t | `gi8Peritime` |
| `d` | double | `dNum` |
| `sz` / `psz` | 文字列 (char配列/ポインタ) | `szBuf`, `pszCmdname` |
| `p` / `pst` | ポインタ / 構造体ポインタ | `pstTm`, `pvArgs` |
| `st` | struct（値） | `gstCtrlfile` |
| `ts` / `tsp` | `struct timespec`（`typedef struct timespec tmsp;` をファイルごとに定義） | `tspcSleeping_time` |
| `g` | グローバル変数（他プレフィックスと併用: `gpsz`, `gi`, `gst` 等） | `gpszCmdname`, `giVerbose` |
| `sa` | `struct sigaction` | `gsaExit` |

- グローバル変数はファイル冒頭の `/*--- global variables ---*/` にまとめて宣言し、各行末に用途コメントを桁揃えで記述する（例: `getfilets.c:54-56`）。
- 構造体は `typedef struct _xxx_t {...} xxx_t;` のように、内部タグ名に `_` プレフィックス、typedef名に `_t` サフィックスを付ける。

## 5. インデント・波括弧

- インデントは半角スペース2つ。タブは使わない。
- 波括弧はK&R方式（制御構文・関数定義と同じ行に開き波括弧）。
- 短い `if`/`case` 文は同じ行に収め、`break` やコメントの位置を縦に桁揃えする:

```c
case '9': iNanosec = 1;                 break;
case 'c': iFmttype = 0;                 break;
case 'u': (void)setenv("TZ", "UTC", 1); break;
```
（`getfilets.c:142-147`）

- 関数末尾で `return 0;}` のように return文と閉じ波括弧を同一行にまとめる慣習がある（`getfilets.c:236`）。
- **`case`ブロック中の1行を編集して文字数が変わった場合、その行だけでなく同じブロック内の`break;`が揃っている他の全行のパディングも桁が合うように調整すること。** 1行だけ直して他行を放置すると、その行だけ`break`の位置がずれて縦の桁揃えが崩れる。空きスペースに余裕があればそのペア行のスペースを増減するだけで直るが、余裕が無い場合はブロック全体の目標列を1つ右にずらし、他の行にも同じ分だけスペースを足すこと（詰めすぎて`);break;`のようにスペース0にはしない）。

## 6. Usageバナーの `Version` / `Last Updated` 行

`print_usage_and_exit()` は `fprintf(stderr, ...)` でUsageを出力し、末尾近くに以下の形式で2行を含む（`charts.c`参照。この規約は`cmd_scripts/*.sh`の`print_usage_and_exit()`ヘッドドックにも同様に適用される）:

```c
"Version      : 1.0.0\n"
"Last Updated : 2026-10-06 00:55:00 JST\n"
"               (POSIX C language)\n"
"\n"
"Shell-Shoccar Japan (@shellshoccarjpn), No rights reserved.\n"
"This is public domain software. (CC0)\n"
"\n"
"The latest version is distributed at the following page.\n"
"https://github.com/ShellShoccar-jpn/tokideli\n"
```

この2行は意味が全く異なるので混同しないこと。

- **`Version`**: プロジェクト全体のセマンティックバージョン（例: `1.0.0`）。全`c_src/*.c`・`cmd_scripts/*.sh`で共通の値であり、個々のファイルを編集しても変えてはならない。更新するのはリリース時のみで、`release/bump_version.sh`（保守者専用スクリプト。ルートの`VERSION`ファイルも同時に更新する）が一括で書き換える。
- **`Last Updated`**: そのファイル個別の最終編集日時（`YYYY-MM-DD HH:MM:SS JST`、秒まで含む）。**こちらが旧`Version`行に相当するもの**で、ユーザーのグローバル指示により、このソースファイルを編集するたびに、この行を編集時点の現在日時(JST)へ必ず更新すること（頼まれなくても毎回行う）。`release/bump_version.sh`はこの行には一切触れない（バージョン番号を上げる操作自体は、そのファイルのロジックが変わったことを意味しないため）。

2行の値を縦に揃えるため、`Version`側のラベルは`"Version      : "`（`Version`の後に半角スペース6個＋`: `）のようにパディングし、`"Last Updated : "`と同じ15文字幅に揃える。直後の注釈行（`(POSIX C language)`等）も、値の開始位置に揃えて15個の半角スペースでインデントする。

**さらに重要**: ファイル冒頭バナーの `# Written by Shell-Shoccar Japan (@shellshoccarjpn) on YYYY-MM-DD`（`calclock.c`のようにシェルスクリプトのC移植版では `# Ported to C by ... on YYYY-MM-DD` という表記になる）の日付部分も、上記`Last Updated`行を更新するたびに**同じ日付**（時刻部分を除いた`YYYY-MM-DD`のみ）へ必ず一致させること。`Last Updated`だけ更新して`Written by`/`Ported to C by`を放置すると、2つの「最終更新日」表示が食い違ってしまうため、この2箇所は常にセットで更新する。

なお、全コマンドは`--version`オプションにも対応している。`-v`が全ファイルで既に「verboseモード」に使われているため（`getopt_long`等のGNU拡張は使わない方針のため長短オプションの共存もできない）、`getopt()`呼び出しより前に`argv[1]`が文字列`"--version"`そのものかを素朴にチェックする前処理で対応する（`argc>=2 && strcmp(argv[1],"--version")==0`）。一致した場合は`"%s (tokideli) 1.0.0\n"`を出力して`return 0;`する。

## 7. include文

```c
/*--- macro constants ----------------------------------------------*/
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
...
```

- `/*--- macro constants ---*/` の下に `#define` 類、続けて `/*--- headers ---*/`（省略される場合もある）の下に `#include` を概ねアルファベット順で並べる。
- 独自の `.h` は作らない。標準ヘッダのみ使用する。
- OS依存の分岐は `#ifdef`/`#ifndef` で行う（例: `getfilets.c:159,202,209,216`）。

## 8. エラーハンドリング

`perror()` は使わない。各ファイルで以下の2〜3関数を複製実装する（`getfilets.c:88-103`）:

```c
void error_exit(int iErrno, const char* szFormat, ...) {
  va_list va;
  va_start(va, szFormat);
  fprintf(stderr,"%s: ",gpszCmdname);
  vfprintf(stderr,szFormat,va);
  va_end(va);
  exit(iErrno);
}
void warning(const char* szFormat, ...) {
  va_list va;
  va_start(va, szFormat);
  fprintf(stderr,"%s: ",gpszCmdname);
  vfprintf(stderr,szFormat,va);
  va_end(va);
  return;
}
```

- 致命的エラーは `error_exit(errno, "...: %s\n", strerror(errno))` のように `errno` をそのまま終了コードに渡す。
- 続行可能な問題は `warning()` を使い、処理を継続する。

## 9. main関数・引数パース

```c
int main(int argc, char *argv[]) {
  ...
  gpszCmdname = argv[0];
  for (i=0; *(gpszCmdname+i)!='\0'; i++) {
    if (*(gpszCmdname+i)=='/') {gpszCmdname=gpszCmdname+i+1;}
  }
  ...
  while ((i=getopt(argc, argv, "9cehIuv")) != -1) {
    switch (i) {
      ...
      case 'h': print_usage_and_exit();
      default : print_usage_and_exit();
    }
  }
  argc -= optind;
  argv += optind;
  ...
  return 0;
}
```
（`sleep.c:72-99`, `getfilets.c:111-153`）

- `argv[0]` から `basename()` を使わず自前ループでコマンド名を抽出し `gpszCmdname` に格納する。
- オプション解析には `getopt()` を使う。解析後は `argc -= optind; argv += optind;` で残り引数を得る。
- `main` の戻り値は `int`。正常終了は `return 0;`。

## 10. 関数プロトタイプ・static

- `static` 修飾子は使わない。全関数を外部リンケージのまま定義する。
- 関数が複数あるやや大きめのファイルは、先頭付近に `/*--- prototype functions ---*/` セクションでプロトタイプを列挙する。単純なファイルではプロトタイプ無しで直接定義してよい。

## 11. ライセンス表記

- 著作権表記は「Copyright」ではなくCC0（パブリックドメイン）宣言で統一する。ファイル冒頭コメントとUsage出力の両方に重複して記載する。
- 著者は個人名ではなく `Shell-Shoccar Japan (@shellshoccarjpn)` と表記する。
- リポジトリURLは `https://github.com/ShellShoccar-jpn/tokideli`。

## 12. 新規ファイル作成時のチェックリスト

- [ ] ファイル冒頭にCC0バナー（USAGE/Args/Retuen/How to compile/Written by/ライセンス文/URL）を書いたか
- [ ] `print_usage_and_exit()` / `error_exit()` /（必要なら）`warning()` を複製実装したか
- [ ] Usage出力内に `Version      : X.Y.Z`（プロジェクト全体のバージョン。個別ファイル編集時には変更しない）と `Last Updated : YYYY-MM-DD HH:MM:SS JST`（そのファイルの最終編集日時。編集の都度更新）の2行を、桁揃えして含めたか
- [ ] 冒頭バナーの `Written by`（または `Ported to C by`）の日付を、上記`Last Updated`の日付と一致させたか
- [ ] `--version`オプション（`argv[1]`が`"--version"`かを`getopt()`より前に素朴にチェックする前処理）に対応したか
- [ ] 大/中/小の3階層セクションコメントで構成したか
- [ ] 変数名がハンガリアン記法（型プレフィックス+PascalCase）に従っているか
- [ ] グローバル変数に `g` プレフィックスを付け、冒頭にまとめたか
- [ ] インデント2スペース・K&R波括弧になっているか
- [ ] `getopt()` でオプション解析し、`perror()` ではなく自作 `error_exit`/`warning` を使っているか
- [ ] `static` を使わず、独自 `.h` を作らずファイル単体で完結しているか
