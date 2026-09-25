# VALVE(1)

（日本語版は[こちら](valve.man.ja.md)）

## Name

valve - outputs data one byte or one character at a time, at a fixed time interval

## Synopsis

```sh:
valve [-c|-l] [-r|-s] [-p n] periodictime [file [...]]
valve [-c|-l] [-r|-s] [-p n] controlfile [file [...]]
```

## Description

Whereas cat(1) writes the given file(s) to standard output as fast as possible, this command writes out each byte, or each line (a string up to where `\n` appears), of the given *file* at a fixed transfer rate. Rather than sleeping for a fixed duration between the end of one write and the start of the next, it adjusts the sleep time as needed so that the *start* of each write happens at a fixed interval.

Each argument and option is explained separately below.

## Arguments

### file

The file to use as the data source. If omitted, or if `-` is given, standard input is assumed. Multiple files can be specified.

### periodictime

A parameter that specifies the transfer rate of the bytes or lines to be output. You can choose one of the following three formats.

* The time interval between the start of each byte/line write
  * `n[.d][s|ms|us|ns|m|h|d]`
* A data transfer rate
  * `n[.d]{bps|kbps|Mbps|Gbps|cps}`
* A valve open/close ratio
  * `{0|100}%`

#### For a Time Interval

Specify a number (which may include a fractional part) followed by a unit suffix. The available suffixes are `s` for seconds, `ms` for milliseconds, `us` for microseconds, `ns` for nanoseconds, `m` for minutes, `h` for hours, and `d` for days; if omitted, seconds are assumed.

For example, to specify 1.25 milliseconds, you can write `1.25ms`, or equivalently `0.00125`, `0.00125s`, `1250us`, and so on. To specify 1 hour 30 minutes, you can write `1.5h` or `90m`.

#### For a Data Transfer Rate

Specify a number (which may include a fractional part) followed by a unit suffix. The available suffixes are `bps` for bits per second, `kbps` for kilobits per second, `Mbps` for megabits per second, `Gbps` for gigabits per second, and `cps` for characters per second (1cps = 10bps). These suffixes cannot be omitted; if you omit one, the value is treated as a plain time interval in seconds instead of a data transfer rate, so be careful.

For example, to specify 1200bps, you can write `1200bps`, `1.2kbps`, `120cps`, and so on.

Note that this way of specifying the rate is only meaningful in byte-output mode (the -c option). It can also be given in line-output mode (the -l option), but it has no real meaning there, because internally, when "*N*bps" is given, the command simply converts it into a time interval of "(8/*N*)s".

#### For an Open/Close Ratio

Use the suffix `%`, and specify either `100%` (fully open) or `0%` (fully closed). An intermediate value such as `12%` cannot be specified (because the data transfer rate at an intermediate value would be undefined).

### controlfile

Whereas *periodictime* specifies the byte or line transfer rate directly as an argument, this method specifies it indirectly, via a file (*controlfile*) into which the transfer rate is written. Because this command watches the contents of *controlfile*, you can change the transfer rate dynamically while the command is running, by writing a new parameter into it.

The parameter you write follows the same format as *periodictime*. If an invalid string is given as the parameter, it is silently ignored without an error. The default, while no valid parameter has been given yet, is "0%".

You can choose one of the following file types to use as *controlfile*.

#### Regular File

If you use a regular file as *controlfile*, you must write the new parameter using the "create" mode (`O_CREAT` or `>`), not the "append" mode (`O_APPEND` or `>>`). This is because, for a regular file, this command always watches the head of the file.

The file is watched at an interval of 0.1 seconds. If you want a newly written parameter to be reflected immediately, send this command a SIGHUP after updating the file.

#### Character-Special File or Named Pipe

If you use one of these as *controlfile*, either of the above write modes is fine. Also, whatever you write takes effect immediately, so there is no need to send a signal.

From a performance standpoint, these are more advantageous than a regular file.

## Options

### -c, -l

Specifies the unit of the data to output. -c and -l mean byte-output mode and line-output mode respectively, and the two options are mutually exclusive. If neither is given, -c is assumed.

In byte-output mode, the command sleeps after each byte it outputs; in line-output mode, it sleeps after each line.

### -r, -s

Specifies how to handle the case where the sleep performed after each byte or line output runs longer than scheduled. -r and -s mean recovery mode and strict mode respectively, and the two options are mutually exclusive. If neither is given, -r is assumed.

Each mode works as follows.

#### -r: Recovery Mode

An ordinary UNIX-like OS is multi-user and multi-tasking, so the execution time of any given instruction cannot be predicted exactly. The same is true of a sleep instruction: extra sleep time beyond what was requested will occur, and its length varies each time.

However, once the sleep has finished, checking the clock tells you exactly how much extra time was slept. Recovery mode uses this to shorten the sleep on the next cycle and catch up on the delay. With this mode, you don't need to worry about delays accumulating over time.

#### -s: Strict Mode

On the other hand, if you shorten the sleep time on the next cycle whenever a delay occurs, the instantaneous data transfer rate goes up. If, say, an IoT device with a small receive buffer is connected downstream of the valve command, this could cause it to drop data. Strict mode addresses this: it never tries to catch up on a delay, and instead always honors the specified data transfer rate. With this mode, you don't need to worry about the receiving side dropping data.

### -p *n*

(Only on OSes supporting _POSIX_PRIORITY_SCHEDULING) Process priority setting. To improve the accuracy of the nanosleep() function used for adjusting the data transfer rate, setting *n* to 2 or 3 raises the process priority. *n* ranges over four levels from 0 to 3, and the default is 1.

Note that this option may require administrator privileges in some environments.

## Return Value

Returns 0 only if all of the specified files were processed successfully. Returns non-zero if an argument or option was invalid, or if processing failed for at least one file.

## Examples

Output file a.txt, then file b.txt from standard input, then file c.txt, in that order, at 300bps.

```sh:
$ cat b.txt | valve 300bps a.txt - c.txt
```

Display this command's own help message, one line every 0.5 seconds.

```sh:
$ valve -h 2>&1 | valve -l 0.5s
```

Likewise, display this command's own help message, while dynamically changing the data transfer rate via parameters written into the named pipe "faucet".

```sh:
# Operations in terminal 1
$ mkfifo faucet
$ valve -h 2>&1 | valve faucet

# Operations in terminal 2
# (run in the same directory as terminal 1, after the operations there)
$ cat > faucet
300bps⏎  ← write a data transfer rate, and
0%⏎      ← when you press [Enter],
10ms⏎    ← it is reflected in terminal 1's output.
  :
```

In a shell script, build a loop that repeats 100 times at a 1-second period, more precisely than with the sleep(1) command.

```sh:
awk 'BEGIN{for(i=0;i<100;i++){print i}}' |
valve -l 1s                              |
while read i; do
  (some processing here)
done
```

## Bugs

The command lets you specify a data output interval down to the nanosecond, but that does not guarantee it can actually produce an interval that precise. How precise an interval it can actually produce depends on the state of the OS and the performance of the hardware.

Likewise, you can specify a high data transfer rate (e.g. 1000Gbps), but that does not guarantee that rate can actually be achieved. That too depends on the OS and the hardware's performance.

The -p option may help improve the precision.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").
