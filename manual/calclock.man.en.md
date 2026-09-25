# CALCLOCK(1)

（日本語版は[こちら](calclock.man.ja.md)）

## Name

calclock - convert between calendar time and UNIX time

## Synopsis

```sh:
calclock [+[n]h] [-r] f1 [f2 [...]] file
calclock -d[r] string
```

## Description

Given a *file* in space-separated text format such as crontab(5) or fstab(5), this command treats the specified column(s) (given by f1, f2, ...) as containing calendar time ("YYYYMMDDhhmmss[.d]", where "d" is the sub-second part) or UNIX time ("n[.d]", where "d" is the sub-second part), and inserts a new column immediately to the right of each specified column, containing the value converted to UNIX time or calendar time, respectively.

Note that converting between calendar time and UNIX time requires timezone information. For calendar time, the timezone currently set for the environment is assumed. If you want to specify the timezone explicitly, set the environment variable TZ. (See the "Examples" section.)

Each argument and option is explained in detail below.

## Arguments

### f1, f2, ......

Column number(s) that hold the date/time data to be converted. At least one must be given, and multiple columns may be specified. Each one can be written in any of the following forms.

* `n` ... an absolute column number (a natural number, 1 or greater)
* `NF` ... the last column
* `NF-n` ... n columns before the last one (n is a natural number, 1 or greater)
* `a/b` ... a range. Both `a` and `b` can each be any of the three forms above, freely combined (e.g. `2/5`, `3/NF`, `NF-3/NF`, `NF-5/NF-2`). Their order doesn't matter (they are automatically sorted from the smaller to the larger).
* `0` ... targets every column. This overrides everything else: wherever a literal `0` appears on the command line, even alongside other column specifications, "every column" wins.

`NF` and `NF-n` are resolved separately for each line being processed, based on that line's own actual number of columns (NF). Consequently, the same column specification can end up pointing at a different actual column from one line to the next.

* For a range (`a/b`) where at least one side is `NF-n` (n is 1 or greater), if the resolved lower bound ends up below column 1, this command **reports an error to the standard error output and terminates entirely** (this does not apply when one side is bare `NF`, nor when both sides are absolute column numbers).
* In contrast, when a *single* `NF-n` column (not part of a range) resolves below column 1, that is not an error: the conversion for that column is simply skipped on that particular line, and processing continues.
* If a specified column number is greater than a line's actual number of columns (NF), that is not an error either: the line is automatically extended (any columns in between are filled with an empty string) so that the converted value can be appended at the specified column.

### file

The file used as the data source. If omitted, or if `-` is given, standard input is assumed. Only one file may be specified.

### string

An argument for directly specifying, as a string, the calendar time or UNIX time to be converted. It can be set when the -d option is given.

## Options

### +[n]h

Specifies the number of header lines. If there are *n* lines at the top of the input that do not contain date/time data to be converted (such as a line of column names), those lines are output unchanged. *n* may be omitted, in which case it is treated as 1. If this option itself is omitted, it is treated as 0 (no header lines).

### -r

Reverse-conversion mode. Without this option, the data in the specified column(s) is treated as calendar time (YYYYMMDDhhmmss[.d], where d is the sub-second part), and a column converted to UNIX time (n[.d], where d is the sub-second part) is inserted immediately to its right — this is the "forward conversion." With this option, the data in the specified column(s) is instead treated as UNIX time, and a column converted to calendar time is inserted immediately to its right.

### -d

Direct mode. A mode for specifying the time to be converted directly as an argument. The result is output as two columns: `original-time converted-time`.

## Environment Variables

### LINE_BUFFERED

Specifies the buffering mode of the standard output.

* `yes` ... Switches to line-buffered mode.
* `forcible` ... Switches to line-buffered mode (behaves exactly the same as `yes`. The difference in meaning is "exit with an error instead, if switching to line-buffered mode turns out to be impossible" — but in practice, there is no environment this command runs on where that switch could ever be impossible, so there is no practical difference from `yes`).

If neither is set, the normal buffering mode applies (line-buffered if connected to a terminal, fully buffered otherwise).

## Notes

### Converting a malformed date/time value

Even if the column to be converted contains a string that cannot be interpreted as a calendar time or UNIX time (an invalid date, a seconds value of 63 or more, an out-of-range month or day, and so on), this is not treated as an error. Instead, the 10-character placeholder string `xxxxxxxxxx` is inserted as the converted value on the spot, and processing continues (since this command itself does not abort, this has no effect on the return value).

Likewise, when converting from UNIX time to calendar time (with `-r`), if the UNIX time's absolute value exceeds one trillion (10^12), the 14-character placeholder string `xxxxxxxxxxxxxx` is inserted instead.

Note also that the sub-second decimal part (`.d`) is never validated as a number during conversion; it is simply appended, as-is, to the end of the converted value as a literal string. This means that even if the decimal part contains non-numeric characters, they are passed through to the output unchanged.

## Return Value

Returns 0 only when the specified file was processed successfully; returns non-zero if it could not be processed due to an invalid argument, option, file, or content.

## Examples

Suppose the source file access.log has the following content.

```text:access.log
20220718000000 user1: log in
20220718000010 user1: access "members.cgi"
20220718000030 user1: log out
```

Forward-convert the data in column 1 to UNIX time (using the Japan Standard Time timezone) and store the result in access2.log.

```sh:
$ export TZ=JST-9
$ calclock 1 access.log > access2.log
```

The resulting file access2.log has the following content.

```text:access2.log
20220718000000 1658070000 user1: log in
20220718000010 1658070010 user1: access "members.cgi"
20220718000030 1658070030 user1: log out
```

## History

The original of this command is Open usp Tukubai by USP Laboratory Co., Ltd.; this is a port of it to a POSIX-conformant shell script. It should be functionally a superset of the original. Alongside the shell-script version (`cmd_scripts/calclock.sh`), a C-language version with equivalent behavior (`c_src/calclock.c`) is also provided.

## Compliance with Standards

The shell-script version is written as a shell script for sh that conforms to IEEE Std 1003.1-2001 ("POSIX.1"), and every command it invokes stays within the scope of that same standard. The C-language version conforms to C99, and to IEEE Std 1003.1-2008 ("POSIX.1").
