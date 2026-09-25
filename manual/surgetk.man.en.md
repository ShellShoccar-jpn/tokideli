# SURGETK(1)

（日本語版は[こちら](surgetk.man.ja.md)）

## Name

surgetk - Surge tank: absorb a temporary burst arriving at the standard input into a buffer, and send it out to the standard output smoothly

## Synopsis

```sh:
surgetk [-s|-o|-d] [-v] size
surgetk -i
```

## Description

Each byte that arrives from the standard input is first held in a ring buffer (whose capacity is given by *size*), and then sent to the standard output. Even when data arrives at a temporarily high rate on the standard input, this command absorbs it by accumulating it in the ring buffer, and can smoothly send it out at whatever pace the standard output side can handle. The name comes from a "surge tank," a piece of equipment installed in piping systems to absorb pulsations from pumps and the like.

This command corresponds to the behavior of the [delay(1)](delay.man.en.md) command with its *time* parameter permanently fixed at 0. It never intentionally delays an arrived byte; it sends each byte out as soon as the standard output side is ready to receive it. For this reason, there is no parameter corresponding to *time*.

What happens when the ring buffer becomes full — because it cannot hold any more bytes that have arrived but have not yet been sent out — can be chosen via the [options](#options) (`-s`/`-o`/`-d`).

When the standard input reaches EOF, this command does not terminate immediately. It first sends out every byte still remaining, unsent, in the buffer, and only then terminates.

## Arguments

### size

The parameter that specifies the capacity, in bytes, of the ring buffer that holds bytes which have arrived but have not yet been sent. It cannot be omitted.

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

If you specify a size that amounts to 80% or more of the total physical memory installed on the machine this command is running on, this is treated as an error. (Unlike [delay(1)](delay.man.en.md), this command's ring buffer actually consumes exactly 1 byte of memory for every 1 byte it holds, so the *size* value is compared directly against the installed memory, with no correction factor.)

#### What it means to make size small

When you insert this command into a pipeline as in `cmd0 | surgetk size | cmd1`, the data passing through actually crosses several buffering stages laid out in series, not just *size*.

1. The kernel pipe from cmd0 to surgetk (owned by the OS)
2. surgetk's stdio buffer on the standard-input side (owned by libc)
3. surgetk's own ring buffer (the part you specify with *size*)
4. The kernel pipe from surgetk to cmd1 (owned by the OS)

Stages 1, 2, and 4 are "free" buffering that the OS and the C standard library already provide from the outset, regardless of the value of *size*. Moreover, connecting `cmd0 | cmd1` directly creates only a single pipe, whereas inserting this command in between creates a second, independent kernel pipe (stage 4 above); so merely inserting this command already adds some amount of burst tolerance, no matter what value you choose for *size*.

Consequently, if you set *size* to a value well below the sum of stages 1, 2, and 4 (as a rough guide, on the order of a few tens of KiB or less), the effect of *size* gets buried inside this "free" buffering and is barely noticeable in practice. *size* starts to matter once you specify a value that clearly exceeds the sum of stages 1, 2, and 4. Kernel pipe capacity has a ceiling (such as `pipe-max-size`), and raising it can require privileges or system configuration changes, whereas *size* can, in effect, be made arbitrarily large using nothing but ordinary user privileges (aside from the 80%-of-physical-memory ceiling) — that is precisely this command's reason for existing.

For reference, here are the values measured on Linux (the environment this command was developed and verified on).

| Buffering stage | Approximate capacity |
|---|---|
| Default capacity of one kernel pipe | 65536 bytes (64KiB) |
| Ceiling on kernel pipe capacity (`pipe-max-size`) | Environment-dependent (1048576 bytes = 1MiB on the verification machine) |
| stdio's standard-input buffer (based on `st_blksize`) | 4096 bytes (4KiB) |

These figures depend on the OS's and kernel's configuration and are not guaranteed by the POSIX standard. Other UNIX-like OSes may differ (BSD-family systems and macOS in particular tend to use smaller values).

Here are simple ways to check the values on your own environment.

* Use this command's own `-i` option ([described below](#-i)). This works even in environments with no Python or the like installed, as long as you can build this command itself.
  ```sh:
  $ surgetk -i
  ```
* Linux only — check the actual capacity of a kernel pipe (on environments where Python 3.10 or later's `fcntl` module supports `F_GETPIPE_SZ`):
  ```sh:
  $ python3 -c "import os,fcntl; r,w=os.pipe(); print(fcntl.fcntl(w, fcntl.F_GETPIPE_SZ))"
  ```
* Linux only — check the ceiling on kernel pipe capacity:
  ```sh:
  $ cat /proc/sys/fs/pipe-max-size
  ```
* Common to most POSIX-conformant OSes — check `PIPE_BUF` (the size up to which a write is guaranteed to be atomic; note that this is a more limited value, less than or equal to a kernel pipe's actual total capacity, not the capacity itself):
  ```sh:
  $ getconf PIPE_BUF /
  ```

## Options

### -s, -o, -d

Specifies what to do when a new byte arrives while the ring buffer is full. These three options are mutually exclusive; if more than one is given, the last one takes effect.

* `-s` ... (Default) Suspend. Block (pause) reading from the standard input until a slot in the buffer becomes free.
* `-o` ... Overwrite. Discard the oldest unsent byte and store the new byte in its place.
* `-d` ... Disconnect. Discard whatever unsent bytes are currently sitting in the buffer at that moment, and terminate this command immediately.

  Note, however, that because this command sends each arrived byte out as soon as possible (since *time* is always 0), some bytes may already have been sent to the standard output before `-d` is triggered. What `-d` guarantees is that "whichever bytes were still sitting in the buffer, not yet sent, at the moment it was triggered, are discarded" — not that "this command never outputs anything at all." Keep this distinction in mind.

### -v

Raises the verbosity level by one each time it is given. `-v` sets level 1, `-vv` sets level 2, and the higher the level, the more detail about internal behavior is reported to the standard error output.

At level 1, at minimum, every time the buffer becomes full (overflows), this is reported together with the action taken this time (suspend, overwrite, or disconnect).

### -i

Investigation mode. When given, this command doesn't perform the surge-tank operation at all; instead, it investigates the sizes of the various OS/libc buffers that a byte passes through around this command (see [above](#what-it-means-to-make-size-small)), prints them to the standard output, and exits immediately. All other options (`-s`/`-o`/`-d`/`-v`) and the *size* argument are ignored (you don't even need to specify *size*).

The output format is one buffer per line: the name in the 1st column and its size in bytes in the 2nd column, separated by a space. The 2nd column (the size) is right-aligned. The name is one of the following (depending on the environment, a line for an item that could not be determined may simply be omitted; that is not an error).

* `pipe_default_size` ... the default capacity of one kernel pipe (Linux only)
* `pipe_max_size` ... the ceiling on kernel pipe capacity (Linux only)
* `stdio_buf_size` ... the size stdio uses as the standard-input buffer for a pipe
* `pipe_buf` ... `PIPE_BUF` (the size up to which a write is guaranteed to be atomic)

### -h

Displays the usage, the last-updated date, the license, and so on, to the standard error output.

## Return Value

Returns 0 when the standard input reaches EOF and all of the buffer's contents have been sent, terminating normally. Returns 1 when this command was forced to terminate because the buffer was full, due to the `-d` option. Returns a value other than 0 for any other abnormal termination (such as an argument error).

## Examples

Absorb the output of a command that instantaneously produces a large amount of data (e.g. `yes`) with a 64-kibibyte surge tank, and forward it as is to the next command ("COMMAND1"). (Even if COMMAND1 temporarily falls behind, up to 64KiB of data is never lost.)

```sh:
$ yes | surgetk 64KiB | COMMAND1
```

With a buffer of only 4 bytes, while data arrives rapidly, discard the oldest data (`-o`) and report each discard (`-v`).

```sh:
$ yes | surgetk -o -v 4B | head -c 20
```

Suppose you receive data arriving via MQTT with the [mosquitto_sub(1)](https://mosquitto.org/man/mosquitto_sub-1.html) command and want to pipe it to "COMMAND1" for processing. However, data occasionally arrives faster than COMMAND1 can keep up with, risking data loss if it were fed straight through. To guard against this, insert surgetk in between, allocating a buffer that can hold up to 1 mebibyte of input data. This way, up to 1 mebibyte of MQTT data is protected from being dropped. If the buffer overflows even so, the command exits with an error and processing is aborted.

```sh:
$ mosquitto_sub -t MY_TOPIC -h BROKER | surgetk -d 1MiB | COMMAND1
```

When using [linets(1)](linets.man.en.md) to record the arrival time of text data, if the downstream command ("COMMAND1") is too slow to keep up, that slowness will eventually propagate back to linets and make it block on its own input, so the arrival time can no longer be recorded accurately. To prevent this, put a sizable buffer (100MiB in this example) right after linets, keeping it from ever blocking on its input.

```sh:
$ cat FAST_DATA_SOURCE_VIA_NAMED_PIPE | linets -3 | surgetk -d 100MiB | COMMAND1
```

Check the sizes of the OS/libc buffers around this command.

```sh:
$ surgetk -i
pipe_default_size   65536
pipe_max_size     1048576
stdio_buf_size       4096
pipe_buf             4096
```

## Conformance to Standards

The source code of this command conforms to C99 and IEEE Std 1003.1-2008 ("POSIX.1").

## See Also

[delay(1)](delay.man.en.md)
