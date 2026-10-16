# Comparing tokideli's Commands with Similar Existing Tools — Why We Reinvented the Wheel

（日本語版は[こちら](tokideli_vs_simillar_products.info.ja.md)）

Some of tokideli's commands look, at first glance, conceptually similar to well-known existing commands. "Why not just use the existing one instead — isn't this reinventing the wheel?" is a fair question to ask. So here, based on actual investigation and measurement of the corresponding existing commands, we explain, command by command, why each tokideli command was implemented anew anyway.

## `valve` vs [pv](https://www.ivarch.com/programs/pv.shtml)'s `-L` Option

Limiting a data transfer to a constant rate is not something only [valve(1)](valve.man.en.md) does. The well-known [pv](https://www.ivarch.com/programs/pv.shtml) command has an equivalent feature in its `-L` option.

The conclusion of comparing the two can be summed up in one line: `pv -L`'s precision is simply too coarse and too loose. Looking at pv 1.11.0's source code (`src/pv/loop.c`, `src/pv/transfer.c`), `-L` turns out to be a discrete token-bucket scheme that only replenishes the sendable quota once every 100 milliseconds, and even detecting that quota itself depends on `select()` polling with a fixed 90-millisecond timeout. These two coarse, mutually unsynchronized periods — a 100ms token refill and a 90ms poll — beat against each other and produce jitter.

`valve`, on the other hand, directly computes the absolute time at which the next chunk should be sent and calls `nanosleep()` against it, so error never accumulates. In a measured 1-line-per-second test, the standard deviation of the interval came out to 0.288ms for `valve` versus 3.573ms for `pv -L` — nearly an order of magnitude apart. The full design comparison and measurement data are written up in the [Report on the Transfer-Rate Precision of the valve Command](valve_precision.info.en.md).

### Verdict

`pv`'s primary purpose appears to be visualizing the flow of data through a pipe — progress monitoring. Rate limiting via `-L` seems to be positioned as an add-on feature bolted onto that primary purpose. If so, it's understandable that precision wasn't a priority.

## `delay` vs [rom1v/delay](https://github.com/rom1v/delay)

A command also named "`delay`", with a similar concept, already existed before tokideli ([rom1v/delay](https://github.com/rom1v/delay)). Both share the same goal — "delay data arriving on standard input by a fixed amount of time before sending it to standard output" — but on inspection, their design philosophies turned out to differ considerably.

* **Performance/design**: rom1v/delay stores data in a ring buffer in units of timestamped "chunks" (up to 4000 bytes each), so its timestamp granularity is limited to millisecond precision per chunk. tokideli's [delay(1)](delay.man.en.md), by contrast, records an individual nanosecond-precision timestamp for every single byte. The price of that precision is memory usage roughly 15 times the requested size.
* **Features**: rom1v/delay always blocks further reads once its buffer fills. tokideli's `delay` additionally offers several overflow-handling modes — `-o` (overwrite) and `-d` (disconnect) — and lets you change the delay time and buffer size at runtime via its `controlfile` mechanism. rom1v/delay has neither option.
* **Philosophy/portability**: rom1v/delay is a small, single-purpose tool spread across 5 source files, with active development having stopped since May 2019. An unresolved GitHub issue notes undefined behavior in a spot where a pointer to a header struct is cast onto a byte array at an alignment-unguaranteed position. tokideli's `delay.c` is a single, self-contained file, verified to build and run on 7 environments: Linux, Solaris, FreeBSD, NetBSD, OpenBSD, macOS, and Linux-based Android.

The finer details (the technical breakdown of how "buffer size" means different things in each, the overflow-behavior comparison, and so on) are written up in [Comparing tokideli's delay Command with an Earlier delay Command (rom1v/delay)](delay_commands_comparision.info.en.md); see that for more.

### Verdict

rom1v/delay is memory-efficient and easy to understand, but lacks operational flexibility and fine-grained timestamp precision. tokideli's `delay` gains byte-level, nanosecond precision and runtime flexibility, at the cost of higher memory usage. Neither is simply "better" — the right choice depends on how much precision and flexibility the use case demands.

## `linets`/`charts` vs `ts` ([moreutils](https://github.com/pgdr/moreutils))

Among existing commands, [moreutils](https://github.com/pgdr/moreutils)' `ts` is the closest match to the idea of attaching a timestamp to each line.

`ts` is implemented in Perl. Perhaps because some consider Perl an outdated language and are reluctant to depend on it, an [unofficial, Go-based reimplementation of moreutils](https://github.com/sweetbbak/go-moreutils) has even appeared. And indeed, `ts`'s slowness is hard to deny. Running the same job (attaching a leading timestamp to 100,000 lines) through both commands showed `ts` taking roughly twice as long.

```sh:
$ time yes | head -n 100000 | ts %.s     > /dev/null
$ time yes | head -n 100000 | linets -6e > /dev/null
```

(Measured in this document's test environment at the time of writing; average of 3 runs.)

| | `ts %.s` | `linets -6e` |
|---|---:|---:|
| real | 0.88s | 0.46s |
| user | 0.28s | 0.09s |
| sys | 0.59s | 0.37s |

Wall-clock time (real) shows roughly a 1.9x gap, and user CPU time alone shows roughly a 3x gap — a good illustration of how much per-line overhead the Perl interpreter is paying.

On the feature side, the differences are:

* On the point of output-format flexibility, there's no real complaint about `ts`. Calendar date-time, UNIX time, and extended ISO 8601 format are all supported by [linets(1)](linets.man.en.md)/[charts(1)](charts.man.en.md) too, down to microsecond precision.
* `ts`'s `-s` (elapsed time since the program started) corresponds to `linets`'s `-z`. However, `ts` has no equivalent to `linets`'s `-Z` (elapsed time since the first line of the text data arrived).
* `linets` timestamps the moment the **first character** of a line arrives. `ts`, on the other hand, timestamps the moment its read loop (`while (<>)`) finishes reading the entire line — i.e., when the line's end (newline or EOF) arrives.
* Choosing `ts`'s `-i` (elapsed time since the previous timestamp) makes the output show only the delta; there's no way to get both the absolute timestamp and the delta at once. `linets`'s `-d` option, by contrast, outputs both the absolute timestamp in column 1 and the elapsed time since the previous line in column 2, simultaneously.
* `ts` offers a `-m` option to use the monotonic clock, but reading `ts`'s own Perl source shows that even with `-m`, the output is still converted back to wall-clock (local time) form before being printed — it simply adds the monotonically-measured elapsed time to a wall-clock baseline captured at startup. If the system clock is corrected mid-run, the displayed absolute time will not reflect that correction. So when combined with an absolute-time display format (calendar date-time, UNIX time, extended ISO 8601), choosing `-m` buys little. `linets` instead automatically picks REALTIME for its absolute-time formats (`-c`/`-e`/`-I`) and MONOTONIC for its elapsed-time formats (`-z`/`-Z`), based on what each option actually means.
* `ts` can only attach timestamps line by line; it has no standalone way to record at the character or byte level. If you really need that, you'd have to pre-process the text data into one-character-per-line form first, but doing so takes time of its own, which undermines the whole point of recording arrival timing accurately. `charts` records character-level timestamps directly, with no such pre-processing step needed.

### Verdict

`ts` is implemented in Perl, a language some consider somewhat dated and are reluctant to depend on, and it is also slower in practice. `linets`/`charts`, written in POSIX C, have a lower barrier to adoption and are faster. They are also somewhat more capable: `ts` cannot output an absolute timestamp and a delta together, nor record at the character level. `ts`'s flexible `strftime`-based format string is a genuine strength, but format conversion itself can just as well be handed off to a separate text-processing command such as `sed`.

## `charts`/`linets`/`tscat` vs [asciinema](https://asciinema.org/)

[asciinema](https://asciinema.org/) is a wonderful tool. It can record and replay what happens on a terminal screen, and since its data is text-based (JSON Lines, with a `.cast` extension), it's also lightweight.

That said, `asciinema`'s `.cast` format is built with the assumption that it will only ever be consumed by `asciinema` itself. It is text, but each line is a JSON array of the form `[time, type, data]`, and the `data` field is escaped as a JSON string. So while a human can read it as text, processing it directly with general-purpose text tools like `grep` or `sed` is difficult — you need a JSON parser first.

What we wanted was something more general-purpose: a tool that simply controls the flow of the data itself passing through standard input/output. The timestamped text that [charts(1)](charts.man.en.md), [linets(1)](linets.man.en.md), and [tscat(1)](tscat.man.en.md) produce has no JSON-array structure wrapped around it, so it can be fed straight into ordinary UNIX text tools — `grep`, `sed`, `awk`, and the like.

That said, the `.cast` format and the format tokideli's commands produce are, in principle, mutually convertible (aside from the metadata recorded at the start, such as screen size). In fact, the single-line structure that `charts`/`linets`'s `-j` option produces — "timestamp + JSON-escaped payload" — corresponds almost one-to-one with each element of `.cast`'s `[time, type, data]` JSON array. We'd like to provide a conversion tool between the two formats at some point in the future.

### Verdict

`asciinema` and these tokideli commands aren't in a relationship where one is "better" and should be chosen over the other. `asciinema` specializes in recording and replaying terminal-screen interaction, while these commands pursue the more general goal of recording and replaying the flow of standard input/output itself. If anything, the two should become mutually convertible and build a friendly relationship, rather than compete.

## `ptw` vs `unbuffer` ([expect](https://sourceforge.net/projects/expect/))

The problem [ptw(1)](ptw.man.en.md) solves — forcing a command's output buffering mode to line-buffered — is, on paper, also something `unbuffer`, which ships with [expect](https://sourceforge.net/projects/expect/), can handle. But comparing the two in practice leads to the conclusion that `unbuffer` is simply too poorly made to take seriously.

### Startup and processing overhead

`unbuffer` is implemented as a Tcl script (`expect_unbuffer`), paying the cost of starting a Tcl interpreter on every invocation. Measuring the time from start to finish while processing just a single line of data showed the following gap (measured in this document's test environment at the time of writing; average of 3 runs).

```sh:
$ time echo x | unbuffer -p cat > /dev/null
$ time echo x | ptw cat > /dev/null
```

| | `unbuffer -p cat` | `ptw cat` |
|---|---:|---:|
| real | ~0.02s | ~0.003s |

Despite processing only a single line, `unbuffer` takes roughly 5 to 10 times as long as `ptw`. This is the Tcl interpreter's startup cost — an overhead that applies regardless of how much data is actually processed.

### Bug 1: It exits leaving unprocessed data behind the moment the upstream command exits

Try relaying the 4000 lines `seq` produces through `tr` with the following one-liners (`tr 1 1` was chosen here as the relay because it's a harmless filter — it translates the character "1" into "1", i.e. it leaves the data unchanged).

```sh:
$ seq 4000 | unbuffer -p tr 1 1 | cat > with_unbuffer.txt
$ seq 4000 | ptw tr 1 1          | cat > with_ptw.txt
```

`ptw` outputs all 4000 lines in about 9ms. `unbuffer`, by contrast, gives a different result on every run, but in every case ends up with far fewer lines than 4000 (around 21 lines, in a measurement taken for this document) — and the whole thing finishes in a very short time regardless.

The cause lies in how `unbuffer -p` is implemented (the `interact` call inside `expect_unbuffer`): the moment the upstream side of its standard input hits EOF, it waits only briefly for any remaining output from `tr` (`expect -timeout 1 -re .+`) and then cuts the whole thing short. When paired with an upstream command like `seq` that finishes producing its data almost instantly, the upstream hits EOF before `unbuffer`'s internal relay has processed most of the data, and whatever is left simply never gets relayed and is lost.

### Bug 2: It can freeze or lose data to a race condition during large transfers

Artificially delaying the upstream's exit (e.g., tacking a `sleep` onto the end, as in `(seq 4000; sleep 5) | unbuffer -p tr 1 1 | cat`) works around Bug 1. But even then, a second, separate defect remains.

Using this workaround on `seq 4000` in this document's test environment at the time of writing, one run ended up outputting only 840 lines (while still exiting with a normal exit code), and a separate run froze outright and had to be force-killed. Same command, same environment, yet the outcome differed between runs.

Tracking this down using a copy of `strace` obtained without root privileges revealed a telltale pattern in `unbuffer`'s own (tclsh) system call sequence (fd0 = standard input, fd3 = the pty connecting to the child process `tr`):

```text:
read(0, ...)
write(3, "...", 4096)        # relayed correctly, up to here
write(3, "...", 1905)
read(0, ...)
write(3, "...", 4096)
write(3, "...", 1905)
read(0, ...)
write(3, "...", 2417)        # <- last write to the pty
read(0, <unfinished ...>)    # no more write(3, ...) calls after this
read(0, <unfinished ...>)
read(0, "...", 4096) = 4096  # data was read, but not written out
read(0, <unfinished ...>)
# -> blocks here indefinitely
```

In other words, `interact`'s internal loop is implemented as a greedy "keep reading for as long as you can, and put off writing" scheme, and it can accumulate already-read data without ever writing it to the pty side, calling `read()` again instead. If, while stuck in this "read-only" streak, it happens to call `read(0, ...)` at the exact moment the upstream pipe is momentarily empty, that system call blocks until more data arrives — stranding whatever was already read but never forwarded to `tr`.

This is not something that happens deterministically upon hitting some fixed limit, such as a kernel buffer size. Whether a `read` or a `write` gets called next is a **race condition** that depends on the timing of data arriving on the upstream pipe and on OS scheduling. The more data there is (i.e., the more times `read` gets called), the higher the odds of unluckily landing on this "read-only" streak at the exact moment the upstream pipe goes momentarily empty. As a result, how much data gets processed before a freeze (or data loss) occurs varies from run to run, and a single run can't tell you "how many lines are safe."

### Verdict

Comparing this against `ptw` isn't even really worth doing. On top of the performance weakness of its high startup cost, `unbuffer` carries two distinct, genuinely harmful bugs: it cuts off unprocessed data the moment the upstream exits, and it can freeze or lose data to a race condition during large transfers. For tokideli's purpose — accurately recording and replaying the timing of data as it arrives — it is not a tool that can be trusted.

## Summary

None of the existing commands covered here were ignored simply because they "didn't exist" — each was investigated and found to fall short of what tokideli aims for, whether in precision, performance, features, or reliability, which is why it was implemented anew. Some, like `pv -L` and `unbuffer`, have clear-cut roughness or defects traceable to their own design and implementation. Others, like rom1v/delay and `ts`, come down not to a simple matter of one being better, but to a difference in intended use case. And still others, like `asciinema`, are not competitors at all, but something tokideli should aim to interoperate with in the future.
