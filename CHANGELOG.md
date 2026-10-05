# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

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
