# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

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
