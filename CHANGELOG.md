# Changelog

All notable changes to this project. The format follows [Keep a Changelog](https://keepachangelog.com/),
versions follow [Semantic Versioning](https://semver.org/).

## [3.0.0] - 2026-10-27

### Added
- `metric_protocol/LogProtocol.h`: wire format for node event logs (`ver=1`), moved from esp32-monitoring-node:
  `logproto::serialize()`, `isValidSrc()`, `toString(Level)`.
- `logproto::parse()` and `logproto::parseLevel()` for the receiver.

### Changed
- **Breaking:** constants renamed to the `kCamelCase` style: `k_version` → `kVersion`, `k_max_name_len` → `kMaxNameLen`,
  `k_dev_len` → `kDevLen`, `k_max_message_len` → `kMaxMessageLen`; `names::k_temperature_c` → `names::kTemperatureC`,
  `k_humidity_pct` → `kHumidityPct`, `k_temp_alert` → `kTempAlert`, `k_dht_ok` → `kDhtOk`;
  `temp_alert::k_too_cold` → `kTooCold`, `k_normal` → `kNormal`, `k_too_hot` → `kTooHot`.

The metric wire format is unchanged (`ver=2`), devices and receivers on 2.x and 3.0 understand each other.
The library stays C++17.

## [2.1.0] - 2026-10-21

### Added
- `metric_protocol/MetricNames.h`: shared metric names (`metric::names`) for the monitoring node:
  `temperature_c`, `humidity_pct`, `temp_alert`, `dht_ok`.
- `metric::temp_alert` values: `k_too_cold` (-1), `k_normal` (0), `k_too_hot` (1).

The wire format is unchanged (`ver=2`); any valid name is still accepted by `parse()`.

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