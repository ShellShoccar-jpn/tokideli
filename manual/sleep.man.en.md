# SLEEP(1)

（日本語版は[こちら](sleep.man.ja.md)）

## Name

sleep - a "sleep" command supporting fractional seconds and time units

## Synopsis

```sh:
sleep duration
```

## Description

The POSIX-specified `sleep` command only guarantees an integer number of seconds, but this command accepts a decimal point so you can specify down to the nanosecond, and also accepts a unit such as "m" (minutes) or "h" (hours) so you can specify a length of time other than seconds.

Some major OSes, including Linux, already ship a `sleep` command that supports fractional seconds. This command exists to guarantee that such a `sleep` command is available on every OS that conforms to the POSIX standard.

## Arguments

### duration

The length of time to sleep for. It is given in the following format.

* `[-]A[.B][u]`
  * "A" is the integer part.
  * "B" is the optional decimal part (up to 9 digits).
  * "u" is the unit, and can be "s" (seconds; the default if omitted), "ms" (milliseconds), "us" (microseconds), "ns" (nanoseconds), "m" (minutes), "h" (hours), or "d" (days).
  * If you give it a negative value (with a leading "-"), this command does not sleep at all; it exits immediately with 0 (this is not an error).

For example, to sleep for 1.25 seconds, write `sleep 1.25` or `sleep 1.25s`; to sleep for 1 minute 30 seconds, write `sleep 90` or `sleep 1.5m`.

## Return Value

Returns 0 only if the argument was valid and the command finished sleeping normally. Returns non-zero if the argument was invalid or the command was interrupted partway through.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").
