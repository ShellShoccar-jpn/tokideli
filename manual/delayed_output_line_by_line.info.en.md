# How to Delay Output Line by Line

（日本語版は[こちら](delayed_output_line_by_line.info.ja.md)）

The [delay(1)](delay.man.en.md) command records a timestamp for every single byte that arrives and delays its output on a per-byte basis. For text data delimited by newlines, however, there are cases where you want to delay the output "line by line" rather than byte by byte. This document explains how to achieve that, and what to watch out for along the way.

## Note: strict "line-by-line" delay is not actually possible

Delaying output "line by line" means that, at the moment of output, an entire line's worth of text data is emitted all at once. On the input side, however, a line's worth of data does not necessarily arrive all at once, and a line cannot be output at all until it has been received in full.

So, for example, if a data source takes 5 seconds to send a single line from its first character to its last, it is impossible to delay that line's output by only 1 second measured from the line's first character arriving (1 second in, the line hasn't even half arrived yet, so there's nothing to output).

The discussion that follows therefore assumes that **the time it takes for each line's text data to arrive, from first character to last, is sufficiently shorter than the delay you want to achieve**.

### Dealing with slowly-arriving data

If you want to delay data based on the arrival time of each line's end, but the text data source itself arrives slowly, pass it through the harmless grep command (and, if available, the stdbuf(1) command) first. This adjusts the timing so that each whole line is emitted all at once, right when that line's end arrives, before feeding it into the rest of the pipeline.

* If the stdbuf(1) command is available:
  ```sh:
  $ cat SLOW_TEXTDATA_SOURCE | stdbuf -o L grep ""
  ```
* If the stdbuf(1) command is not available, you can use the [ptw(1)](ptw.man.en.md) command instead.
  ```sh:
  $ cat SLOW_TEXTDATA_SOURCE | ptw grep ""
  ```

## Delaying output line by line with delay(1)

As long as each line's text data arrives fast enough, you can achieve line-by-line delayed output simply by feeding the text data into the [delay(1)](delay.man.en.md) command. For example, to delay each line's output by 3 seconds, write:

```sh:
$ cat TEXTDATA_SOURCE | delay 3s
```

If the data arrives fast enough that a large buffer is needed to hold it while it waits out the delay, allocate a larger buffer. For example, to allocate a buffer that can hold 10MiB of incoming data, write:

```sh:
$ cat TEXTDATA_SOURCE | delay 10MiB@3s
```

### If you want to keep memory usage down

Keep in mind, though, that [delay(1)](delay.man.en.md) is fundamentally designed around byte-level delay, and allocates room for a timestamp alongside every single byte. Because that timestamp has nanosecond precision, on a 64-bit environment holding one byte actually costs 16 bytes of real memory (see [delay(1)'s description of the *size* argument](delay.man.en.md#size) for details). So allocating a large buffer this way can consume a correspondingly large amount of memory.

If you want to avoid that, you can instead combine the [surgetk(1)](surgetk.man.en.md), [linets(1)](linets.man.en.md), and [tscat(1)](tscat.man.en.md) commands with the awk(1) command, as follows.

```sh:
$ cat TEXTDATA_SOURCE |
  linets -6e          |
  surgetk 1MiB        |
  awk -v dt=3.0 '
    BEGIN {
      OFMT="%.6f"; OFS="";
      while (getline l) {
        i=index(l," ");
        print substr(l,1,i-1)+dt, substr(l,i);
        fflush();
      }
    }'                |
  tscat -e
```

Here's a brief explanation of how this works. First, [linets(1)](linets.man.en.md) promptly prepends a timestamp to each incoming line. The key point is to use the UNIX-time format (`-e`) for this timestamp, so that it can later be added to and subtracted from using plain arithmetic. Right after that, [surgetk(1)](surgetk.man.en.md) allocates a buffer. As long as this buffer is large enough, even if the downstream commands' processing temporarily falls behind, that won't propagate back and cause [linets(1)](linets.man.en.md) to block on its own input. The following awk(1) command then adds 3 seconds to the timestamp at the front of each line, and once that is handed to [tscat(1)](tscat.man.en.md), each line is output at that adjusted time.

## Deciding on a buffer size

To decide on a reasonable buffer size (the *size* argument) for the [delay(1)](delay.man.en.md)/[surgetk(1)](surgetk.man.en.md) commands, define the following parameters.

* *sl* — the average number of bytes per line in the incoming text data
* *nl* — the number of lines arriving per second
* *dt* — the number of seconds you want to delay by

Given these, the number of bytes *sb* that must be held for the delayed output (i.e. the minimum value to give as the *size* parameter of [delay(1)](delay.man.en.md)/[surgetk(1)](surgetk.man.en.md)) can be computed as:

```
sb = sl * nl * dt
```

So, for example, if each line is 81 bytes on average, 50 lines arrive per second on average, and you want to delay them by 3 seconds, then *sb* = 12150 (about 12KiB).

Alternatively, if you know the incoming data's transfer rate is *r* bps, *sb* can be computed as:

```
sb = ( r / 8 ) * dt
```

So, for example, if you want to delay data arriving over a 57600bps link by 5 seconds, *sb* = 36000 (36kB).

A couple of notes:

* These are theoretical estimates. Real-world fluctuations in the transfer rate can push the actual requirement above them, so it's safer to allocate a somewhat larger buffer than the computed value.
* When you give this value to the [delay(1)](delay.man.en.md) command, it internally tries to allocate roughly 16 times as much memory (on a 64-bit environment), so pay attention to how much memory the host actually has installed. (As noted above, this multiplier does not apply to the [surgetk(1)](surgetk.man.en.md) command.)

## Summary

As long as each line arrives fast enough relative to the delay you want, delaying text data line by line is easy to achieve with the [delay(1)](delay.man.en.md) command alone. If you need a large buffer and are concerned about the resulting memory usage, combining [linets(1)](linets.man.en.md), [surgetk(1)](surgetk.man.en.md), and [tscat(1)](tscat.man.en.md) with awk(1) achieves the same effect while avoiding the cost of recording a timestamp for every single byte. Which approach to use depends on the data volume you expect and the memory available on your host.
