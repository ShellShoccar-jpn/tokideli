# LINETS(1)

（日本語版は[こちら](linets.man.ja.md)）

## Name

linets - prepend a timestamp to each line read, then output it

## Synopsis

```sh:
linets [-0|-3|-6|-9] [-c|-e|-I|-z|-Z] [-1du] [file [...]]
```

## Description

This command reads each line from the text file *file*, prepends a timestamp — the moment the first character of that line was read — to the front of the line, and writes the whole line to standard output. The precision of the timestamp (from second down to nanosecond), its format (calendar time, UNIX time, etc.), and whether to also append the difference from the previous line can all be switched via options.

See the description of each option below (-c, -e, -z, -Z) for the exact format of each one.

## Arguments

### file

The file used as the data source. If omitted, or if `-` is given, standard input is assumed. Multiple files may be specified.

## Options

### -0, -3, -6, -9

Specifies the precision of the timestamp that is prepended. -0, -3, -6, and -9 mean second, millisecond, microsecond, and nanosecond precision respectively, and these options are mutually exclusive. If none of them is given, -0 is assumed.

If the -d option is also used, the same precision is applied to the elapsed time since the previous line's output that -d appends as well.

### -c, -e, -I, -z, -Z

Specifies the format of the timestamp. -c, -e, -I, -z, and -Z mean, respectively: calendar time (a 14-digit integer part of the form "YYYYMMDDhhmmss"), UNIX time (the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone), extended ISO 8601 format, the number of seconds elapsed since this command started, and the number of seconds elapsed since the first character of the first line of the text data arrived; these options are mutually exclusive. If none of them is given, -c is assumed. The format details for each option are as follows.

#### -c: calendar time

If the line data read is "foo bar 123", it is output in the following format.

```text:
YYYYMMDDhhmmss.ddddddddd foo bar 123
```

That is: the 14-digit calendar-time integer YYYYMMDDhhmmss; then, if the -3, -6, or -9 option is used, a decimal point "." followed by the fractional part *d* at that precision (with -0, no decimal point is shown either); then a single space " "; followed by the original one line of text data.

Note that the 14-digit integer value depends on the timezone set for the OS. If you want to specify the timezone explicitly, set the environment variable TZ. (See the "Examples" section.)

#### -e: UNIX time

If the line data read is "foo bar 123", it is output in the following format.

```text:
n.ddddddddd foo bar 123
```

That is: the UNIX-time integer *n*; then, if the -3, -6, or -9 option is used, a decimal point "." followed by the fractional part *d* at that precision (with -0, no decimal point is shown either); then a single space " "; followed by the original one line of text data.

#### -I: extended ISO 8601 format

If the line data read is "foo bar 123", it is output in the following format.

```text:
YYYY-MM-DDThh:mm:ss,ddddddddd{+|-}hh:mm foo bar 123
```

That is: the year, month, day, hour, minute, and second according to ISO 8601; then, if the -3, -6, or -9 option is used, a decimal comma "," (a comma, not a period) followed by the fractional part *d* at that precision (with -0, no decimal part is shown either); then a signed hour-and-minute part indicating the timezone; then a single space " "; followed by the original one line of text data.

Note that the timezone can be changed by setting the environment variable TZ. (See the "Examples" section.)

#### -z: elapsed seconds since this command started

If the line data read is "foo bar 123", it is output in the following format.

```text:
n.ddddddddd foo bar 123
```

That is: the integer *n* representing the number of seconds elapsed since this command started; then, if the -3, -6, or -9 option is used, a decimal point "." followed by the fractional part *d* at that precision (with -0, no decimal point is shown either); then a single space " "; followed by the original one line of text data.

#### -Z: elapsed seconds since the first character of the first line of the text data arrived

If the line data read is "foo bar 123", it is output in the following format.

```text:
n.ddddddddd foo bar 123
```

That is: the integer *n* representing the number of seconds elapsed since the first line (strictly speaking, its first character) arrived; then, if the -3, -6, or -9 option is used, a decimal point "." followed by the fractional part *d* at that precision (with -0, no decimal point is shown either); then a single space " "; followed by the original one line of text data.

Consequently, the timestamp of the first line is always 0. This is not merely 0 within the significant digits shown — it is exactly 0 — so no fractional part is shown for it.

### -1

At startup, outputs one line (LF) that has nothing to do with the data coming from standard input. The purpose of this option is to prevent a deadlock when building a bidirectional pipe between this command and an AWK or shell script. (See the AWK script example in the [Examples](#examples) section.)

### -d

Inserts, as a second column right after the column-1 timestamp, the number of seconds elapsed since the previous line arrived. When this option is used, the original text therefore appears as the third column.

If the line data read is "foo bar 123", it is output in the following format.

```text:
TIMESTAMP n.ddddddddd foo bar 123
```

The column shown as "TIMESTAMP" is the same as column 1 when one of the timestamp-format options (-c, -e, -z, -Z) is specified. The second column is the difference between the time the previous line (strictly speaking, its first character) arrived and the time the first character "f" of this line arrived: *n* is its integer part, then, if the -3, -6, or -9 option is used, a decimal point "." followed by the fractional part *d* at that precision (with -0, no decimal point is shown either); then a single space " "; followed by the original one line of text data.

Note that, since there is no preceding line for the first line, the second column is basically "0". However, only when the -z option is given, it instead shows the elapsed time since the command started (which, as a result, is the same value as column 1).

### -u

Sets the timezone to UTC. This is the same as setting the environment variable TZ to `UTC0`. This option affects the displayed values when the -c or -I option is given.

## Return Value

Returns 0 only when all the specified files were processed successfully. Returns non-zero if an argument or option was invalid, or if processing of one or more files failed.

## Examples

Attach a calendar-time timestamp, with millisecond precision, in the JST-9 timezone, to the output of ping(8).

```sh:
$ ping example.com | TZ=JST-9 linets -3
```

Repeatedly run `sleep 1` and display, down to microsecond precision, the timestamp and the difference from the previous one, in order to see how long each round actually takes.

```sh:
$ while sleep 1; do echo; done | linets -6d
```

Display the above timestamps using the Los Angeles, USA, timezone (using the environment variable TZ).

```sh:
$ while sleep 1; do echo; done | TZ='America/Los_Angeles' linets -6d
```

Inside an AWK script, obtain a lighter-weight and more precise time than by invoking the external command date(1). (This usage requires the -1 option and one named-pipe file.)

```awk:
BEGIN {
  cmd_gettime="linets -1c3 named_pipe"; # format is "YYYYMMDDhhmmss.nnn"
  cmd_gettime | getline dummy;          # preparation to get the time

  system("sleep 1");

  print "" > "named_pipe"; fflush();
  cmd_gettime | getline t; t=substr(t,1,length(t)-1);
  print "Current time: " t;

  system("sleep 1");

  print "" > "named_pipe"; fflush();
  cmd_gettime | getline t; t=substr(t,1,length(t)-1);
  print "Current time: " t;

  close("named_pipe");
  close(cmd_gettime);
}
```

When recording the arrival time of text data, if the downstream command ("COMMAND1") is too slow to keep up, that slowness will eventually propagate back to linets and make it block on its own input, so the arrival time can no longer be recorded accurately. To prevent this, put [surgetk(1)](surgetk.man.en.md) right after linets to allocate a sizable buffer (100MiB in this example), keeping linets from ever blocking on its input.

```sh:
$ cat FAST_DATA_SOURCE_VIA_NAMED_PIPE | linets -3 | surgetk -d 100MiB | COMMAND1
```

## Bugs

This command can display timestamps down to nanosecond precision, but that does not mean the time shown is always accurate to that precision. How accurate it actually is depends on the state of the OS and the performance of the hardware.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").

## See Also

[tscat(1)](tscat.man.en.md), [LINETS & TSCAT Command Tutorial](linets_and_tscat.en.md)
