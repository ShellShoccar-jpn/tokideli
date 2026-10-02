# OOBLECK(1)

（日本語版は[こちら](oobleck.man.ja.md)）

## Name

oobleck - Output the currently held line only when the next line does not arrive for a while

## Synopsis

```sh:
oobleck [-d fd|file] [-p n] holdingrule [file]
oobleck [-d fd|file] [-p n] controlfile [file]
```

## Description

Oobleck, a fluid made by mixing water into cornstarch (or a similar powder) at a certain ratio, stays afloat as long as you keep walking on it, but sinks as soon as you stop. This command behaves in a similar way: as long as data (line-oriented text) keeps arriving, it does nothing with it and discards it. However, once the arrival of data pauses for a certain length of time, it takes out the last line that had arrived and sends it to the standard output.

### Intended Use

This command is useful in fields such as IoT and M2M. For example, suppose there is a device that operates a potentiometer and outputs data only while its value is changing. What you often want is not the values while the knob (slider) is being adjusted, but only the final value once the adjustment has settled. This is where this command becomes handy: if you convert the device's data into line-oriented text and feed it to this command, only the final settled value is obtained from the standard output.

## Arguments

### file

The file to be used as the data source. If omitted, or if `-` is specified, the standard input is assumed. You cannot specify more than one.

### holdingrule

A string that specifies the rule for holding the lines arriving from the data source (which must be in text-data format). It can be specified in either of the following two formats.

#### a. Holding-time ( *holding-time* )

The maximum length of time to keep holding the last line that arrived from the data source (which must be in text-data format) since it arrived. If the next line does not arrive even after the time specified by this argument has elapsed since the current line arrived, the currently held line is sent to the standard output.

The time specified by *holding-time* is the interval between the moment the last byte (LF) of the current line arrives and the moment the first byte of the next line arrives.

The value of *holding-time* can be specified in either of the following ways.

* `n[.d][s|ms|us|ns|m|h|d]`
  * Specify the length of the holding time in seconds (`s`), milliseconds (`ms`), microseconds (`us`), nanoseconds (`ns`), minutes (`m`), hours (`h`), or days (`d`).
  * When specifying it in seconds, the unit string can be omitted.
  * You can include not only integers but also decimal fractions.
  * For instance, to specify 1.25 milliseconds, you can write `1.25ms`, as well as `0.00125`, `0.00125s`, or `1250us`. To specify 1 hour 30 minutes, you can write `1.5h` or `90m`.
* `{0|100}%`
  * `0%` outputs the last line that arrived immediately, without holding it. In other words, no matter how fast the data arrives, all of it is sent to the standard output. ("0% block")
  * `100%` holds the last line that arrived forever. In other words, no data is ever sent to the standard output. ("100% block")
  * Both of these are meaningful when you want to temporarily block or pass through all the data while using a controlfile (described below).
  * Only `0%` or `100%` can be specified; an intermediate value such as `12%` cannot be specified.

#### b. Number-of-lines and holding-time ( *number-of-lines* and *holding-time* )

This format specifies two parameters. The latter, *holding-time*, is exactly the same as described above.

The former, *number-of-lines*, is the number of lines to hold. If you set this to *n*, this command always keeps the latest *n* lines of the incoming data in memory, and outputs them once the specified *holding-time* has elapsed.

The format is `number@time`, where each part means the following.

* `number` is the *number-of-lines*. You can specify a natural number from 1 to 256.
* `@` is the delimiter separating the two values. No whitespace is allowed between the delimiter and the numbers.
* `time` is the *holding-time*. Its format is the same as described in the previous section, so refer to that.

For example, if you want to output the last 3 lines that had arrived when the incoming data pauses for 500 milliseconds, you can specify "3@500ms".

### controlfile

If you specify a filename instead of a string satisfying the *holdingrule* format described above, this command regards it as the specification of a file called *controlfile*. In this case, it tries to read the string corresponding to *holdingrule* from the *controlfile*. Since this command reflects any new value written by the user within a short time, this is convenient when you want to update the *holdingrule* dynamically.

You can choose one of the following file types to use as the *controlfile*.

#### Regular Files

If you use a regular file as the *controlfile*, you must write the new parameter using **the "create" mode (`O_CREAT` or `>`)**, not the "append" mode (`O_APPEND` or `>>`). This is because, in the case of a regular file, this command always watches the beginning of the file.

The file is checked every 0.1 seconds. If you want a newly written parameter to be reflected immediately, send this command a SIGHUP after updating the file.

#### Character-Special Files or Named Pipes

If you use either of these as the *controlfile*, you may write in either of the above modes. Also, since the written content is reflected immediately, there is no need to send a signal.

From a performance standpoint, these are more advantageous than regular files.

## Options

### -d {fd|file}

Normally, when the next line arrives while a line is being held, the held line is discarded. However, if this option is specified, it is not discarded, but instead written out to the specified file descriptor or file.

If an integer is specified, it is regarded as a file descriptor number (`fd`). If a non-integer string is specified, it is regarded as a filename (`file`). If you want to specify a file whose name is an integer, include a path such as `./2`.

When a filename is given and that file already exists, its existing content is kept, and the discarded lines are appended after it (the file is never truncated and rewritten from the beginning).

### -p *n*

(Only on OSes supporting _POSIX_PRIORITY_SCHEDULING) Process priority setting. To improve the accuracy of the nanosleep() function used for adjusting the data transfer rate, setting *n* to 2 or 3 raises the process priority. *n* ranges over four levels from 0 to 3, and the default is 1.

Note that this option may require administrator privileges in some environments.

## Return Value

Returns 0 only when the specified file was processed successfully. Returns a value other than 0 if the arguments or options are invalid, or if processing the file fails.

## Examples

From data arriving intermittently on the serial device /dev/ttyS1, which communicates at 9600bps, send to the standard output only the lines for which the next line did not arrive within 500 milliseconds. (Used together with the cu(1) command)

```sh:
$ cu -s 9600 -l /dev/ttyS1 | oobleck 500ms
```

The above example outputs only the last line; the following outputs the last 3 lines instead.

```sh:
$ cu -s 9600 -l /dev/ttyS1 | oobleck 3@500ms
```

For the same serial device, allow the line-holding time to be specified indirectly through a file named `./control`. Also, send any data that was not sent to the standard output to the standard error output (file descriptor number 2).

```sh:
$ cu -s 9600 -l /dev/ttyS1 | oobleck -d 2 ./control
```

## Bugs

The *holding-time*, both in the argument and in the value specified inside a *controlfile*, can be specified down to nanosecond precision, but that does not mean a holding time of that precision can actually be achieved. How high a precision can be achieved depends on the state of the OS and the performance of the hardware.

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").
