# Report on the Transfer-Rate Precision of the valve Command

（日本語版は[こちら](valve_precision.info.ja.md)）

## Overview

This report examines the precision of the data transfer rate limiting command [valve(1)](valve.man.en.md) from two angles.

1. Its design-level advantage over the well-known `pv` command's rate-limiting feature (`-L`), based on a source-code comparison and an empirical measurement comparison.
2. The unavoidable sources of error that even valve itself is subject to on a typical spec of hardware running a typical UNIX-like environment, along with actual measured figures.

All measurements were taken using the `-9 -d` options of the [linets(1)](linets.man.en.md) command, which attaches a nanosecond-precision timestamp to each line, together with the elapsed time ("delta-t") since the previous line was written.

### Experimental Environment

| Item | Value |
|---|---|
| CPU | AMD Ryzen 7 7735HS with Radeon Graphics |
| Logical cores | 16 |
| Kernel | Linux 6.18.33.2-microsoft-standard-WSL2 (WSL2, PREEMPT_DYNAMIC) |
| valve build command | `cc -O3 -o valve valve.c -pthread -lrt` |
| pv version | 1.11.0 (built in the same environment) |

WSL2 is a Linux kernel running inside a lightweight virtual machine on top of Windows, which adds an extra layer of scheduling abstraction from the host OS (Windows). Bear in mind that the error figures reported below may therefore be somewhat larger than what would be observed on a bare-metal Linux environment.

## ① valve's Advantage Over pv, by Comparison

### The Design Difference

[valve(1)](valve.man.en.md) and pv's `-L` (rate limit) both aim to send data out at a constant rate, but they take fundamentally different approaches to timing control.

#### valve: Absolute-Time-Based Scheduling

Inside `spend_my_spare_time()` (in `c_src/valve.c`), valve directly computes "the absolute time at which the next block should be sent," and calls `nanosleep()` against that time.

```c
/*--- Calculate "tsTo", the time until which I have to wait ------*/
ui8 = (uint64_t)tsPrev.tv_nsec + gi8Peritime;
tsTo.tv_sec  = tsPrev.tv_sec + (time_t)(ui8/1000000000);
tsTo.tv_nsec = (long)(ui8%1000000000);
```

What matters here is that when `tsPrev` (the reference time) is updated after sending, it is set to **the theoretical scheduled time (`tsTo`), not the time the send actually happened**. As a result, even if `nanosleep()` oversleeps somewhat, that error never accumulates over subsequent iterations. valve also has a "recovery mode" that shortens subsequent sleep durations to catch up if oversleeping continues. In practice, precision is determined almost entirely by the resolution of `nanosleep()` itself.

#### pv: A Polling-Based, Discrete Token-Bucket Scheme

Analyzing the pv 1.11.0 source code (`src/pv/loop.c`, `src/pv/transfer.c`), the `-L` rate limit is implemented as follows.

```c
#define RATE_GRANULARITY 100000000  /* nsec between -L rate chunks */
...
while (pv_elapsedtime_compare(&cur_time, &next_ratecheck) > 0) {
    rate_limited_target += rate / (1000000000.0 / RATE_GRANULARITY);
    pv_elapsedtime_add_nsec(&next_ratecheck, RATE_GRANULARITY);
}
cansend = (off_t)(rate_limited_target);
```

The transferable amount is only ever updated in **100-millisecond** steps — a stair-stepped token bucket. Furthermore, the mechanism for actually noticing "it's now possible to send" depends on the next `select()` call.

```c
n = is_data_ready(check_read_fd, &ready_to_read, check_write_fd, &ready_to_write, 90000);
```

This timeout is hard-coded to **90 milliseconds**. In addition, pv reads input ahead into its buffer regardless of the rate limit, until the buffer is full. In a situation where a large amount of data is fed in immediately through a pipe (exactly the case in the experiment below), the buffer fills up quickly, after which `select()` is called with no file descriptor left to monitor while "the buffer is full and there is no send allowance" — effectively turning it into a **90-millisecond polling timer**.

In other words, pv's send timing is governed by the combination of two coarse, mutually uncoordinated cycles: a 100ms token-refill cycle and a 90ms polling cycle. This asynchronous combination produces a beat-like interference, so that even though the average rate is correct, individual send events are jittered on the order of these cycle lengths.

#### Summary of the Design Difference

| | valve | pv (`-L`) |
|---|---|---|
| Approach | Absolute-time-based scheduling | Polling-based, discrete token-bucket scheme |
| Timing determination | Directly computes the absolute time at which the next block should be sent, and calls `nanosleep()` against it | Determined by the combination of a 100ms-granularity token refill and a 90ms-period `select()` poll |
| Error accumulation | Does not accumulate, since the reference time is updated to the theoretical scheduled time (oversleep is also absorbed by the recovery mode) | The two coarse, mutually-asynchronous cycles interact, producing beat-like jitter |
| Theoretical precision limit | Asymptotically approaches the resolution of `nanosleep()` and `clock_gettime()` themselves | Bounded by the 100ms token granularity and the 90ms polling period (on the order of roughly 100-190ms at worst) |

### Empirical Comparison

Precision at a rate of 1 line/second was measured using the following commands (60 lines, roughly 60 seconds).

```sh:
$ seq -f '%05.0f' 1 60 | valve 48bps | linets -9 -d
$ seq -f '%05.0f' 1 60 | pv -qL 6   | linets -9 -d
```

("48bps" translates, at 8 bits per character, to 6 bytes/sec; "pv -qL 6" likewise specifies 6 bytes/sec. Both settings send one 6-character line (e.g. "00001\n") per second. `-q` suppresses pv's progress display.)

The elapsed time between consecutive lines (delta-t, attached by linets), tabulated as a deviation from the expected 1000ms, is as follows.

| | valve 48bps | pv -qL 6 |
|---|---:|---:|
| Sample count | 59 | 59 |
| Mean interval | 1000.0019 ms | 999.9394 ms |
| Interval stdev | **0.288 ms** | **3.573 ms** |
| Max deviation (abs) | **1.069 ms** | **7.704 ms** |
| Mean deviation (abs) | 0.187 ms | 2.868 ms |

Both the standard deviation and the maximum deviation show valve to be roughly **an order of magnitude (10x or more) more precise** than pv. This is consistent with the design difference described above: valve's absolute-time-based scheduling versus pv's token-bucket scheme, which depends on coarse polling cycles.

## ② The Precision Limits of valve Itself

That said, valve is not infallible either. Since it relies on the OS-provided time-management facilities `nanosleep()` and `clock_gettime()`, errors on the order of microseconds to sub-milliseconds are unavoidable when running on a typical spec of hardware in a typical UNIX-like environment. The main contributing factors are:

* **The resolution limit of `nanosleep()` itself**: Linux's high-resolution timers (hrtimers) offer a resolution on the order of a few microseconds, but that only concerns the precision of the wake-up *request* — not when the process is actually resumed.
* **Scheduler intervention**: Even after waking from `nanosleep()`, execution can be delayed if another process (especially a higher-priority one) is using the CPU. The granularity of the CFS (Completely Fair Scheduler) time slice also plays a role here.
* **Context-switch overhead**: Suspending and resuming a process inherently costs a few to a few dozen microseconds.
* **Interrupt latency**: Handling a hardware timer interrupt can be delayed by a few dozen microseconds, for instance when the CPU returns from a power-saving state.
* **CPU frequency scaling (DVFS)**: Transitioning from a low power-saving clock speed back to normal speed can take some time.
* **Virtualization overhead**: In an environment like WSL2 (used for this experiment), where the host OS's scheduler sits on top of the guest OS's scheduler, additional jitter can arise.
* **Error from the measurement method itself**: linets, the tool used for measurement, itself calls `clock_gettime()` and performs formatting work, and that processing time is added, however slightly, to the measured results.

### Measurements

Actual jitter at shorter periods (10 milliseconds and 1 millisecond) was measured using the following commands.

```sh:
$ seq -f '%05.0f' 1 500  | valve -l 10ms | linets -9 -d
$ seq -f '%05.0f' 1 1000 | valve -l 1ms  | linets -9 -d
```

| | 10ms period | 1ms period |
|---|---:|---:|
| Sample count | 499 | 999 |
| Mean interval | 10.0008 ms | 1000.05 μs |
| Interval stdev | 254.06 μs | 28.74 μs |
| Max deviation (abs) | 860.18 μs | 246.89 μs |
| Mean deviation (abs) | 170.32 μs | 19.20 μs |

An excerpt of the actual output (10ms period; the timestamp and the elapsed seconds since the previous line, both attached by linets):

```text:
20260906013151.427904104 0 00001
20260906013151.438012167 0.010108063 00002
20260906013151.447994354 0.009982187 00003
20260906013151.458026325 0.010031971 00004
20260906013151.468087135 0.010060810 00005
```

A few observations follow from this data.

* The absolute magnitude of the jitter (a maximum deviation on the order of a few hundred microseconds) stays roughly in the same range regardless of the period length. This supports the view that the main source of jitter is fixed-magnitude noise arising from OS scheduling and interrupt handling, rather than a flaw in valve's own period-calculation logic.
* Consequently, the shorter the period, the larger the **relative** impact of this fixed-magnitude noise becomes (a maximum relative error of roughly 8.6% at a 10ms period, versus roughly 24.7% at a 1ms period). Bear in mind that there is an inherent limit to how demanding a sub-millisecond precision requirement can be met by an ordinary application running on a standard, time-sharing OS kernel.
* On the other hand, at a comparatively long period such as the 1-second one used in section ①, the absolute magnitude of this fixed noise (on the order of at most about 1ms) amounts to only about 0.1% in relative terms — a precision that is practically negligible for most purposes.

## Summary

* Unlike pv, whose rate limiting depends on coarse polling cycles, valve directly computes the absolute time at which the next block should be sent and calls `nanosleep()` against it, achieving roughly an order of magnitude better precision in the same environment (standard deviation 0.288ms vs. 3.573ms, measured at a 1-second period).
* Even so, valve's precision cannot escape a few-hundred-microsecond-order of fixed noise arising from OS scheduling and interrupt handling. Since this fixed noise stays roughly constant regardless of the period length, the relative error grows larger for shorter periods. In practice, it is advisable to use valve with an awareness of the balance between the precision required and the length of the period being used.
