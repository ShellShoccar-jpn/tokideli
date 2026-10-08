# Comparing tokideli's delay Command with an Earlier delay Command (rom1v/delay)

（日本語版は[こちら](delay_commands_comparision.info.ja.md)）

It turns out a command also named `delay`, sharing a similar concept, already existed before tokideli's: [rom1v/delay](https://github.com/rom1v/delay). Both aim to do the same thing — "delay data arriving on standard input by a fixed amount of time before sending it to standard output" — but comparing the two implementations revealed some fairly different design philosophies, which this document records.

## About the Two Projects

| | [rom1v/delay](https://github.com/rom1v/delay) | tokideli's [delay](delay.man.en.md) |
|---|---|---|
| Author | Romain Vimont (rom1v) | Shell-Shoccar Japan |
| First commit | January 2014 | 2026 |
| Last updated | May 2019 (no activity since, as of 2026) | Under active development as of this writing |
| License | MIT | CC0 (public domain) |
| GitHub scale | 35 stars / 10 forks / 2 open issues | (scale of the tokideli repository as a whole) |
| Source layout | 5 files: `main.c`/`dtbuf.c`/`dtbuf.h`/`time_ms.c`/`time_ms.h` | Single self-contained `delay.c` |

rom1v/delay was created in 2014 and, as far as GitHub activity shows, has not been actively developed since its last update in May 2019. Its star/fork counts also suggest a small personal utility rather than a widely-adopted project.

## The Different Meanings of "Buffer Size"

This was the question that started this whole investigation. The short answer: **the two commands' "buffer size" mean different things.**

### rom1v/delay: the byte count of a timestamp-inclusive "chunk"

Reading rom1v/delay's `dtbuf.c`, the ring buffer is a plain `char *data` byte array, into which data is written in units called "chunks." Each chunk looks like this (from `dtbuf.c`):

```c
struct header {
    time_ms timestamp;      /* int64_t */
    chunk_length data_length; /* uint16_t */
};
#define DTBUF_CHUNK_PAYLOAD_SIZE 4000
#define DTBUF_CHUNK_SIZE (sizeof (struct header) + DTBUF_CHUNK_PAYLOAD_SIZE)
```

That is, every chunk write puts a timestamp header (an `int64_t` plus a `uint16_t`, typically padded to around 16 bytes under normal alignment) **immediately before** its 4000-byte payload. The `capacity` bytes given via `-b` is used entirely as storage for these header+payload pairs (`dtbuf_init()` actually allocates `capacity + DTBUF_CHUNK_SIZE - 1`, but that extra margin exists only so a chunk is never split across the ring buffer's wraparound boundary — it is not a standing allocation set aside for timestamps).

So the data itself and its timestamp **share** the byte budget specified via `-b`. That said, the actual per-chunk overhead — roughly 16 bytes of header for 4000 bytes of payload — is only about 0.4%, practically negligible. The README's description, "dtbufsize is the buffer size storing the data," is technically a slight simplification; more precisely, it is the total number of bytes needed to store the timestamped data.

### tokideli's delay: timestamps live outside the requested size

tokideli's `delay.c`, by contrast, implements its ring buffer as an array of `ringelem_t` structs, each holding "one byte of data plus its arrival time (nanosecond-precision)":

```c
typedef struct _ringelem_t {
  unsigned char ucData;      /* the byte received */
  uint32_t      u4Nsec;      /* fractional-second part of the arrival
                                 time (nanoseconds, 0-999,999,999)     */
  time_t        i8Sec;       /* integer-second part of the arrival time */
} ringelem_t;
```

Rather than keeping the arrival time as a `struct timespec`, it is split into a `uint32_t` (the nanosecond fraction only ever needs 0-999,999,999, which fits comfortably in 32 bits) and a `time_t`, with the fields ordered from smallest to largest alignment requirement (1 byte, then 4 bytes, then 8 bytes). This lets the struct pack with minimal padding.

The `size` argument (the `100MiB` part of, e.g., `100MiB@1s`) represents the **element count** of this struct array — i.e., the pure number of data bytes. The actual memory allocated is `sizeof(ringelem_t) * size`, which comes to 16 bytes per element on a 64-bit system (versus the 24 bytes it would be if a plain `struct timespec` were used instead, a 33% reduction from the layout trick above). In other words, roughly 15 times as much memory again is allocated **outside** the requested `size`, purely to hold the per-byte timestamps.

### Why the difference: granularity of recording

This isn't just an implementation quirk — it reflects a difference in the granularity of the problem each command actually solves.

- rom1v/delay reads and writes data in chunks of up to 4000 bytes at a time, as much as a single `poll()` call happens to detect. All bytes within one chunk share a single (millisecond-precision) timestamp, so the cost of recording a timestamp is paid once per chunk.
- tokideli's delay records a separate nanosecond-precision arrival time for every single byte, so it can compute the delay precisely on a byte-by-byte basis. Achieving that precision requires attaching a timestamp to every byte individually, which is exactly why the memory overhead is so much larger.

In short, rom1v/delay trades away timestamp precision and granularity (chunk-level, millisecond-level, up to 4000 bytes at a time) for a much smaller memory footprint, while tokideli's delay spends far more memory to get byte-level, nanosecond-level precision. This trade-off is also why tokideli's `delay.c` rejects any size request exceeding 80% of the machine's physical RAM: since every requested byte actually costs roughly 16 real bytes, the gap between the requested size and the real memory footprint is large enough that an careless size argument could exhaust the machine's memory.

## Difference in Overflow Handling

When its buffer fills up, rom1v/delay always blocks further reads from standard input (implemented by clearing its `poll_stdin` flag). This matches tokideli's `delay.c` `-s` (Suspend) mode, which is also its default.

tokideli's `delay.c` additionally offers `-o` (Overwrite: discard the oldest data to make room for new) and `-d` (Disconnect: discard everything buffered and exit immediately the moment it overflows), plus a `-v` option that reports overflow events to standard error. rom1v/delay offers neither of these alternatives nor any visibility into overflow events.

## Changing Parameters at Runtime

rom1v/delay has no way to change the delay time or buffer size it was started with — doing so requires restarting the process.

tokideli's `delay.c` supports a `controlfile` mechanism that allows changing the delay time and/or buffer size while the command is running: for a regular file, the new value is picked up by polling every 0.1 seconds (or immediately if you send `SIGHUP`); for a character-special file or named pipe, it takes effect immediately upon being written.

## Portability and Standards Conformance

rom1v/delay's `Makefile` only sets `CFLAGS += -Wall -g -O3`, with no OS-specific conditionals visible anywhere. That doesn't necessarily mean it fails to build elsewhere, but at least the source shows no explicit accommodation for multiple operating systems (this has not been independently verified here).

tokideli's `delay.c` (along with all 16 other commands in the suite) has been built and exercised on seven real environments — Linux, Solaris 11.3, FreeBSD 12.1, NetBSD 7.0.2, OpenBSD 6.5, macOS 15.8, and Linux-based Android — with explicit handling for each OS's `feature_tests.h` quirks (e.g. Solaris's need for `__EXTENSIONS__`, FreeBSD's need for `_XOPEN_SOURCE`).

Separately, rom1v/delay's GitHub repository has an open, unresolved issue titled ["Undefined behavior"](https://github.com/rom1v/delay/issues/5), pointing out that `dtbuf.c` casts a pointer to the header struct onto an arbitrary position within a byte array with no guaranteed alignment — strictly undefined behavior, of the same kind described in [this well-known write-up on x86 data-alignment bugs](http://pzemtsov.github.io/2016/11/06/bug-story-alignment-on-x86.html). As of 2026 this remains unfixed.

## Summary

rom1v/delay is a small, self-contained tool built for one specific purpose: introducing a fixed delay. It is memory-efficient and easy to reason about, but offers no operational flexibility such as changing settings at runtime or choosing how to handle overflow, and its timestamps are limited to chunk-level, millisecond precision.

tokideli's delay command offers byte-level, nanosecond-precision delay calculation, multiple overflow-handling modes, runtime parameter changes, and support across a wide range of operating systems — but at the cost of using considerably more memory than the size actually requested. Neither is simply "better"; **which one fits depends on the use case** — a lightweight one-off delay, versus one that needs precision and operational flexibility.
