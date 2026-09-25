# TSCAT(1)

（日本語版は[こちら](tscat.man.ja.md)）

## Name

tscat - timestamp-aware version of the "cat" command

## Synopsis

```sh:
tscat [-c|-e|-I|-z] [-Z] [-1kuy] [-p n] [file [...]]
```

## Description

Unlike cat(1), which writes the given files to standard output as fast as possible, this command treats the number in the first field (the first column) of each line of the given file as a timestamp, and outputs that line the very moment the time it indicates arrives. Note that the timestamp string must be followed by a single half-width space <0x20> or horizontal tab <0x09>, and that part is stripped off before the line is sent to output. This is, in a sense, the exact opposite of what the [linets(1)](linets.man.en.md) command does.

For example, if you feed data like the following into this command,

```text:
20220801000000.000 1st_line
20220801000000.500 2nd_line
20220801000001.000 3rd_line
         :             :
```

it is written to standard output with the following content, at the following timing.

```text:
1st_line       ← output at 2022-08-01 00:00:00.0
2nd_line       ← output at 2022-08-01 00:00:00.5
3rd_line       ← output at 2022-08-01 00:00:01.0
   :
```

However, in most cases the timestamps added by the [linets(1)](linets.man.en.md) command already point to a moment in the past, so feeding them to this command would make every line come out immediately, and the original timing would not be reproduced. To solve this problem, the -Z option (described below) is provided. When -Z is given, this command computes the time difference between each line's timestamp and the timestamp of the first line, and outputs each line once that much time has elapsed. This lets you reproduce the original, line-by-line arrival timing of the data.

The individual arguments and options are explained below.

## Arguments

### file

The file to use as the data source. If omitted, or if `-` is given, standard input is assumed. You may specify more than one.

## Options

### -c, -e, -I, -z

These options specify the format in which the timestamp string (the first column) is recorded. -c, -e, -I, and -z respectively mean calendar time (a 14-digit integer part of "YYYYMMDDhhmmss"), UNIX time (the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone), extended ISO 8601 format, and the number of seconds elapsed since the timestamped data started being produced; these options are mutually exclusive. If none of them is given, -c is assumed. The format details for each option are as follows.

* -c: calendar time
  * `YYYYMMDDhhmmss.ddddddddd`
* -e: UNIX time
  * `[+|-]n.ddddddddd`
* -I: extended ISO 8601 format
  * `YYYY-MM-DDThh:mm:ss,ddddddddd{+|-}hh:mm`
  * `YYYY-MM-DDThh:mm:ss,dddddddddZ`
* -z: seconds elapsed since the data started being produced
  * `[+|-]N.ddddddddd`

*YYYYMMDDhhmmss* is a 14-digit integer made up of the year, month, day, hour, minute, and second; *n* is the number of seconds elapsed since 1970-01-01T00:00:00 in the UTC timezone; *N* is the number of seconds elapsed since the timestamped data started being produced. In every case, you may also append a decimal part of up to 9 digits (*ddddddddd*) to show sub-second precision.

Note that the 14-digit integer value depends on the timezone configured on the OS. If you want to specify the timezone explicitly, set it via the environment variable TZ. (See the "Examples" section.)

If a negative value is given while the -z option is in effect, the origin time used for the elapsed-seconds count from then on is reset to the moment that negative value was received.

### -Z

Instead of outputting each line the moment the time indicated by its timestamp arrives, this command computes the time difference between each line's timestamp and the timestamp of the first line, and outputs each line once that much time has elapsed. With this option, you can reproduce, at any time, the original arrival timing of data recorded by the [linets(1)](linets.man.en.md) command.

### -1

When the command starts, it outputs one line (LF) unrelated to the data coming from standard input. The purpose of this option is to prevent deadlocks when building a bidirectional pipe between this command and an AWK script or shell script. (See the AWK script example in the "[Examples](#examples)" section.)

### -k

Sends the input text data to standard output as is, without removing the timestamp column that was prefixed to it.

### -u

Sets the timezone to UTC. This is the same as setting the environment variable TZ to `UTC0`. This option affects the displayed output when the -c option is given.

### -y

Switches to "typing mode." Normally, each line of the timestamped data is output as a line (that is, with its newline code attached). In this mode, however, no newline code is output for any line except a blank one.

For example, suppose you have a file (mytyping.txt) whose timestamps represent the time elapsed since the moment the first character, "H," was typed.

```text:mytyping.txt
0 H
0.250 e
0.437 l
0.550 l
0.705 o
0.971 
1.855 w
1.969 o
2.104 r
2.198 l
2.302 d
2.835 !
3.113 
```

Running this command with both the -z and -y options, as in `tscat -zy mytyping.txt`, displays the following string on the screen while reproducing the original typing.

```text:
Hello
world!
```

This mytyping.txt file can be created by combining the [typeliner(1)](typeliner.man.en.md) and [linets(1)](linets.man.en.md) commands, as follows.

```sh:
$ typeliner -e | linets -3Z > mytyping.txt
Hello⏎          ← run the command,
world!⏎         ← type this, then press [Ctrl]+[D]
```

See the [typeliner(1)](typeliner.man.en.md) manual for details.

### -p *n*

(Only on OSes supporting _POSIX_PRIORITY_SCHEDULING) Process priority setting. To improve the accuracy of the nanosleep() function used to adjust the data transfer rate, setting *n* to 2 or 3 raises the process priority. *n* ranges over four levels from 0 to 3, and the default is 1.

Note that this option may require administrator privileges in some environments.

## Return Value

Returns 0 only if all of the given files were processed successfully; returns a value other than 0 if the arguments or options were invalid, or if processing failed for even one file.

## Examples

Use the [linets(1)](linets.man.en.md) command to record the behavior of the ping(8) command with millisecond-level timing, then play it back.

```sh:
$ ping -c 10 example.cpm | linets -3 > ping_result.log
$ tscat -Z ping_result.log
```

Use the [typeliner(1)](typeliner.man.en.md) and [linets(1)](linets.man.en.md) commands to record your own typing, then play it back. (Here the timestamp uses the calendar-time format, with the timezone set to `JST-9`.)

```sh:
$ export TZ=JST-9
$ typeliner -e | linets -c3 > mytyping.txt
(type some characters, then press [Ctrl]+[D])
$ tscat -cy mytyping.txt
(your typing from a moment ago is replayed on the screen)
```

Inside an AWK script, sleep more accurately and more lightly than by calling the external sleep(1) command. (This usage requires the -1 option and one named-pipe file.)

```awk:
BEGIN {
  cmd_wait="tscat -1z named_pipe";

  cmd_wait | getline dummy;  # Set the timer to 0
  print "The time is set 0. Please wait until the time will be 1...";

  print "1 " > "named_pipe"; fflush();
  cmd_wait | gettime dummy;  # Wait for about 1 sec

  print "Now the time is 1. Please wait until the time will be 2...";

  print "2 " > "named_pipe"; fflush();
  cmd_wait | gettime dummy;  # Wait for about 1 sec again

  print "Finish";
  close("named_pipe");
  close(cmd_wait);
}
```

## Bugs

This command lets you specify timestamps down to the nanosecond, but that does not mean it can always reproduce timing with that precision. How high a precision it can actually achieve depends on the state of the OS and the performance of the hardware.

The -p option may help improve the precision.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").

## See Also

[linets(1)](linets.man.en.md), [LINETS & TSCAT tutorial](linets_and_tscat.en.md), [typeliner(1)](typeliner.man.en.md)
