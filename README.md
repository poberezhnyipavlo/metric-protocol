# metric-protocol

Text wire format for sending metrics from devices, one metric per line (one UDP datagram, or one `\n`-terminated
line in a TCP stream).

```
ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=42 age_ms=1500 name=uptime_s value=123
```

| Field | Meaning |
|---|---|
| `ver` | wire format version; decides which fields follow. Unknown versions are rejected |
| `dev` | device ID: the factory MAC from the chip's eFuse, lowercase, with colons (exactly 17 characters) |
| `boot` | random `uint32` chosen at every start of the device, exactly 8 lowercase hex digits |
| `seq` | sequence number (`uint32`) within one `boot`; lets the receiver see loss and duplicates |
| `age_ms` | how old the sample is at the moment it is **sent**, in ms (`uint32`, up to ~49 days) |
| `name` | metric name: `[a-z][a-z0-9_]*`, 1–31 characters |
| `value` | finite number, serialized with `%.10g` (any `uint32` without loss) |

A trailing `\n` is added by `serialize()` and ignored by `parse()`. The longest `ver=2` line is 136 bytes.

### Why these fields

- **`age_ms` instead of a timestamp.** The device needs no clock and no NTP (there may be no internet, or the link may
  be expensive). The receiver computes `sample time = receive time − age_ms`. Samples that waited in a queue on the
  device arrive with a large `age_ms`.
- **`boot` next to `seq`.** `seq` restarts from 0 after every reboot. Deduplicating by `(dev, seq)` would silently
  drop new data after a reboot as "duplicates"; `(dev, boot, seq)` is unique. A changed `boot` also tells the receiver
  that the device restarted.
- **`dev` from eFuse.** Unique per chip, needs no registration or configuration. It is an identifier, not
  authentication: anyone on the network can send any `dev`.

### Older versions

`parse()` accepts `ver=2` only; `ver=1` lines are rejected. Policy: when the format changes, devices and receivers
are updated together. Until a device is updated, its lines are dropped by the receiver.

## API

```cpp
#include <metric_protocol/MetricProtocol.h>

// sender: once per start
std::array<uint8_t, 6> mac{};
esp_efuse_mac_get_default(mac.data());               // ESP32
const auto dev = metric::formatDev(mac);             // "20:9b:a9:61:15:6c"
const metric::Origin origin{dev.data(), esp_random()};

// sender: per metric
std::array<char, metric::k_max_message_len> buf{};
size_t len = metric::serialize(origin, seq, age_ms, metric::Metric{"uptime_s", 123}, buf.data(), buf.size());
// len == 0: invalid dev/name, NaN/inf value, or buffer too small; nothing to send

// receiver
if (auto msg = metric::parse(text)) {
  // msg->dev, msg->boot, msg->seq, msg->age_ms, msg->name, msg->value
  // dev and name are string_views into `text`
}
```

`parse()` is strict: exactly seven fields in this order, `ver=2`, valid values; anything else (including a
trailing space) is `std::nullopt`. Which metric names to accept is the receiver's policy and is not part of this library.

No Arduino or OS dependencies, C++17, no heap allocation.

## Use it

PlatformIO (`platformio.ini`):

```ini
lib_deps = https://github.com/poberezhnyipavlo/metric-protocol.git#v2.0.0
```

CMake:

```cmake
include(FetchContent)
FetchContent_Declare(metric_protocol
  GIT_REPOSITORY https://github.com/poberezhnyipavlo/metric-protocol.git
  GIT_TAG v2.0.0)
FetchContent_MakeAvailable(metric_protocol)
target_link_libraries(your_target PRIVATE metric_protocol::metric_protocol)
```

Projects pinned to `v1.0.0` keep building unchanged.

Tests are built only when this repository is the top-level CMake project.

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Versioning

- The library follows semantic versioning; the git tag, `library.json` and `project(VERSION)` in `CMakeLists.txt`
  must match. See [CHANGELOG.md](CHANGELOG.md).
- The wire format version (`ver=`) changes only when the format on the wire changes, which is also a major library
  version. Library bug fixes do not change `ver`.
- A new wire version is rolled out to devices and receivers together; old versions are not accepted.

## License

MIT