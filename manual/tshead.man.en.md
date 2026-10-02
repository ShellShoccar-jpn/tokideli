# TSHEAD(1)

（日本語版は[こちら](tshead.man.ja.md)）

## Name

tshead - timestamp-aware version of the "head" command

## Synopsis

```sh:
tshead [options] -d    duration      [file ...]   … (a)
tshead [options] -xd   duration      [file ...]   … (b)
tshead [options] -d   -duration      [file ...]   … (c)
tshead [options] -xd  -duration      [file ...]   … (d)
tshead [options] -t    date-and-time [file ...]   … (e)
tshead [options] -xt   date-and-time [file ...]   … (f)
```

## Description

Unlike head(1), which cuts a specified *number of lines* from the start of a given file, this command looks at the timestamp in the first field (the head) of each line of the given file(s) and, from the start, cuts out only the lines at or before some reference *time*. The timestamp string must be followed by a single half-width space <0x20> or horizontal tab <0x09>, which is treated as the delimiter between the timestamp and the rest of the fields. Note that, unlike [tscat(1)](tscat.man.en.md), the timestamp part is never stripped from the output — this command never touches the contents of a line, it only decides whether to let it through. This command has a sister command, [tstail(1)](tstail.man.en.md), which treats the "last line" as its fixed point and cuts out, from the end, the lines at or after some reference time.

For example, suppose there is data like the following (access.log).

```text:access.log
20260821190000 request 1
20260821190001 request 2
20260821190002 request 3
20260821190003 request 4
20260821190004 request 5
```

Running `tshead -t 20260821190002 access.log` against this outputs only the lines from the first line up to the reference time (20260821190002), like so.

```text:
20260821190000 request 1
20260821190001 request 2
20260821190002 request 3
```

The reference time is given via either `-d` (a time relative to the first line's time or to the last line's time) or `-t` (an absolute time). Depending on how it is given, the output range is one of the following six patterns.

* (a) `[ first line, first line's time + duration ]`
* (b) `[ first line, first line's time + duration )`
* (c) `[ first line, last line's time - duration ]`
* (d) `[ first line, last line's time - duration )`
* (e) `[ first line, date-and-time ]`
* (f) `[ first line, date-and-time )`

(`[` and `]` mean the endpoint of the range is included; `(` and `)` mean it is excluded.) See the "[Modes](#modes)" section for exactly what each of these means.

When two or more files are given, they are treated as a single concatenated stream, just as with cat(1). Accordingly, the "last line" in patterns (c) and (d) means the last line of the last file given, and as soon as a line whose time exceeds the reference time turns up partway through any file, the entire process stops — including any files (not yet opened) that would have come after it.

## Arguments

### file

The file to use as the data source. If omitted, or if `-` is given, standard input is assumed. You may specify more than one.

Note that if file is a regular file, it is processed at high speed internally by using a memory map (mmap). For non-seekable special files, such as pipes or terminals, it is instead processed by reading it line by line.

## Options

### -c, -e, -I, -z

These options specify which format the timestamp string (the first field) and the argument of the `-t` option are recorded in. `-c`, `-e`, `-I`, and `-z` respectively mean calendar time, UNIX time, extended ISO 8601 format, and the number of seconds elapsed since the timestamped data started being produced; they are mutually exclusive. If none of them is given, `-c` is assumed. The format details for each option are as follows.

* -c: calendar time
  * `YYYYMMDDhhmmss[.ddddddddd]`
* -e: UNIX time
  * `[+|-]n[.ddddddddd]`
* -I: extended ISO 8601 format
  * `YYYY-MM-DDThh:mm:ss[,ddddddddd]{+|-}hh:mm`
  * `YYYY-MM-DDThh:mm:ss[,ddddddddd]Z`
* -z: the number of seconds elapsed since the timestamped data started being produced
  * `[+|-]n[.ddddddddd]`

*YYYYMMDDhhmmss* is a 14-digit integer made up of the year, month, day, hour, minute, and second; *n* is the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone (for -e), or since the timestamped data started being produced (for -z). In either case, you may append a decimal part of up to 9 digits (*ddddddddd*) to show sub-second precision.

The origin of the `-z` elapsed-seconds count (what "0" means) has nothing to do with when this command itself was started. It is always whatever reference moment the producer of that timestamped data independently chose to call "0."

The interpretation of the 14-digit integer value (with `-c`), and of the extended ISO 8601 format with the timezone omitted (with `-I`), depends on the timezone configured on the OS. If you want to specify the timezone explicitly, set the environment variable TZ, or use the `-u` option.

### -d *duration*

Specifies, as a length of time (a duration), the reference time at which the output range ends. This cannot be combined with `-t`.

*duration* is given in the following format.

* `A[.B][u]`
  * "A" is the integer part.
  * "B" is the optional decimal part.
  * "u" is the unit, and can be "s" (seconds; the default if omitted), "ms" (milliseconds), "us" (microseconds), "ns" (nanoseconds), "m" (minutes), "h" (hours), or "d" (days).

Whether or not you prefix *duration* with a "-" changes how the reference time is computed. See the "[Modes](#modes)" section for details.

### -t *date-and-time*

Specifies, as a time itself, the reference time at which the output range ends. This cannot be combined with `-d`.

The format of *date-and-time* depends on which of `-c`, `-e`, `-I`, or `-z` is given (see "[-c, -e, -I, -z](#-c--e--i--z)").

### -x

An additional option used together with `-d` or `-t`. It excludes the endpoint of the range (the line at exactly the reference time) from the output.

You cannot fuse `-x` right after `-d` or `-t` into a single token (e.g. "-dx" or "-tx"); see the "[Bugs](#bugs)" section for why. Write it as "-xd" / "-xt", with `-x` first, or give the two options separately, as in "-d -x".

### -q

Suppresses the `==> filename <==` header line that is normally printed when two or more files are given.

### -u

Sets the timezone to UTC. This is the same as setting the environment variable TZ to `UTC0`. It affects how the first field is interpreted when `-c` is given, and how the `-t` argument is interpreted.

### -Z

Only meaningful for pattern (a)/(b) (when the argument of `-d` has no leading "-"). The reference time for this pattern is normally "the first line's timestamp" + *duration*, but with `-Z` it becomes "time zero" + *duration* instead.

What "time zero" means depends on which of [`-c`, `-e`, `-I`, or `-z`](#-c--e--i--z) is given.

* With `-c`, `-e`, or `-I`: the UNIX epoch (1970-01-01T00:00:00 UTC)
* With `-z`: whatever moment the `-z`-formatted data itself treats as its own origin (unrelated to the UNIX epoch)

For any other pattern (the last-line-based patterns (c)/(d), or the absolute-time patterns (e)/(f) given via `-t`), `-Z` has no effect at all (it is silently ignored, with no error or warning).

## Modes

This command runs in one of two modes, depending on whether you give it the `-d` option or the `-t` option. You must give exactly one of the two.

### Duration Mode (-d duration)

This is the mode used when the command is started with the `-d` option. The reference time at which the output range ends is computed from the length of time given as *duration*.

Whether or not *duration* is prefixed with a "-" changes how the reference time is computed.

* Without a leading "-" (patterns (a) and (b))
  * The reference time is "the first line's timestamp" + *duration*. (With the [`-Z`](#-z) option, it is "time zero" + *duration* instead.)
  * Without `-x` (pattern (a)): outputs the range `[ first line, reference time ]` (including the line exactly at the reference time).
  * With `-x` (pattern (b)): outputs the range `[ first line, reference time )` (excluding the line exactly at the reference time).
* With a leading "-" (patterns (c) and (d))
  * The reference time is "the last line's timestamp" - *duration*. When two or more files are given, "the last line" means the last line of the last file given.
  * Without `-x` (pattern (c)): outputs the range `[ first line, reference time ]`.
  * With `-x` (pattern (d)): outputs the range `[ first line, reference time )`.

### Absolute-Time Mode (-t date-and-time)

This is the mode used when the command is started with the `-t` option. The time given as *date-and-time* itself becomes the reference time at which the output range ends.

* Without `-x` (pattern (e)): outputs the range `[ first line, reference time ]`.
* With `-x` (pattern (f)): outputs the range `[ first line, reference time )`.

## Return Value

Returns 0 only if all of the given files were processed successfully. Returns a value other than 0 if the arguments or options were invalid, if a file could not be opened, or if there was a line whose timestamp could not be parsed.

## Examples

Extract, from `access.log`, only the access records up to a certain time.

```sh:
$ tshead -t 20260821190002 access.log
20260821190000 request 1
20260821190001 request 2
20260821190002 request 3
```

From an already-accumulated log file, extract only the settled part, excluding the last 2 seconds (which might still be subject to change).

```sh:
$ tshead -d -2s access.log
20260821190000 request 1
20260821190001 request 2
20260821190002 request 3
```

Taking the first line's timestamp as the reference point, extract only the lines within 3 seconds of it.

```sh:
$ tshead -d 3s access.log
20260821190000 request 1
20260821190001 request 2
20260821190002 request 3
20260821190003 request 4
```

Against a `-z`-formatted log (whose first line is not necessarily at elapsed-seconds "0"), take the data's own creation-start moment, rather than whatever happens to be the first line of the stream, as the reference point, and extract only the lines within 12 seconds of it.

```text:uz.log
5 line-at-5
10 line-at-10
15 line-at-15
```

```sh:
$ tshead -z -Z -d 12s uz.log
5 line-at-5
10 line-at-10
```

When two or more files are given, a filename header is added, just as with head(1). Give `-q` to suppress it.

```sh:
$ tshead -t 20260821190004 access1.log access2.log
==> access1.log <==
20260821190000 request 1
20260821190001 request 2

==> access2.log <==
20260821190002 request 3
20260821190003 request 4
20260821190004 request 5
$ tshead -q -t 20260821190004 access1.log access2.log
20260821190000 request 1
20260821190001 request 2
20260821190002 request 3
20260821190003 request 4
20260821190004 request 5
```

## Bugs

You cannot fuse `-x` right after `-d` or `-t` into a single token (e.g. "-dx" or "-tx"). This is because `-d` and `-t` are options that require an argument, so under the OS's getopt() rules, the fused "x" part gets interpreted as part of that argument (for example, "-dx" is interpreted as `-d` with the argument string "x"). Write it as "-xd" / "-xt", with `-x` first, or give the two options separately, as in "-d -x".

When this command encounters a line with a malformed timestamp, it prints a warning and stops processing entirely at that point. Unlike [tscat(1)](tscat.man.en.md), it does not simply skip that line and continue processing the rest.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").

## See Also

[tstail(1)](tstail.man.en.md), [tscat(1)](tscat.man.en.md)
