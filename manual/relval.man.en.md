# RELVAL(1)

（日本語版は[こちら](relval.man.ja.md)）

## Name

relval - keeps the line transfer rate at or below a limit, like a relief valve

## Synopsis

```sh:
relval [-c|-e|-I|-z] [-ku] [-d fd|file] ratelimit   [file [...]]
relval [-c|-e|-I|-z] [-ku] [-d fd|file] controlfile [file [...]]
```

## Description

This command reads *file* (standard input if omitted or if `-` is given) as a text data source and streams it to standard output, but if the incoming line-by-line transfer rate is too high, it thins out some of the lines so that the rate stays at or below the limit given by *ratelimit*.

However, every line of the incoming text data must carry a timestamp in its first field. Rather than measuring the transfer rate against the wall clock of the machine it runs on, this command determines the transfer rate by reading the timestamp on each line. The timestamp format it expects is the same as [tscat(1)](tscat.man.en.md)'s (see [Requirements for the Text Data](#requirements-for-the-text-data)). If you have ordinary text data with no timestamps, we recommend attaching timestamps beforehand with the [linets(1)](linets.man.en.md) command and feeding the result into this command's standard input. (See the "Examples" section.)

### An Example

Suppose you have a data source that generates one line of text data every 0.1 seconds. If you attach a timestamp to the head of each line with, say, [linets(1)](linets.man.en.md), you get data like the following.

```text:
20240621000000.000 1st_line (output at 2024-06-21 00:00:00.0)
20240621000000.100 2nd_line (output at 2024-06-21 00:00:00.1)
20240621000000.200 3rd_line (output at 2024-06-21 00:00:00.2)
20240621000000.300 4th_line (output at 2024-06-21 00:00:00.3)
20240621000000.400 5th_line (output at 2024-06-21 00:00:00.4)
20240621000000.500 6th_line (output at 2024-06-21 00:00:00.5)
20240621000000.600 7th_line (output at 2024-06-21 00:00:00.6)
20240621000000.700 8th_line (output at 2024-06-21 00:00:00.7)
20240621000000.800 9th_line (output at 2024-06-21 00:00:00.8)
20240621000000.900 10th_line (output at 2024-06-21 00:00:00.9)
         :             :
```

Let's think about how this gets thinned out once it's fed into this command.

Suppose the *ratelimit* argument is set to `2/500ms`. Then, at most 2 lines every 500 milliseconds are allowed through to standard output. Since the text data above arrives more frequently than that limit allows, some lines get thinned out. Finally, the timestamp field is stripped. As a result, standard output produces the following data.

```text:
1st_line (output at 2024-06-21 00:00:00.0)
2nd_line (output at 2024-06-21 00:00:00.1)
6th_line (output at 2024-06-21 00:00:00.5)
7th_line (output at 2024-06-21 00:00:00.6)
         :             :
```

In this example, the original text data arrives every 0.1 seconds, but because a limit of at most 2 lines per 500 milliseconds is in effect, the limit is already reached after the first 2 lines are output. This limit is not lifted until 500 milliseconds after the first line arrived, so "3rd_line" through "5th_line," which arrive before those 500 milliseconds have passed, are discarded without being output. Then, starting from the line that arrives 500 milliseconds after the limit was lifted ("6th_line"), the same behavior repeats.

Normally, thinned-out lines that get discarded are simply erased from memory, but by using the `-d` option described below, you can also send them to a separate file or descriptor instead (like a relief valve's drain pipe).

### Requirements for the Text Data

As already explained, every line must begin with a timestamp string. And that timestamp string must be immediately followed by a single space <0x20> or horizontal tab <0x09>.

The timestamp string can be given in one of the following four formats, and you must specify at runtime, via an option, which format is being used.

#### Calendar-Time Format: `YYYYMMDDhhmmss[.ddddddddd]`

This represents the time as a 14-digit number for year, month, day, hour, minute, and second. If more precision is needed, you can append a fractional part (down to the nanosecond). It doesn't matter which timezone this is expressed in (timezone has no meaning to this command).

To specify that the timestamp is given in this format, use the `-c` option; however, this is the default timestamp format for this command, so you may omit it.

#### UNIX-Time Format: `n[.ddddddddd]`

This represents the time as the number of seconds elapsed since the UNIX epoch (1970-01-01T00:00:00 in the UTC timezone). If more precision is needed, you can append a fractional part of up to 9 digits (*ddddddddd*).

To specify that the timestamp is given in this format, you must give the `-e` option.

#### Extended ISO 8601 Format: `YYYY-MM-DDThh:mm:ss[,ddddddddd]{+|-}hh:mm` or `YYYY-MM-DDThh:mm:ss[,ddddddddd]Z`

This represents year, month, day, hour, minute, and second in the extended ISO 8601 format. If more precision is needed, you can append a fractional part (after a `,` or `.`, down to the nanosecond). At the end, you can attach a timezone as either a `{+|-}hh:mm` offset or `Z` (meaning UTC). If the timezone is omitted, the string is treated as local time in whatever timezone the runtime environment is set to (it doesn't matter which timezone this is expressed in, since timezone has no effect on this command's actual behavior).

To specify that the timestamp is given in this format, you must give the `-I` option.

#### Elapsed Seconds Since Data Creation Started: `n[.ddddddddd]`

This represents the time as the number of seconds elapsed since some reference moment, such as when the timestamped data started being created. If more precision is needed, you can append a fractional part of up to 9 digits (*ddddddddd*).

To specify that the timestamp is given in this format, you must give the `-z` option.

## Arguments

### ratelimit

A transfer-rate-limiting parameter. It sets the maximum number of lines allowed to pass from standard input to standard output per unit of time. It is written in one of the following two formats.

#### Unit Time: `n[.ddddddddd][s|ms|us|ns|m|h|d]`

Limits the transfer rate so that only 1 line can pass per the specified unit of time. The format is `n[.ddddddddd][s|ms|us|ns|m|h|d]`, consisting of a numeric part `n[.ddddddddd]` and a unit part `[s|ms|us|ns|m|h|d]`.

The numeric part is set to an integer or real number that is 0 or greater. 0 means the transfer rate is not limited.

The unit part is written directly after the numeric part with no space, using `s`, `ms`, `us`, `ns`, `m`, `h`, or `d` to mean "seconds," "milliseconds," "microseconds," "nanoseconds," "minutes," "hours," and "days" respectively; however, the actual precision achieved depends on the runtime environment. If the unit is omitted, "seconds" is assumed.

#### Maximum Number of Lines and Unit Time: `N/n[.ddddddddd][s|ms|us|ns|m|h|d]`

Limits the transfer rate so that at most *N* lines can pass per the specified unit of time. *N* must be an integer from 1 to 65535.

Write the maximum number of lines *N*, immediately followed (with no space) by a slash `/`, immediately followed (again with no space) by the unit time. The unit time follows the format explained above.

### controlfile

If you give a filename instead of a string satisfying the *ratelimit* format above, this command regards it as a *controlfile*. In that case, it tries to read a string equivalent to *ratelimit* from within the *controlfile*. Since this command applies whatever new value you write into the file within a short time, this is convenient when you want to update *ratelimit* dynamically.

The syntax of the string you write inside the *controlfile* is exactly the same as when giving it as an argument, but if you write an invalid value, this command silently ignores it without raising an error. Also, before any valid value has ever been written, the default value is `1/86400d` (which, in effect, lets only the very first line through and nothing else).

You can choose one of the following file types to use as the *controlfile*.

#### Regular File

If you use a regular file as the *controlfile*, you must write the new parameter using **a mode that creates a new file (`O_CREAT` or `>`)**, not append mode (`O_APPEND` or `>>`). This is because, for a regular file, this command always watches the head of the file.

It is watched at 0.1-second intervals. If you want a newly written parameter to be applied immediately, send this command a SIGHUP after updating the file.

#### Character-Special File or Named Pipe

If you use either of these as the *controlfile*, either of the above write modes is fine. Also, since whatever you write is applied immediately, there is no need to send a signal.

From a performance standpoint, this is more advantageous than a regular file.

### file

The text file to use as the data source. If omitted, or if `-` is given, standard input is assumed. Multiple files can be specified.

## Options

### -c, -e, -I, -z

Specifies which format the timestamp string (the first field) is recorded in. -c, -e, -I, and -z respectively mean calendar time (with a 14-digit integer part in "YYYYMMDDhhmmss"), UNIX time (the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone), the extended ISO 8601 format, and the number of seconds elapsed since the timestamped data started being created; the options are mutually exclusive. If none of them is given, -c is assumed. The format details for each option are as follows.

* -c: calendar time
  * `YYYYMMDDhhmmss.ddddddddd`
* -e: UNIX time
  * `n.ddddddddd`
* -I: extended ISO 8601 format
  * `YYYY-MM-DDThh:mm:ss,ddddddddd{+|-}hh:mm`
  * `YYYY-MM-DDThh:mm:ss,dddddddddZ`
* -z: elapsed seconds since data creation started
  * `N.ddddddddd`

*YYYYMMDDhhmmss* is a 14-digit integer for year, month, day, hour, minute, and second in sequence; *n* is the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone; *N* is the number of seconds elapsed since the timestamped data started being created. In either case, you can also append a fractional part of up to 9 digits (*ddddddddd*) to indicate sub-second precision.

Note that with calendar time, or with the extended ISO 8601 format when its timezone is omitted, the meaning of the actual moment in time normally depends on the timezone. However, since this command decides whether to thin out a line based only on the relative time difference between lines, it makes no difference to the thinning result which timezone is set in the environment it runs in.

### -d *fd*|*file*

Normally, thinned-out lines are simply discarded, but with this option, they are instead written to the file specified by *file*, or the file descriptor specified by *fd*. If an integer is given, this option treats it as a file descriptor number, so if you want to specify a file whose name is an integer, include a path, e.g. `./2`.

### -k

Does not strip the timestamp field that was attached to the head of the input text data; outputs it as is. This applies both to lines that pass through to standard output without being thinned, and to lines sent elsewhere via the `-d` option when they are thinned out.

### -u

Sets the timezone to UTC. This is the same as setting the environment variable TZ to `UTC0`. This option exists for compatibility with other commands and has no effect at all on the actual behavior.

## Return Value

Returns 0 only if all of the specified files were processed successfully. Returns non-zero if an argument or option was invalid, or if processing failed for at least one file.

## Examples

While monitoring some web server's access log (access.log) in real time, thin it out to at most 1 line per 1.5 seconds because it's moving too fast to follow with your eyes. (uses tail(1) and [linets(1)](linets.man.en.md) together)

```sh:
$ tail -f access.log | linets -3 | relval 1/1.5s
```

Generate the numbers 1 through 100 at 0.1-second intervals, and observe exactly which lines get thinned out when limiting to at most 4 lines per 2 seconds. (uses seq(1), awk(1), and [tscat(1)](tscat.man.en.md) together)

```sh:
$ seq -f '%.3f' 0 0.1 9.9 | awk '{print $1,NR}' | tscat -kz | relval -z 4/2s
```

Likewise, generate the numbers 1 through 100 at 0.1-second intervals, and observe exactly which lines get thinned out when limiting to at most 2 lines per 1 second. (the lines that get thinned out differ between the "at most 4 per 2 seconds" case and the "at most 2 per 1 second" case)

```sh:
$ seq -f '%.3f' 0 0.1 9.9 | awk '{print $1,NR}' | tscat -kz | relval -z 2/1s
```

In the example above, output the lines that would normally be thinned out, and discard the lines that would normally pass through. (connect the thinned-out lines via the drain pipe to standard error, send the lines that would pass through to /dev/null to discard them, and then, on the outside, connect standard error to standard output)

```sh:
$ seq -f '%.3f' 0 0.1 9.9 | awk '{print $1,NR}' | tscat -kz | (relval -zd 2 2/1s >/dev/null) 2>&1
```

While monitoring some web server's access log (access.log) in real time, make it possible to dynamically change the rate limit via a parameter written into the named pipe "knob."

```sh:
# Operations in terminal 1
$ mkfifo knob
$ tail -f access.log | linets -3 | relval knob

# Operations in terminal 2
# (run in the same directory as terminal 1, after the operations in terminal 1)
$ cat > knob
1/1.5s⏎  ← Write a rate limit, and
1/500ms⏎ ← pressing [Enter] applies it
0⏎       ← to terminal 1's output. ("0" means no limit)
  :
```

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").

## See Also

[linets(1)](linets.man.en.md), [tscat(1)](tscat.man.en.md)
