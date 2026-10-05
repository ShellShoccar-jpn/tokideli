# CHARTS(1)

（日本語版は[こちら](charts.man.ja.md)）

## Name

charts - prepend a timestamp to each locale-aware character read, then output it

## Synopsis

```sh:
charts [-0|-3|-6|-9] [-c|-e|-I|-z|-Z] [-1bdu] [file [...]]
```

## Description

This command reads each locale-aware character — a single byte in the "C"/"POSIX" locale, or possibly several bytes in a multibyte locale such as UTF-8 — from the text file *file*, prepends a timestamp (the moment that character was read) to the front of it, and writes it as a record of its own. It is the character-grained sibling of [linets(1)](linets.man.en.md), which does the same thing at line granularity instead.

By default, this command decodes the input according to the current locale (via `setlocale(LC_CTYPE, "")` and `mbrtowc()`) to determine how many bytes make up each character. If the `-b` option is given, it instead always treats each single byte as its own character, ignoring the locale entirely (see the [-b](#-b) option below).

Every record this command writes ends with a newline, except when the character itself already is a single `\n` byte — in that case no extra newline is appended, since the character's own byte already terminates the record (see [Record Format and the Trailing Newline Rule](#record-format-and-the-trailing-newline-rule) below). This makes the output losslessly reversible: concatenating every record's character payload, in order, reproduces the original byte stream exactly, including the position of every line break.

The precision of the timestamp (from second down to nanosecond), its format (calendar time, UNIX time, etc.), and whether to also append the difference from the previous character can all be switched via options, exactly as with [linets(1)](linets.man.en.md).

## Arguments

### file

The file used as the data source. If omitted, or if `-` is given, standard input is assumed. Multiple files may be specified.

## Options

### -0, -3, -6, -9

Specifies the precision of the timestamp that is prepended. -0, -3, -6, and -9 mean second, millisecond, microsecond, and nanosecond precision respectively, and these options are mutually exclusive. If none of them is given, -0 is assumed.

If the -d option is also used, the same precision is applied to the elapsed time since the previous character's output that -d appends as well.

### -c, -e, -I, -z, -Z

Specifies the format of the timestamp. -c, -e, -I, -z, and -Z mean, respectively: calendar time, UNIX time, extended ISO 8601 format, the number of seconds elapsed since this command started, and the number of seconds elapsed since the first character of the text data arrived; these options are mutually exclusive. If none of them is given, -c is assumed.

The exact byte-level format of each is identical to [linets(1)](linets.man.en.md)'s corresponding option, with "line" read as "character" throughout (e.g. -Z's elapsed time is measured from the first *character* of the text data, not the first line).

### -1

At startup, outputs one line (LF) that has nothing to do with the data coming from standard input. The purpose of this option is to prevent a deadlock when building a bidirectional pipe between this command and an AWK or shell script — exactly as with [linets(1)](linets.man.en.md)'s own -1 option.

### -b

Forces byte mode: always treats each single byte of the input as its own character, regardless of the current locale. In this mode, the command never calls `setlocale()` or `mbrtowc()` at all. This option exists for two reasons:

* **Clarity** — you don't need to know anything about locales (or set `LC_ALL=C` yourself) to get plain byte-by-byte processing; `-b` says it explicitly.
* **Speed** — skipping the locale/`mbrtowc()` machinery entirely makes this a measurably faster code path, useful when the input is known to be plain ASCII, or when multibyte boundaries simply don't matter for the task at hand.

### -d

Inserts, as a second column right after the column-1 timestamp, the number of seconds elapsed since the previous character arrived. Behaves exactly as [linets(1)](linets.man.en.md)'s -d option, with "line" read as "character" throughout.

### -u

Sets the timezone to UTC. Same as [linets(1)](linets.man.en.md)'s -u option.

## Record Format and the Trailing Newline Rule

Every record this command writes has the form:

```text:
TIMESTAMP <raw bytes of one character>
```

followed by a newline — **except when the character's raw bytes are exactly a single `\n`**, in which case no additional newline is appended, because the character itself already terminates the record.

For example, feeding the two-line UTF-8 text "こんにちは、\n世界!\n" into this command under a UTF-8 locale, with microsecond precision (since all of `printf`'s output arrives through the pipe in one short burst, the gaps between characters only show up at this resolution — at millisecond precision they would all appear identical):

```sh:
$ printf 'こんにちは、\n世界!\n' | LC_ALL=ja_JP.UTF-8 charts -6e
1791201867.431638 こ
1791201867.431651 ん
1791201867.431913 に
1791201867.431919 ち
1791201867.431924 は
1791201867.431929 、
1791201867.431934 
1791201867.431939 世
1791201867.431944 界
1791201867.431949 !
1791201867.431954 
```

yields exactly 11 records — one per character, including the two `\n` characters themselves (each shown as a timestamp followed by nothing, since the character's own byte already ends that line). There is no doubled or blank line anywhere. Concatenating the character payload of all 11 records in order — こ+ん+に+ち+は+、+(the first `\n`)+世+界+!+(the second `\n`) — reproduces the original input byte-for-byte.

## Invalid Byte Sequences

If the input contains a byte sequence that is not valid under the current locale (i.e. `mbrtowc()` reports an encoding error), this command prints a warning to standard error, treats only the first byte of that sequence as a one-byte character (emitting it as-is), and resumes decoding from the very next byte. It never aborts the whole command over a single bad byte, so a long-running pipe keeps working even if it occasionally receives malformed data.

```sh:
$ printf 'AB\xffCD\n' | LC_ALL=ja_JP.UTF-8 charts -e
charts: an invalid byte (0xff) was replaced with a 1-byte character
1700000000 A
1700000000 B
1700000000 (the raw 0xff byte, as-is)
1700000000 C
1700000000 D
1700000000 
```

A particularly common way to end up seeing a long cascade of these warnings — one for every single byte of otherwise perfectly valid multibyte text — is requesting a locale that isn't actually installed on the system (e.g. `LC_ALL=ja_JP.UTF-8` when that locale was never generated). `setlocale()` fails silently in that case, leaving the "C" locale in effect, under which no byte above 0x7F is a valid character at all. When this command detects that `setlocale()` failed, it prints a warning about that specific problem before any of the per-byte ones, so if you see it, check which locales are actually installed (`locale -a`) rather than assuming the input data itself is broken.

```sh:
$ LC_ALL=ja_JP.UTF-8 charts -e < kanji.log
charts: failed to set the locale requested via LC_ALL/LC_CTYPE/LANG; falling back to the "C" locale, so multibyte characters will be reported as invalid bytes. Install/generate the requested locale, or specify one that is actually available (see: locale -a), or use -b if byte-mode processing is what you actually want.
charts: an invalid byte (0xe3) was replaced with a 1-byte character
  :
```

## Truncated Trailing Character

If the input stream ends in the middle of a multibyte character (i.e. there are not enough bytes left to complete it), this command prints a warning to standard error, outputs whatever incomplete bytes remain as the final record, and still exits with status 0 — the same as a clean end of file.

```sh:
$ printf 'AB\xe3\x81' | LC_ALL=ja_JP.UTF-8 charts -e
charts: an incomplete multibyte character (2 byte(s)) was cut off by EOF; flushing it as the last character
1700000000 A
1700000000 B
1700000000 (the 2 leftover bytes, as-is)
```

## Return Value

Returns 0 only when all the specified files were processed successfully. Returns non-zero if an argument or option was invalid, or if processing of one or more files failed.

## Examples

Attach a calendar-time timestamp, with millisecond precision, to a UTF-8 Japanese log file, one character at a time.

```sh:
$ LC_ALL=ja_JP.UTF-8 charts -3 < kanji.log
```

Process the same file byte-by-byte instead, either because the faster path is preferred or because multibyte boundaries don't matter here.

```sh:
$ charts -b -3 < kanji.log
```

Record exactly when each individual keystroke arrives while a user types into a terminal — a natural complement to [typeliner(1)](typeliner.man.en.md), which does the reverse (gathering a bunch of individually-typed characters back into a single line).

```sh:
$ some_keystroke_source | charts -9e
```

Confirm the output is losslessly reversible: stripping the "TIMESTAMP " prefix from each record and concatenating the remainders (adding a newline only where a record's own payload was the `\n` character) reproduces the original file exactly.

```sh:
$ charts -e < orig.txt |
  awk '{ sub(/^[0-9.]+ /, ""); if ($0=="") printf "\n"; else printf "%s", $0 }' |
  diff - orig.txt && echo "lossless"
```

Replay the recorded characters with their original timing reproduced, using [tscat(1)](tscat.man.en.md)'s `-y` ("typing mode") option. `-y` suppresses the newline it would otherwise add after every record, except when a record's own payload is empty — which is exactly what a `charts` record for a literal `\n` character looks like (see [Record Format and the Trailing Newline Rule](#record-format-and-the-trailing-newline-rule) above). So feeding `charts`' output straight into `tscat -y` (together with `-Z`, to replay relative to the first character, rather than waiting for each absolute timestamp to actually arrive) reconstructs both the original text and the original inter-character timing.

```sh:
$ some_keystroke_source | charts -3e > recorded.txt
$ tscat -ey -Z < recorded.txt
```

## Bugs

This command can display timestamps down to nanosecond precision, but that does not mean the time shown is always accurate to that precision. How accurate it actually is depends on the state of the OS and the performance of the hardware.

Decoding multibyte characters (when -b is not given) depends on the platform's `mbrtowc()`/locale support.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").

## See Also

[linets(1)](linets.man.en.md), [typeliner(1)](typeliner.man.en.md), [tscat(1)](tscat.man.en.md)
