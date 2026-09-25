# WAITILL(1)

（日本語版は[こちら](waitill.man.ja.md)）

## Name

waitill - sleep until a deadline instead of for a duration

## Synopsis

```sh:
waitill [-lu] [-p n] abstime
waitill [-lu] [-p n] -e length
```

## Description

When you sleep with the sleep(1) command, you specify how long to sleep. This command, by contrast, sleeps by specifying a deadline — that is, the moment at which the sleep is to end. The advantage of specifying a deadline is that it makes it easy to build a loop that repeats at an exact interval. Suppose you want a loop that does something (call it "task A") every *t* seconds. If you insert "`sleep t`" into the loop, each round of the loop will take slightly more than *t* seconds, because neither task A nor the loop's own processing takes zero time. In a case like this, if you use this command to set the sleep deadline to "`waitill $((t0+n*t))`" (where *t*0 is the time the loop started and *n* is the round number), each round of the loop can be kept to exactly *t* seconds, as long as the combined execution time of task A and the loop's own processing stays under *t* seconds.

## Arguments

### abstime

The sleep deadline (an absolute time). This command keeps sleeping until the time set here arrives, then terminates immediately once it does.

The time can be specified in any of the following formats.

* `YYYYMMDDhhmmss[.d]`
  * Calendar time.
  * "`YYYYMMDDhhmmss`" is the integer part: year, month, day, hour, minute, and second.
  * "`.d`" is the optional decimal part.
  * This format has no place to specify a timezone; the timezone configured on the computer running this command is used instead (changeable via the environment variable `TZ` or the -u option).
* "`YYYY-MM-DDThh:mm:ss[,d][+hh:mm|Z]`"
  * Extended ISO 8601 format.
  * "`YYYY-MM-DDThh:mm:ss`" is the part down to the second: year, month, day, hour, minute, and second.
  * "`.d`" is the optional decimal part.
  * "`+hh:mm`" or "`Z`" is the optional part indicating the timezone. If omitted, the timezone configured on the computer running this command is used (changeable via the environment variable `TZ` or the -u option).
* "`{+|-}n[.d]`"
  * UNIX time (the number of seconds elapsed since 1970-01-01T00:00:00Z).
  * "`n`" is the integer part.
  * "`.d`" is the optional decimal part.
  * Always attach a sign ("`+`" or "`-`") in front of the integer part, so this can be distinguished from a calendar-time value.
* `hhmmss[.d]`, `hhmm`, `mm`, `mmss.[d]`, `ss.[d]`, `.[d]`
  * Abbreviated forms of calendar time.
  * Any year/month/day/hour/minute/second unit that is not specified is assumed to take whatever value produces the nearest moment in the future, based on the clock at the time this command runs. For example:
    1. If this command is run at 2025-04-12T23:56:55 with abstime set to "`57`"
       * "`57`" is treated as the minute unit (mm), and the second unit (ss) is treated as if it had been set to "`00`".
       * In this case, simply taking the year/month/day/hour units (YYYYMMDDhh) as-is from the clock already gives the nearest future moment.
       * That is, abstime becomes 2025-04-12T23:57:00.
    2. If this command is run at 2025-04-12T23:57:05 with abstime set to "`57`"
       * "`57`" is treated as the minute unit (mm), and the second unit (ss) is treated as if it had been set to "`00`".
       * In this case, taking the year/month/day/hour units (YYYYMMDDhh) as-is from the clock would give a moment in the past.
       * So the unit one level above the one given in the argument (here, hh) is incremented by 1 to produce the nearest future moment.
       * That is, abstime becomes 2025-04-13T00:57:00.
  * You cannot omit the decimal point in the `mmss` or `ss` forms — always write "`mmss.`" or "`ss.`", and so on. If you write "`1234`" or "`34`" instead, there would be no way to tell it apart from `hhmm` or `mm`, so it would be interpreted as 12:34 or minute 34.

### length

The number of seconds elapsed since the time set in the environment variable [`WT_EPOCH`](#wt_epoch). When the -e option (see [Epoch Mode](#epoch-mode)) is given, this is interpreted as this length instead of as abstime.

The elapsed seconds can be specified in the following format.

* `n[.d]`
  * "`n`" is the integer part.
  * "`.d`" is the optional decimal part.

For details about this argument, see the "[Modes](#modes)" section.

## Options

### -e

Tells this command to run in epoch mode.

For details, see the "[Modes](#modes)" section.

### -l

Lists the sleep's end time, in three formats, each prefixed with a label at the start of its line:

```text:
abstime_iso YYYY-MM-DDThh:mm:ss,ddddddddd+hh:mm
abstime_uni {+|-}n.ddddddddd
abstime_cal YYYYMMDDhhmmss.ddddddddd
```

* `abstime_iso`: extended ISO 8601 format
* `abstime_uni`: UNIX time
* `abstime_cal`: calendar time

All three formats represent the same moment in time, shown down to the nanosecond. Thanks to the label at the start of each line, you can also pick out just one format with `grep`, `awk`, and so on.

### -u

Sets the timezone to UTC. This is the same as setting the environment variable TZ to `UTC0`. It affects how abstime is interpreted when given as calendar time or as ISO 8601 format without an explicit timezone, and it affects the time shown by the -l option.

### -p *n*

(Only on OSes supporting _POSIX_PRIORITY_SCHEDULING) Process priority setting. To improve the accuracy of the nanosleep() function used to adjust the data transfer rate, setting *n* to 2 or 3 raises the process priority. *n* ranges over four levels from 0 to 3, and the default is 1.

Note that this option may require administrator privileges in some environments.

## Environment Variables

### WT_EPOCH

The environment variable used to set the reference time for [epoch mode](#epoch-mode).

The time can be set in any of the following formats.

* `YYYYMMDDhhmmss[.d]`
  * Calendar time.
  * "`YYYYMMDDhhmmss`" is the integer part: year, month, day, hour, minute, and second.
  * "`.d`" is the optional decimal part.
  * This format has no place to specify a timezone; the timezone configured on the computer running this command is used instead (changeable via the environment variable `TZ` or the -u option).
* "`YYYY-MM-DDThh:mm:ss[,d][+hh:mm|Z]`"
  * Extended ISO 8601 format.
  * "`YYYY-MM-DDThh:mm:ss`" is the part down to the second: year, month, day, hour, minute, and second.
  * "`.d`" is the optional decimal part.
  * "`+hh:mm`" or "`Z`" is the optional part indicating the timezone. If omitted, the timezone configured on the computer running this command is used (changeable via the environment variable `TZ` or the -u option).
* "`{+|-}n[.d]`"
  * UNIX time (the number of seconds elapsed since 1970-01-01T00:00:00Z).
  * "`n`" is the integer part.
  * "`.d`" is the optional decimal part.
  * Always attach a sign ("`+`" or "`-`") in front of the integer part, so this can be distinguished from a calendar-time value.

For details, see the [Epoch Mode](#epoch-mode) part of the "[Modes](#modes)" section.

## Modes

This command runs in one of two modes, depending on whether it is started with the -e option.

### Basic Mode

This is the mode used when the command is started without the -e option. The argument right after the option is treated as abstime, and this command sleeps until that time arrives.

This mode is simple and easy to use.

### Epoch Mode

This is the mode used when the command is started with the -e option.

The argument right after the option is treated as length, and this command computes the sleep deadline from that value and the time set in the environment variable [`WT_EPOCH`](#wt_epoch), then sleeps. Specifically:

1. It reads the time set in the environment variable [`WT_EPOCH`](#wt_epoch), and the number of seconds set in the first argument, length.
2. It adds the latter to the former, and treats that moment as the sleep's end time.

It then sleeps until the computed time arrives.

This mode is convenient when you want to set the sleep's end time relative to some reference moment, and it lets you keep your program's code simple.

## Return Value

Returns 0 only if it completed successfully; returns a value other than 0 if the arguments or options were invalid.

## Examples

Sleep until exactly the top of the next 27th minute. (If the command starts between minute 0 and 26 of the hour, it sleeps until minute 27 of that hour; if it starts between minute 27 and 59, it sleeps until minute 27 of the next hour.)

```sh:
$ waitill 27
```

Since it's hard to tell from the above just when the sleep will actually end, show the sleep deadline (the end time) as well.

```sh:
$ waitill -l 27
```

Set a reference time, then build a loop that runs tasks 1 and 4 seconds after it. (Use the [herewego(1)](herewego.man.en.md) command to set the reference time.)

```sh:
export WT_EPOCH=+$(herewego -3e 0)
n=0
while :; do
  echo "begin of the loop"
  waitill -e $((1+n*4))
  echo "task 1"
  waitill -e $((4+n*4))
  echo "task 2"
  n=$((n+1))
done
```

## Bugs

This command can display and set the sleep deadline down to nanosecond precision, but that does not mean it can always produce sleep timing with that precision. How high a precision it can actually achieve depends on the state of the OS and the performance of the hardware.

The -p option may help improve the precision.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").
