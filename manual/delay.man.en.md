# DELAY(1)

（日本語版は[こちら](delay.man.ja.md)）

## Name

delay - Send each byte arriving from the standard input to the standard output after delaying it for a fixed amount of time

## Synopsis

```sh:
delay [-s|-o|-d] [-v] [size@]time
delay [-s|-o|-d] [-v] controlfile
```

## Description

Each byte that arrives from the standard input is held in an internal ring buffer until the specified delay time (*time*) has elapsed since it arrived, and is then sent to the standard output. In other words, this is a "delay filter."

Bytes that have arrived but whose holding time has not yet elapsed are stored in the internal ring buffer, whose capacity is given by *size*. What happens when this buffer becomes full for some reason can be chosen via the [options](#options) (`-s`/`-o`/`-d`).

When the standard input reaches EOF, this command does not terminate immediately. For each byte still remaining, unsent, in the buffer at that point, it waits precisely until that byte's scheduled time (arrival time + *time*) before sending it, and terminates only once the buffer has become empty.

## Arguments

### time

The parameter that specifies the delay time.

It consists of a real number greater than or equal to 0, immediately followed (with no space) by an optional unit string. If the unit is omitted, seconds (`s`) are assumed.

* `s` ... seconds (default)
* `ms` ... milliseconds
* `us` ... microseconds
* `ns` ... nanoseconds
* `m` ... minutes
* `h` ... hours
* `d` ... days

Specifying "0" means no delay at all (output happens at the same moment as arrival).

If you specify a value so extreme that it exceeds the range of the data type used internally to hold it, this is treated as an error.

### size

The parameter that specifies the capacity, in bytes, of the ring buffer that holds bytes which have arrived but have not yet been sent.

It consists of a real number greater than 0, immediately followed (with no space) by an optional unit string. If the unit is omitted, bytes (`B`) are assumed.

* `B` ... bytes (default)
* `kB` ... kilobytes (x1000)
* `KB` ... kibibytes (x1024)
* `KiB` ... kibibytes (x1024)
* `MB` ... megabytes (x1000^2)
* `MiB` ... mebibytes (x1024^2)
* `GB` ... gigabytes (x1000^3)
* `GiB` ... gibibytes (x1024^3)

If the value converted to bytes is not an integer, the fractional part is truncated (for example, "0.1KiB" is 102.4 bytes, which is truncated to 102 bytes). If the truncated result is 0 bytes, this is treated as an error.

To distinguish this parameter from *time*, you must append the character `@` immediately after it. If omitted, "1MiB" is assumed.

If you specify a size that amounts to 80% or more of the total physical memory installed on the machine this command is running on, this is treated as an error.

Note that this 80% figure is not compared against the *size* value itself, but against the amount of memory this command actually allocates internally. The ring buffer stores, alongside each held byte, the timestamp of when that byte arrived, so the actual memory footprint ends up considerably larger than the *size* value alone would suggest. As a rough guide, on a typical 64-bit environment, holding one byte actually costs around 24 bytes of memory, so the practical upper bound on *size* is not 80% of installed memory but roughly 1/24 of it (on the order of 3-4%). This multiplier varies depending on the OS and CPU architecture (e.g. 32-bit vs. 64-bit).

### controlfile

Specifies a file (the *controlfile*) used to dynamically set the `[size@]time` parameters described above.

If the string given at this position does not match the `[size@]time` syntax, it is instead treated as the name of a *controlfile*. If you need to use a filename that could be mistaken for the syntax above (such as a name consisting only of digits), give it together with a path, e.g. `./1`.

When using a *controlfile*, unlike the `[size@]time` argument form, the *time* part may be omitted. That is, the accepted syntax is `{time|size@time|size@}` (writing only `size@` means "update the size only, leave the time unchanged").

Until a valid parameter has been read from the *controlfile* for the first time, the initial values are time=0 and size=1MiB.

Whenever a line read from the file matches the syntax above and each given value is valid, the corresponding parameter(s) are updated on the spot. If the line does not match the syntax, or a value is invalid, it is silently ignored with no error. If both time and size are given on the same line, both parts must be valid, or the whole line is ignored (for example, if the `size` part of `size@time` is valid but the `time` part is not, the size change is not applied either).

The file used as the *controlfile* may be either a character-special file such as a named pipe, or a regular file.

#### Regular file

If you use a regular file as the *controlfile*, you must write the new parameter into it using **create mode (`O_CREAT` or `>`)**, not append mode (`O_APPEND` or `>>`). This is because, for a regular file, this command always watches the head of the file.

The watch interval is 0.1 seconds. If you want a newly written parameter to be applied immediately, send this command a SIGHUP after updating the file.

#### Character-special file or named pipe

If you use either of these as the *controlfile*, you may write in either of the modes above. Whatever you write (one line's worth) is applied immediately, so there is no need to send a signal.

From a performance standpoint, this is more advantageous than a regular file.

## Options

### -s, -o, -d

Specifies what to do when a new byte arrives while the ring buffer is full. These three options are mutually exclusive; if more than one is given, the last one takes effect.

* `-s` ... (Default) Suspend. Block (pause) reading from the standard input until a slot in the buffer becomes free.
* `-o` ... Overwrite. Discard the oldest unsent byte and store the new byte in its place.
* `-d` ... Disconnect. Terminate this command immediately without sending any of the buffer's contents.

### -v

Raises the verbosity level by one each time it is given. `-v` sets level 1, `-vv` sets level 2, and the higher the level, the more detail about internal behavior is reported to the standard error output.

At level 1, at minimum, every time the buffer becomes full (overflows), this is reported together with the action taken this time (suspend, overwrite, or disconnect).

### -h

Displays the usage, the last-updated date, the license, and so on, to the standard error output.

## Return Value

Returns 0 when the standard input reaches EOF and all of the buffer's contents have been sent, terminating normally. Returns 1 when this command was forced to terminate because the buffer was full, due to the `-d` option. Returns a value other than 0 for any other abnormal termination (such as an argument error).

## Examples

Delay every arriving byte by 1 second before sending it out. (With the default settings, data that doesn't fit in the buffer blocks until space frees up, so even a large amount of data is never lost — it is simply all output 1 second late.)

```sh:
$ cat file | delay 1s
```

Using the [typeliner(1)](typeliner.man.en.md) command to output each keystroke from the keyboard one character at a time, display it in real time while also outputting it again 3 seconds later, turning it into an echo.

```sh:
$ typeliner -t "" | (tee /dev/stderr | delay 3s) 2>&1
yo-ho!⏎        ← Type "yo-ho!" and press [Enter] within 3 seconds.
yo-ho!          ← 3 seconds later, "yo-ho!" is displayed again, at the same typing timing.
$ 
```

Furthermore, double the number of echoes.

```sh:
$ typeliner -t '' | (( tee /dev/stderr | delay 3 ) 2>&1 | tee /dev/stderr | delay 6) 2>&1
yo-ho!⏎        ← Type "yo-ho!" and press [Enter] within 3 seconds.
yo-ho!          ← 3 seconds later, "yo-ho!" is displayed again, at the same typing timing.
yo-ho!          ← 6 seconds later, "yo-ho!" is displayed again, at the same typing timing.
yo-ho!          ← 9 seconds later, "yo-ho!" is displayed again, at the same typing timing.
$ 
```

Suppose you receive data arriving via MQTT with the [mosquitto_sub(1)](https://mosquitto.org/man/mosquitto_sub-1.html) command and want to pipe it to "COMMAND1" for processing. However, data occasionally arrives faster than COMMAND1 can keep up with, risking data loss if it were fed straight through. To guard against this, insert delay in between: allocate enough buffer to hold up to 1 mebibyte of input data, while outputting with no delay at all. This way, up to 1 mebibyte of MQTT data is protected from being dropped. If the buffer overflows even so, the command exits with an error and processing is aborted. (That said, if you're using it with the delay permanently fixed at 0, you should use [surgetk(1)](surgetk.man.en.md) instead.)

```sh:
$ mosquitto_sub -t MY_TOPIC -h BROKER | delay -d 1MiB@0 | COMMAND1
```

Allow the delay time and buffer size to be changed while the command is running, via the named pipe `ctrl`.

```sh:
# Operations on terminal 1
$ mkfifo ctrl
$ cat file | delay ctrl > result

# Operations on terminal 2
$ echo '1MiB@2s' > ctrl  ⏎  ← Set the buffer to 1MiB and the delay to 2 seconds
$ echo '500ms' > ctrl    ⏎  ← Change only the delay time to 500 milliseconds (buffer size unchanged)
```

## Bugs

*time* accepts a delay expressed down to nanosecond precision, but there is no guarantee that a delay of that precision can actually be produced. How precisely the delay can be realized depends on the state of the OS and the performance of the hardware. (For a related discussion, see also the [report on the transfer-rate precision of the valve command](valve_precision.info.en.md).)

## Conformance to Standards

The source code of this command conforms to C99 and IEEE Std 1003.1-2008 ("POSIX.1").

## See Also

[surgetk(1)](surgetk.man.en.md) - A lighter-weight variant dedicated to burst absorption, equivalent to this command with its *time* parameter permanently fixed at 0.
