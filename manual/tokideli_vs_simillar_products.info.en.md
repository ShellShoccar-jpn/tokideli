# Comparing tokideli's Commands with Similar Existing Tools — Why We Reinvented the Wheel

（日本語版は[こちら](tokideli_vs_simillar_products.info.ja.md)）

Some of tokideli's commands have functionality similar to well-known existing commands. Because of that, we sometimes hear the complaint: "If that's what you want to do, just use the existing command 'X'. Why reinvent the wheel and multiply commands needlessly?" But the reason we built our own commands is that we had complaints about the existing ones — about their quality, their features, or outright defects we found in them. This document compares each tokideli command that has actually drawn that "reinvented wheel" criticism against the existing command with similar functionality, and lays out, command by command, why reinventing it was worthwhile.

## `valve` vs [pv](https://www.ivarch.com/programs/pv.shtml)'s `-L` Option

If you want to limit a data transfer to a constant rate, the claim goes, there's no need to build a command like [valve(1)](valve.man.en.md) — just use the well-known [pv](https://www.ivarch.com/programs/pv.shtml) command's `-L` option.

But as far as we're concerned, it comes down to one line: `pv -L`'s precision is simply too coarse and loose. Looking at pv 1.11.0's source code (`src/pv/loop.c`, `src/pv/transfer.c`), `-L` turns out to be a discrete token-bucket scheme that only replenishes its sendable quota once every 100 milliseconds, and even detecting that quota depends on `select()` polling with a fixed 90-millisecond timeout. These two coarse periods, which never synchronize with each other, beat against one another and produce jitter.

`valve`, on the other hand, uses a simple scheme: each time it sends a chunk, it checks the clock and computes how long it needs to sleep until the next send so that every interval comes out the same, then calls `nanosleep()` for that long. This produces no jitter. On top of that, error from the varying execution time of the surrounding processing never accumulates either.

Measuring the two side by side, in a 1-line-per-second test the standard deviation of the interval came out to 0.288ms for `valve` versus 3.573ms for `pv -L` — nearly an order of magnitude apart. The full design comparison and measurement data are written up in the [Report on the Transfer-Rate Precision of the valve Command](valve_precision.info.en.md).

### Verdict

`pv`'s primary purpose appears to be visualizing the flow of data through a pipe — progress monitoring. Rate limiting via `-L` seems to be positioned as an add-on feature bolted onto that primary purpose. If so, it's understandable that precision wasn't a priority.

## `delay` vs [rom1v/delay](https://github.com/rom1v/delay)

We've been asked what could possibly be dissatisfying about tokideli's [delay(1)](delay.man.en.md), given that [rom1v's delay](https://github.com/rom1v/delay) already exists with almost the same functionality and even the same name. Both certainly share the same goal — delay data arriving on standard input by a fixed amount of time before sending it to standard output — but we found it a little lacking in the three respects described below.

* **Performance/design**: rom1v's `delay` stores data in a ring buffer in units of timestamped "chunks" (up to 4000 bytes each), so its timestamps are limited to millisecond precision at chunk granularity. tokideli's `delay`, by contrast, records an individual nanosecond-precision timestamp for every single byte. The price of that precision, however, is a downside: tokideli's `delay` ends up using roughly 16 times as much memory as the size of the data it can actually hold in its buffer.
* **Features**: rom1v's `delay` always blocks further reads once its buffer fills, whereas tokideli's `delay` offers several overflow-handling modes — `-o` (overwrite) and `-d` (disconnect) — and lets you change the delay time and buffer size at runtime via its `controlfile` mechanism. rom1v's `delay` has neither option.
* **Portability**: rom1v's `delay` is a small, single-purpose tool spread across 5 source files, with active development having stopped since May 2019. An unresolved GitHub issue notes undefined behavior in a spot where a pointer to a header struct is cast onto a byte array at an alignment-unguaranteed position. tokideli's `delay.c` is a single, self-contained file, verified to build and run on 7 environments: Linux, Solaris, FreeBSD, NetBSD, OpenBSD, macOS, and Linux-based Android.

These details are written up in full in [Comparing tokideli's delay Command with an Earlier delay Command (rom1v/delay)](delay_commands_comparision.info.en.md); see that for more.

### Verdict

rom1v's `delay` is memory-efficient and easy to understand, but lacks operational flexibility and fine-grained timestamp precision. tokideli's `delay` gains byte-level, nanosecond precision and runtime flexibility, at the cost of higher memory usage. Which one is the better fit depends on what you need — whether you want to conserve memory, want high precision, or care about features and portability. And what we wanted was high precision and portability.

## `linets`/`charts` vs `ts` ([moreutils](https://github.com/pgdr/moreutils))

If you want to attach a timestamp to each line based on its arrival time, the claim goes, there's no need for a command like [linets(1)](linets.man.en.md) — [moreutils](https://github.com/pgdr/moreutils)' `ts` command already does that. But we had a few complaints about `ts` as well.

For one thing, `ts` is implemented in Perl. Perhaps because some consider Perl an outdated language and are reluctant to depend on it, an [unofficial, Go-based reimplementation of moreutils](https://github.com/sweetbbak/go-moreutils) has even appeared. And indeed, its slowness is hard to deny. Running the same job (attaching a timestamp to 100,000 lines of text data) through both commands showed `ts` taking roughly twice as long as `linets`.

```sh:
$ time yes | head -n 100000 | ts     %.s > /dev/null
$ time yes | head -n 100000 | linets -6e > /dev/null
```

(Measured in this document's test environment at the time of writing; average of 3 runs.)

|      | `ts %.s` | `linets -6e` |
|------|---------:|-------------:|
| real |    0.88s |        0.46s |
| user |    0.28s |        0.09s |
| sys  |    0.59s |        0.37s |

Wall-clock time (real) shows roughly a 1.9x gap, and user CPU time alone shows roughly a 3x gap — a good illustration of how much per-line overhead the Perl interpreter is paying.

On the feature side, the differences are:

* On the point of output-format flexibility, there's no real complaint about `ts`. It can be configured to produce almost every format [linets(1)](linets.man.en.md)/[charts(1)](charts.man.en.md) can output — calendar date-time, UNIX time, extended ISO 8601, and so on. The one thing `ts` can't do is nanosecond-level output, though given the precision typical PCs and OSes can actually deliver today, that's a reasonable limitation.
* `ts`'s `-s` (elapsed time since the program started) corresponds to `linets`'s `-z`. However, `ts` has no equivalent to `linets`'s `-Z` (elapsed time since the first line of the text data arrived).
* `linets` timestamps the moment the **first character** of a line arrives, while `ts` timestamps the moment the line's end (newline or EOF) arrives.
* Choosing `ts`'s `-i` (elapsed time since the previous line's timestamp) makes the output show only the delta, with no way to get the absolute timestamp and the delta at the same time. `linets`'s `-d` option, by contrast, outputs both at once — the absolute timestamp in column 1 and the elapsed time since the previous line in column 2.
* `ts` offers a `-m` option to use the monotonic clock, but even with `-m` specified, the output is still shown converted back into real-time (calendar time or UNIX time) form. In other words, it just adds the monotonically-measured elapsed time onto a real-time baseline captured at startup — if the system clock gets corrected while it's running, the displayed absolute time won't reflect that correction. So when combined with an absolute-time display format (calendar date-time, UNIX time, extended ISO 8601), choosing `-m` buys little. `linets`, by contrast, automatically picks the REALTIME clock for its absolute-time formats (`-c`/`-e`/`-I`) and the MONOTONIC clock for its elapsed-time formats (`-z`/`-Z`), based on what each option actually means.
* `ts` can only attach timestamps at the line level; there's no standalone way to record at the character or byte level. If you really needed that, you'd have to pre-process the text into one-character-per-line form first, but doing so takes time of its own, which defeats the whole purpose of accurately recording arrival timing. `charts` records character-level timestamps directly, without needing any such pre-processing step.

### Verdict

`ts` is implemented in Perl, a language some consider somewhat dated and are reluctant to depend on, and it's also slower in practice. `linets`/`charts`, written in POSIX C, have a lower barrier to adoption and are faster. `ts` is also somewhat more limited on features compared to `linets`/`charts` — it can't output an absolute timestamp and a delta together, and above all, it can't record at the character level. `ts`'s `strftime`-based format string genuinely gives it some flexibility, but format conversion itself can just as well be handed off to a separate text-processing command like `sed`.

## `charts`/`linets`/`tscat` vs [asciinema](https://asciinema.org/)

[asciinema](https://asciinema.org/) is a wonderful tool. It can record and replay what happens on a terminal screen, and since its data is text-based (JSON Lines, with a `.cast` extension), it's also lightweight.

That said, `asciinema`'s `.cast` format is designed and built with the assumption that it will only ever be consumed by `asciinema` itself. It is text, but each line is a JSON array of the form `[time, type, data]`, with the `data` field escaped as a JSON string. So while a human can read it as text, processing it directly with general-purpose text tools like `grep` or `sed` is difficult — a JSON parser has to come first.

What we wanted was something more general-purpose — a tool for controlling the flow of the data itself as it passes through standard input/output. The timestamped text that [charts(1)](charts.man.en.md), [linets(1)](linets.man.en.md), and [tscat(1)](tscat.man.en.md) handle has no JSON-array structure wrapped around it; in exchange, it can be fed straight into ordinary UNIX text tools — `grep`, `sed`, `awk`, and the like.

That said, the `.cast` format and the format tokideli's commands produce are, in principle, mutually convertible (aside from the metadata recorded at the start, such as screen size). In fact, the single-line structure that `charts`/`linets`'s `-j` option produces — "timestamp + JSON-escaped payload" — corresponds almost one-to-one with each element of `.cast`'s `[time, type, data]` JSON array. We'd like to provide a conversion tool between the two formats at some point in the future.

### Verdict

`asciinema` and these tokideli commands aren't in a relationship where one is "better" and should be chosen over the other. `asciinema` specializes in recording and replaying terminal-screen interaction, while `charts`/`linets`/`tscat` pursue the more general goal of recording and replaying the flow of standard input/output itself. If anything, the two should become mutually convertible and build a friendly relationship, rather than compete.

## `ptw` vs `unbuffer` ([expect](https://sourceforge.net/projects/expect/))

[ptw(1)](ptw.man.en.md) was built as a command that (likely) still works even in cases where [stdbuf(1)](https://www.gnu.org/software/coreutils/manual/html_node/stdbuf-invocation.html), from GNU coreutils, fails to switch full buffering over to line buffering. But the claim is that `unbuffer`, which ships with [expect(1)](https://sourceforge.net/projects/expect/), can already do that. As shown below, though, `unbuffer` is simply too poorly made to take seriously.

### Startup and processing overhead

For one thing, `unbuffer` is implemented as a Tcl script (`expect_unbuffer`), so it pays a large startup cost. Measuring the time from start to finish while processing just a single line of data with the one-liners below showed `unbuffer` taking roughly 5 to 10 times as long as `ptw` (measured in this document's test environment at the time of writing; average of 3 runs).

```sh:
$ time echo x | unbuffer -p cat > /dev/null
$ time echo x | ptw cat > /dev/null
```

|      | `unbuffer -p cat` | `ptw cat` |
|------|------------------:|----------:|
| real |           ~0.02s |  ~0.003s |

### Bug 1: It exits leaving unprocessed data behind the moment the upstream command exits

Try relaying the 4000 lines the `seq` command produces through `tr` with the one-liners below (`tr 1 1` was picked as the relay here specifically because it leaves the data completely unchanged, so only the effect of the buffering-mode change is observed).

```sh:
$ seq 4000 | unbuffer -p tr 1 1 | cat > with_unbuffer.txt
$ seq 4000 | ptw         tr 1 1 | cat > with_ptw.txt
```

`ptw` outputs all 4000 lines generated by `seq` in about 9ms. `unbuffer`, by contrast, gives a different line count on every run, but in every case ends up with far fewer lines than 4000 (around 21 lines, in measurements taken for this document) — and the whole thing finishes in a very short time regardless.

The cause is how `unbuffer -p` is implemented (the `interact` call inside `expect_unbuffer`): the moment the upstream side of its standard input hits EOF, it waits only briefly for any remaining output from `tr` (`expect -timeout 1 -re .+`) and then cuts the whole thing short. Paired with an upstream command like `seq` that finishes producing its data almost instantly, the upstream hits EOF before `unbuffer`'s internal relay has processed most of the data, and whatever's left is simply never relayed — and lost.

### Bug 2: It can freeze or lose data to a race condition during large transfers

Artificially delaying the upstream's exit (e.g., tacking a `sleep` onto the end, as in `(seq 4000; sleep 5) | unbuffer -p tr 1 1 | cat`) works around Bug 1. But then you run into yet another defect.

Trying this workaround on `seq 4000` in the test environment used for this document, one run ended up outputting only 840 lines (while still exiting with a normal exit code), and a separate run froze completely and had to be force-killed.

Tracking this down with `strace` revealed a telltale pattern in `unbuffer`'s own (tclsh) system-call sequence (fd0 = standard input, fd3 = the pty connecting to the child process `tr`):

```text:
read(0, ...)
write(3, "...", 4096)        # relayed correctly up to here
write(3, "...", 1905)
read(0, ...)
write(3, "...", 4096)
write(3, "...", 1905)
read(0, ...)
write(3, "...", 2417)        # <- last write to the pty
read(0, <unfinished ...>)    # no more write(3, ...) calls after this
read(0, <unfinished ...>)
read(0, "...", 4096) = 4096  # data is read, but not written
read(0, <unfinished ...>)
# -> blocks here indefinitely
```

In other words, `interact`'s internal loop is implemented as "keep reading for as long as you can, and put off writing" — it can keep accumulating already-read data without ever writing it out to the pty side, and keep calling `read()` again instead. If, while stuck in this read-only streak, it happens to call `read(0, ...)` at the exact moment the upstream pipe is momentarily empty, that call blocks until more data shows up — stranding data that's already been read but never forwarded to `tr`.

This isn't something that happens deterministically once some fixed limit, like a kernel buffer size, is hit. Whether `read` or `write` gets called next is a **race condition** depending on the timing of data arriving on the upstream pipe and on OS scheduling — the more data there is (i.e., the more times `read` gets called), the higher the odds of unluckily landing on a blocking read.

This is why processing a large amount of data through `unbuffer` leads to data loss or freezes.

### Bug 3: It mixes standard output and standard error together

[A comment on Hacker News](https://news.ycombinator.com/item?id=42401011) pointed out that `unbuffer` mixes standard output and standard error together, and that turned out to be true.

Trying to send data from standard input to both standard output and standard error with `tee`, as below, makes both come out on standard output under `unbuffer`.

```sh:
$ echo foo | ptw         tee /dev/stderr | sed 's/.*/stdout: &/'
foo
stdout: foo
$ echo foo | unbuffer -p tee /dev/stderr | sed 's/.*/stdout: &/'
stdout: foo
stdout: foo
$ 
```

The cause is a difference in implementation. `unbuffer` (the Tcl/Expect `unbuffer` script) launches the target process internally via the `spawn` command, and Expect's `spawn` is specified to connect both the child's standard output and standard error to one and the same pseudo-terminal (PTY). This isn't really a bug specific to `unbuffer` so much as a constraint of Expect's `spawn` itself. `ptw`, by contrast, is designed to connect only standard output to the PTY slave and never touch standard error at all, so standard error stays on the file descriptor it inherited from the caller, never passing through the PTY, keeping a path completely independent from standard output.

### Verdict

`unbuffer` isn't even worth comparing against `ptw` to see which is "better." On top of the performance weakness of its high startup cost, it carries three fatal bugs: it cuts off unprocessed data the moment the upstream exits, it can lose data or freeze to a race condition during large transfers, and it mixes standard output and standard error together. It is not a tool that can be trusted.

## `ptw` vs `pipetty` ([colorized-logs](https://github.com/kilobyte/colorized-logs))

Explain the `unbuffer` findings to someone, and they'll often say, "Then just use `pipetty`, from [colorized-logs](https://github.com/kilobyte/colorized-logs)." But `pipetty` has a few problems — or at least things we find dissatisfying — of its own.

### It mixes standard output and standard error together

Looking at `pipetty`'s source (`pipetty.c`), it connects both standard output and standard error to the same PTY slave, just like this:

```c:pipetty.c
dup2(slave, 1);
dup2(slave, 2);
```

This is the same problem as `unbuffer`, covered above. Running the same `tee`-based experiment used for `unbuffer` shows `pipetty` mixing standard output and standard error together too.

```sh:
$ echo foo | ptw     tee /dev/stderr | sed 's/.*/stdout: &/'
foo
stdout: foo
$ echo foo | pipetty tee /dev/stderr | sed 's/.*/stdout: &/'
stdout: foo
stdout: foo
$ 
```

That said, since the two streams wouldn't mix unless you deliberately wired them to the same PTY, this may well be intentional, by design. Either way, though, this problem (or design choice) produces an unwelcome result for any user who doesn't want the mixing.

### It unconditionally allocates a PTY

`ptw` has a shortcut: when its own standard output is already connected to a terminal (e.g., it's sitting at the end of a pipeline, writing straight to a terminal), it skips allocating a PTY and simply `exec()`s the wrapped command directly (overridable with the `-f` option) — because when it's already connected to a real terminal, there's no need to fake one with a PTY in the first place. `pipetty` has no such check and always allocates a PTY. As a result, `pipetty` can end up paying an unnecessary startup cost.

### Throughput on large transfers

Comparing the two by relaying a large volume of data through `tr`, `ptw` came out slightly faster on Linux (in the author's test environment; 100 million lines, average of several runs).

```sh:
$ time yes | head -n 100000000 | ptw     tr 1 1 | cat > /dev/null
$ time yes | head -n 100000000 | pipetty tr 1 1 | cat > /dev/null
```

|      | `ptw`   | `pipetty` |
|------|--------:|----------:|
| real | ~1.34s  |   ~1.43s  |

This performance gap appears to be specific to Linux, incidentally — trying the same comparison on several other UNIX-like OSes (FreeBSD 12.1, Solaris 11.3, macOS 15.8) showed no significant speed difference.

Investigation found that this speed gap comes down to `ptw` and `pipetty` setting slightly different termios attributes on the PTY slave, and that difference sending execution down a different code path inside the Linux kernel. Specifically, that's `n_tty_write()`, in the Linux kernel source file `drivers/tty/n_tty.c`.

`ptw` leaves the termios `OPOST` attribute on, while `pipetty` turns it off, and `n_tty_write()` branches onto a different code path depending on whether `OPOST` is set. `process_output_block()`, used only when `OPOST` is on, checks the available buffer room (`tty_write_room()`) before writing; when `OPOST` is off, it calls `pty_write()` directly and unconditionally, with no such check. In this environment, the latter path produces more voluntary context switches, which means less CPU time (sys) but more overall wall-clock time (real). Sure enough, comparing a variant of `ptw` with `OPOST` turned off against real `pipetty` produced almost identical run times.

|                             | `ptw` original<br>(OPOST on) | `ptw` (OPOST off only) | real `pipetty` |
|-----------------------------|------------------------------:|------------------------:|---------------:|
| real (100M lines, avg of 3) | ~1.34s                        | ~1.47s                  | ~1.43s          |
| sys                         | ~2.06s                        | ~1.78s                  | ~1.72s          |
| voluntary context switches  | ~65,200                       | ~82,100                 | ~80,350         |

The cause is in Linux's `drivers/tty/n_tty.c`: `n_tty_write()` takes a genuinely different code path depending on `OPOST`, as described above. The same comparison was repeated on the other UNIX-like OSes `ptw` is already verified to build and run on — FreeBSD 12.1, Solaris 11.3, and macOS 15.8 — and none of them showed a significant speed difference tied to `OPOST` (FreeBSD and Solaris, if anything, leaned slightly toward OFF being faster; macOS showed essentially no difference, under 0.6%). This effect is specific to Linux's kernel implementation, not a general POSIX-level principle.

### Verdict

`pipetty` is implemented in C, and so is far lighter than `unbuffer`, which pays the cost of starting a Tcl interpreter on every invocation. Even so, `ptw` holds a clear edge: `pipetty` carries the same "mixes standard output and standard error together" problem as `unbuffer`, it always allocates a PTY unconditionally, and it comes out slightly behind `ptw` on throughput for large transfers.

## `ptw` vs `script`

`ptw` apparently has yet another rival, too — some people say, "Why not just use the `script` command?" But actually putting it to the test shows that `script`, too, is simply not suited at all for use as a substitute for `stdbuf`.

### Advantage: no extra installation needed

Let's start by giving `script` its due credit: it has one powerful advantage — it comes preinstalled on nearly every UNIX-like OS. On the point of adoption barrier, it has a clear edge over `unbuffer`, `pipetty`, and `ptw` alike.

### Issue 1: There are multiple `script` implementations with incompatible syntax and features

There are at least two different `script` commands — a BSD version and a util-linux version — and their command syntax differs. To wrap an arbitrary command, the BSD version has you list the wrapped command's name and arguments at the end of `script`'s own argument list, while the util-linux version requires passing it as the argument to a `-c` option. On top of that, the ability to control local echo exists in the util-linux version, as the `-E` option, but has no equivalent in the BSD version.

If you want to run the same `tee`-based experiment used for `unbuffer` and `pipetty`, here's how to write it for the util-linux version.

```sh
$ echo foo | script -E never -qec 'tee /dev/stderr' /dev/null | sed 's/.*/stdout: &/'
stdout: foo
stdout: foo  # <- this also mixes stdout and stderr together (more on this below)
$ 
```

The BSD version, by contrast, might be written like this. To suppress local echo, you have to tack a suppressing command (`stty -echo`) onto the wrapped command (the one-liner) itself, and the upstream command has to wait, say with `sleep`, until that echo suppression is actually in place. On top of that, the upstream command also has to avoid exiting — again using something like `sleep` — until `script` has finished processing.

```sh
$ ( sleep 0.1 ; echo foo ; sleep 0.1 ) | script -qe /dev/null sh -c 'stty -echo; tee /dev/stderr' | sed 's/.*/stdout: &/'
stdout: foo
stdout: foo  # <- the BSD version mixes stdout and stderr together too (more on this below)
$ 
```

The minimum number of seconds to give `sleep` (especially the second one) isn't something you can determine theoretically — it depends on factors like the OS's implementation and the data size. That makes the BSD version of `script` practically useless for this purpose (as a substitute for `stdbuf`).

### Issue 2: It mixes standard output and standard error together

`script`, too, has the same problem as `unbuffer` and `pipetty`: it mixes standard output and standard error together. This is exactly what the command output shown under [Issue 1](#issue-1-there-are-multiple-script-implementations-with-incompatible-syntax-and-features) demonstrated.

### Issue 3: Newlines get converted from LF to CRLF

Because `OPOST`/`ONLCR` are enabled by default on the PTY slave, every `\n` in the data gets converted to `\r\n` on output. This is easiest to see by piping `script`'s output through `cat -v`.

```sh
# util-linux version
$ echo foo | script -E never -qec 'tee /dev/stderr' /dev/null | sed 's/.*/stdout: &/' | cat -v
stdout: foo^M
stdout: foo^M
$ 

# BSD version
$ ( sleep 0.1 ; echo foo ; sleep 0.1 ) | script -qe /dev/null sh -c 'stty -echo; tee /dev/stderr' | sed 's/.*/stdout: &/' | cat -v
stdout: foo^M
stdout: foo^M
$ 
```

If exact binary matching or `diff`-based comparison is a requirement, this conversion is another problem that breaks transparency.

### Throughput on large transfers

Running the one-liners below for `ptw` and `script` (util-linux version), we compared how long it took each one to relay 1 million lines through `tr 1 1` (measured in the author's test environment; average of 3 runs).

```sh:
$ time yes | head -n 1000000 | ptw                   tr 1 1            >/dev/null
$ time yes | head -n 1000000 | script -E never -qec "tr 1 1" /dev/null >/dev/null
```

|      | `ptw`    | `script` (util-linux version) |
|------|---------:|-------------------------------:|
| real | ~0.016s  |                         ~6.32s |
| sys  | ~0.016s  |                         ~5.79s |

**`ptw` is more than 360 times faster.**

### Verdict

`script` has a low adoption barrier thanks to being preinstalled, but by design it is simply not suited for the use case `ptw` targets — a substitute for `stdbuf`. There are several reasons for that: the incompatible syntax and features between the util-linux and BSD versions, the standard-output/standard-error mixing, the LF-to-CRLF conversion, and the dramatically worse processing speed.

## Summary

None of the tokideli commands covered here were built simply because "no existing command did the same thing." In every case, investigation found that the existing option fell short of the level tokideli aims for — in precision, performance, features, or reliability — which is why it was implemented anew. Some, like `pv -L`, `unbuffer`, and `pipetty`, have clear-cut roughness or defects traceable to their own design and implementation. Others, like rom1v's `delay` and moreutils' `ts`, come down not to simple superiority but to a difference in the performance the use case demands. And still others, like `asciinema`, aren't competitors at all, but something to aim for interoperability with in the future.
