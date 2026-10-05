# HEREWEGO(1)

（日本語版は[こちら](herewego.man.ja.md)）

## Name

herewego - sleep until a nice round time, then show the time it woke up (exited)

## Synopsis

```sh:
herewego [options] [+standby] interval[-premature]
```

## Description

Whereas the sleep(1) command sleeps for a given number of seconds, this command watches the clock (CLOCK_REALTIME) and sleeps until it reaches a "nice round" moment. Consequently, the sleep duration is not constant. When it finishes sleeping, it prints the time it woke up and exits immediately.

What counts as a nice round time is specified by *interval*. For example, setting *interval* to `1s` makes it sleep until the sub-second digits of the clock next become 0, and setting it to `60s` makes it sleep until the sub-minute digits next become 0 (see the "Arguments" section). As a special case, you can also specify `0`. In that case, it prints the current time immediately without sleeping and exits. This is handy for displaying the sub-second part of the current time, which the date(1) command does not support (see the "Examples" section).

If you give a value called *premature*, preceded by a minus sign `-` immediately after the *interval* argument, the sleep finishes that much earlier than the nice round time. Because it takes a little time between finishing the sleep and the command actually exiting after printing the time, this is useful when you want to finish early to account for that delay. For example, if you set *interval* and *premature* to `1s-3ms`, the command prints the current time and exits at the moment the sub-second digits of the clock reach 0.997 (see the "Arguments" section). Note that although setting *premature* makes the command's actual exit time earlier, it does not round up the printed exit time — in other words, *premature* has no effect on the time that gets printed. Also, the value given for *premature* must be smaller than *interval* (otherwise the command exits with an error).

If you give a value called *standby*, preceded by a plus sign `+`, as the argument immediately before *interval*, you can specify an unconditional sleep duration, just like the sleep(1) command. When this is given, the command sleeps for that duration first, and then sleeps until the next nice round time after that. The reason such an argument is needed is to avoid a situation where this command is started just before a nice round moment, misses it while still preparing to run, and ends up finishing late. If, for example, there is a chance that it could take up to 30 milliseconds from when the command starts to when it begins processing, you should set `+30ms` (see the "Arguments" section).

The exit time that gets printed can be selected, via an option, from calendar time (the 14-digit number YYYYMMDDhhmmss) following the timezone set for the OS, UNIX time (the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone), or extended ISO 8601 format, and a sub-second part can optionally be appended as well. The default is calendar time without a sub-second part.

Each argument and option is explained in detail below.

## Arguments

### interval

Sets the length of time used to determine a nice round time. Strictly speaking, a moment is considered "nice and round" when the time indicated by the clock (CLOCK_REALTIME) is evenly divisible by the duration given in this argument, and the command sleeps until the next such moment arrives. For example, setting it to `1s` makes the command sleep until the sub-second digits of the clock next become 0, and setting it to `60s` makes it sleep until the sub-minute digits next become 0.

The time format is a number (which may include a fractional part) followed by a unit suffix. The suffix can be `s` for seconds, `ms` for milliseconds, `us` for microseconds, `ns` for nanoseconds, `m` for minutes, `h` for hours, or `d` for days; if omitted, seconds are assumed. For example, to specify 0.5 seconds you can write `0.5s`, or also `0.5` or `500ms`. To specify a nice round time on every hour, you can write `1h` or `3600s`.

This argument cannot be omitted.

### premature

Makes the sleep finish earlier, by the length of time given by *premature*, than the exit time determined by the *interval* argument. However, the time printed at exit is the same as when the *premature* argument is not given. For example, if *interval* is set to `1s`, the sleep finishes when the sub-second digits of the clock become 0; but if you additionally set *premature* to make it `1s-3ms`, the command prints the current time and exits when the sub-second digits reach 0.997.

The time format is basically the same as for *interval*, but it must be written immediately after the *interval* argument with no space, as a minus sign `-` followed immediately by the value with no space. Also, the value given must be smaller than *interval*, or the command exits with an error.

### standby

Before beginning the nice-round-time sleep determined by *interval*, the command sleeps for the duration given by this argument.

The time format is basically the same as for *interval*, but this argument, when given, must always appear as the argument immediately preceding *interval*, and must begin with a plus sign `+`.

## Options

### -0, -3, -6, -9

Specifies the precision of the exit time that is printed. -0, -3, -6, and -9 mean second, millisecond, microsecond, and nanosecond precision respectively, and these options are mutually exclusive. If none of them is given, -0 is assumed.

### -c, -e, -I

Specifies the format of the exit time that is printed. -c, -e, and -I mean, respectively, calendar time (a 14-digit integer part of the form "YYYYMMDDhhmmss"), UNIX time (the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone), and time in extended ISO 8601 format; these options are mutually exclusive. If none of them is given, -c is assumed. The format details for each option are as follows.

* -c: calendar time
  * `YYYYMMDDhhmmss.ddddddddd`
* -e: UNIX time
  * `n.ddddddddd`
* -I: extended ISO 8601 format
  * `YYYY-MM-DDThh:mm:ss,ddddddddd+hh:mm`

*YYYYMMDDhhmmss* is a 14-digit integer made of the year, month, day, hour, minute, and second; *n* is the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone; and *YYYY-MM-DDThh:mm:ss,ddddddddd+hh:mm* is the year, month, day, hour, minute, second, sub-second part, and timezone based on extended ISO 8601. In every case, the -3, -6, or -9 option lets you append up to a 9-digit fractional part (*ddddddddd*) to show the sub-second time.

Note that the 14-digit integer value depends on the timezone set for the OS. If you want to specify the timezone explicitly, set the environment variable TZ. (See the "Examples" section.)

### -p *n*

(Only on OSes supporting _POSIX_PRIORITY_SCHEDULING) Process priority setting. To raise the precision of the nanosleep() function used for adjusting the data transfer rate, setting *n* to 2 or 3 raises the process priority. *n* ranges over four levels from 0 to 3, and the default is 1.

Note that this option may require administrator privileges in some environments.

### -u

Sets the timezone to UTC. This is the same as setting the environment variable TZ to `UTC0`. This option affects the displayed values when the -c or -I option is given.

## Return Value

Returns 0 only when it finished successfully. Returns non-zero if an argument or option was invalid, or if the command was interrupted by, for example, [Ctrl]+[C].

## Examples

Sleep until the moment the seconds digit of the clock next becomes 0, 15, 30, or 45. To account for the fact that the computer's shutdown processing takes a little time, finish 3 milliseconds early, and, to account for the possibility that starting the command itself may take a little time, insert a 0.1-second sleep beforehand. Display the exit time in extended ISO 8601 format down to millisecond precision, using the PST (Pacific Standard Time, UTC-8) timezone.

```sh:
$ export TZ=PST8
$ herewego -3I +0.1 15s-3ms
```

Display the current time down to microsecond precision, in extended ISO 8601 format, using the JST-9 timezone.

```sh:
$ TZ=JST-9 herewego -6I 0
```

Do some processing in a loop while hitting a web page at a precise 3-second interval, as long as `curl`'s execution time is sufficiently shorter than 3 seconds.

```sh:
$ while herewego 3s >/dev/null; do
    curl https://api.example.com/SOME/ENDPOINT
  done
```

You can do something similar with [valve(1)](valve.man.en.md)'s `-l` option, but `valve` tries to keep its own output paced on an absolute time schedule, so if `curl` occasionally takes longer than 3 seconds, whatever built up during that time gets sent out all at once (practically simultaneously), i.e. a burst. With this command, if processing happens to run long and it misses one nice round time, it doesn't try to make up for it afterward — it just waits for the next nice round time before resuming, so this kind of burst never happens. If you need to strictly honor a rate limit, this command is a better fit than `valve`.

## Bugs

This command can set a nice round time down to nanosecond precision, and can display the exit time down to nanosecond precision as well, but that does not mean it can actually sleep or display the time with that exact precision in practice. How much precision can actually be achieved depends on the state of the OS and the performance of the hardware.

Using the -p option may improve the precision.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").
