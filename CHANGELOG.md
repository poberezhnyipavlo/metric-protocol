# Changelog

All notable changes to this project. The format follows [Keep a Changelog](https://keepachangelog.com/),
versions follow [Semantic Versioning](https://semver.org/).

## [2.0.0] - 2026-10-09

### Added
- Wire format `ver=2`: `dev` (device MAC), `boot` (random per start), `age_ms` (sample age at send time).
- `metric::Origin`, `metric::isValidDev()`, `metric::formatDev()`.
- `Message::dev`, `boot`, `age_ms`.

### Changed
- **Breaking:** `serialize()` takes `Origin`, `seq`, `age_ms` and writes `ver=2`.
- **Breaking:** `parse()` accepts `ver=2` only; `ver=1` lines are rejected (update devices and receivers together).
- `k_max_message_len` raised from 96 to 160 (the longest `ver=2` line is 136 bytes).

### Fixed
- `parse()` accepted a line with a trailing space after the last field.

## [1.0.0] - 2026-10-07

### Added
- Wire format `ver=1`: `ver seq name value`, strict `parse()`, `serialize()`, `isValidName()`.
- PlatformIO (`library.json`) and CMake packaging, GoogleTest suite.