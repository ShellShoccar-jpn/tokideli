# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- `linets`/`charts`: in `-z`/`-Z` mode, sending the process SIGUSR1 now
  rebases the elapsed-seconds origin (column 1) on the fly — to the
  signal's own arrival instant for `-z`, or to the next-arriving
  line/character's instant for `-Z`. Sending SIGUSR2 does the same, and
  additionally rebases the baseline `-d` uses for its elapsed-since-
  previous-line column, so the first line/character after a SIGUSR2
  reset always reports a `-d` delta of (effectively) zero.
- `qvalve`: a new `-k` option that runs an external command (given as a
  single string, run via `system(3)`) every time the amount of data
  actually output reaches the current target, when there is no more
  input data, when the controlfile receives the `t` command, or when
  the command is about to terminate via `-t`. Since it runs via
  `system(3)`, `qvalve`'s own data transfer blocks for as long as the
  external command runs; this is intentional. For the latter two cases
  (`controlfile_closed`/`t_command`), the data transfer is always fully
  stopped before the external command runs, and `qvalve` itself
  terminates only after the external command finishes. The command
  receives `QVALVE_EXEC_BY` (`totalsize_reached`/`end_of_data`/
  `controlfile_closed`/`t_command`), `QVALVE_TOTAL_SIZE`,
  `QVALVE_SET_SIZE`, `QVALVE_UNIT` (`char`/`line`), and `QVALVE_PID`
  (this command's own process ID) as environment variables.

### Fixed

- `qvalve`: sending the process SIGHUP, while a regular file is used as
  the controlfile, was documented to force an immediate re-read of it
  (instead of waiting for the next periodic poll), but actually crashed
  the command (SIGHUP's default disposition) because the sub-thread
  that watches the controlfile unblocked SIGHUP for itself without ever
  registering a handler for it. It now registers a (do-nothing) handler
  for SIGHUP there too, matching the already-correct handling of
  SIGALRM right next to it, so SIGHUP behaves as documented.
- `relval`: the `-d` drain-file was opened without `O_APPEND`, so
  pointing it at a pre-existing file silently overwrote its contents
  from the beginning instead of appending, unlike every other command
  in this project with an equivalent drain/monitor-file option
  (`oobleck`'s `-d`, `surgetk`'s `-m`). It now opens with `O_APPEND`,
  matching them.
- `manual/{linets,tscat,typeliner}.man.{ja,en}.md`: several one-way
  "See Also" gaps were found and fixed — `linets.man` was missing
  `charts(1)`, `typeliner(1)`, and `relval(1)`; `typeliner.man` was
  missing `charts(1)`; `tscat.man` was missing `relval(1)`, `tshead(1)`,
  and `tstail(1)` — in each case despite the other side already linking
  back, or the body text explicitly discussing the relationship.

### Changed

- `manual/linets.man.{ja,en}.md`: document the new SIGUSR1/SIGUSR2
  behavior in its own subsection (covering both `-z` and `-Z`).
- `manual/charts.man.{ja,en}.md`: note that `charts` inherits this same
  SIGUSR1/SIGUSR2 behavior from `linets`, since the existing "format is
  identical to linets(1)" sentence didn't by itself imply that the
  signal behavior is too.

## [1.2.0] - 2026-10-16

### Added

- `linets`/`charts`: a new `-j` option that JSON-escapes the data as it is
  read in (`"`, `\`, and control characters via the standard short escapes
  such as `\t`/`\n`, or `\u00XX` for anything else below 0x20; with
  `charts`'s `-b`, bytes 0x80 and above are also escaped as `\u00XX`).
  This makes it straightforward to turn recorded output into valid JSON
  string content, or feed it safely through JSON-aware tooling, without
  having to worry about embedded quotes, backslashes, or control bytes
  (such as the escape sequences sent by arrow keys) breaking anything
  downstream.
- `tscat`: a matching `-j` option that reverses the escaping applied by
  `linets`/`charts`'s own `-j`, turning the data back into its original
  bytes.
- `manual/tokideli_vs_simillar_products.info.{ja,en}.md`, a report
  comparing `valve`, `delay`, `linets`/`charts`, and `ptw` against
  similar existing commands (`pv -L`, rom1v/delay, moreutils' `ts`,
  asciinema, and expect's `unbuffer`), explaining why each was
  implemented anew rather than relying on the existing alternative.

### Fixed

- `linets`/`charts`/`tscat`: elapsed-time measurements (the `-z`/`-Z`/`-d`
  options and related calculations) were computed from `CLOCK_REALTIME`,
  so a wall-clock adjustment (e.g. an NTP correction, a manual date
  change) occurring mid-measurement could corrupt the reported interval.
  They now use `CLOCK_MONOTONIC`, which is immune to such adjustments.

### Changed

- `manual/{linets,charts,tscat}.man.{ja,en}.md`: document the new `-j`
  option.
- `manual/rec_baby_typing.info.{ja,en}.md`: the tutorial's one-liners now
  use `-j`, and the appendix's sample typing data is shown already
  JSON-escaped, matching what `-j` would actually produce when recording.

## [1.1.1] - 2026-10-09

### Fixed

- Every command's basename-extraction loop (used to compute the `gpszCmdname`
  shown in error messages and in `--version` output) failed to fully strip
  the directory portion of `argv[0]` whenever it contained two or more `/`
  characters (e.g. `/usr/bin/sleep`), leaving a partial path fragment (e.g.
  `bin/sleep`) instead of the plain command name. The loop re-pointed
  `gpszCmdname` mid-scan without resetting its own index, so the scan
  position drifted out of sync with the string it was now scanning. This
  went unnoticed because invoking a command via a single-component relative
  path (e.g. `./sleep`, or a bare name found via `$PATH`) happened not to
  trigger the bug; it surfaced while preparing this project's first
  Homebrew/AUR packages, where commands are commonly invoked by their full
  installed path. Discovered and fixed across all 18 `c_src/*.c` files.

## [1.1.0] - 2026-10-08

### Added

- A `Package : tokideli` line in every command's `-h`/usage banner, shown
  above the `Version` line, so a banner on its own makes clear which
  project the command belongs to (matches the long-standing GNU-tool
  `--version` convention of naming the parent package, e.g.
  `sleep (GNU coreutils) 9.1`).
- `manual/rec_baby_typing.info.{ja,en}.md`, a tutorial showing how to
  record a baby's first keyboard typing (timing included) with
  `typeliner` and `linets`, and replay it with `tscat`.
- `manual/valve_as_wave_generator_on_raspi.info.{ja,en}.md`, a report
  evaluating how well `yes`/`valve` can drive a Raspberry Pi GPIO pin as
  a square-wave pulse source, compared against a dedicated function
  generator.

### Fixed

- `release/bump_version.sh` previously updated only the `Version : X.Y.Z`
  line in each command's `-h`/usage banner, but not the separate,
  independent `X.Y.Z` literal baked into that same command's
  `--version` output. As a result, a version bump left `--version`
  reporting the old version. The script now updates both.

### Changed

- `calclock`'s (`c_src/calclock.c` and `cmd_scripts/calclock.sh`) banner
  comments now note that it is also maintained, kept in sync, as part of
  Open usp Tukubai, the project it originally came from. Its `Package`
  line still reads `tokideli` only (not both names together), since the
  `Version` line right below it follows tokideli's own release numbering,
  which increases independently of Open usp Tukubai's.
- In every `*.info.{ja,en}.md` document (and `linets_and_tscat.{ja,en}.md`)
  whose language cross-link was placed after the introductory paragraph,
  it now comes right after the title instead, so a reader who opened the
  wrong language notices immediately rather than after reading text they
  can't understand. `*.man.{ja,en}.md` files already placed it correctly.

## [1.0.0] - 2026-10-06

### Added

- `charts`: a new command that attaches a locale-aware timestamp to every
  single character of a text stream (one record per character), as the
  character-grained sibling of `linets` (which does the same per line).
- `--version` support for every command, both in `c_src/` and `cmd_scripts/`.
- A root `VERSION` file as the single source of truth for the project's
  semantic version.
- `release/bump_version.sh`, a maintainer-only script that synchronizes the
  version number across every source file at release time.

### Fixed

- `linets`/`charts`: the timestamp shown for the very first line/character of
  a file could occasionally display as numerically earlier than the one
  shown for the line/character right after it. This was caused by the
  "first unit" code path rounding to the display resolution differently
  from the code path used for every subsequent unit.
- `charts`: a decoding bug in `fetch_1char()` could misflag perfectly valid
  multibyte characters as invalid, caused by re-feeding `mbrtowc()` bytes it
  had already consumed.

### Changed

- The `-h`/usage banner's `Version : TIMESTAMP` line — which, despite its
  name, only ever reflected a single file's own last-edited date, never any
  meaningful version number — has been renamed to `Last Updated`. The
  `Version` label is now reserved for the project's actual semantic version.
