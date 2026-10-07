# metric-protocol

Text wire format for sending metrics over UDP, one metric per datagram:

```
ver=1 seq=42 name=uptime_s value=123
```

| Field | Meaning |
|---|---|
| `ver` | wire format version; a receiver rejects versions it does not know |
| `seq` | datagram sequence number (`uint32`), lets the receiver see loss and reordering |
| `name` | metric name: `[a-z][a-z0-9_]*`, 1–31 characters |
| `value` | finite number, serialized with `%.10g` (any `uint32` without loss) |

A trailing `\n` is added by `serialize()` and ignored by `parse()`.

Used by two projects that must agree on the format:

- the ESP32 sender ([esp32-lab / 10-udp-sender](https://github.com/poberezhnyipavlo/esp32-lab/tree/main/10-udp-sender)) via PlatformIO;
- the System Monitor receiver via CMake.

No Arduino or OS dependencies, C++17, no heap allocation.

## API

```cpp
#include <metric_protocol/MetricProtocol.h>

// sender
std::array<char, metric::k_max_message_len> buf{};
size_t len = metric::serialize(metric::Metric{"uptime_s", 123}, seq, buf.data(), buf.size());
// len == 0: invalid name, NaN/inf value, or buffer too small; nothing to send

// receiver
if (auto msg = metric::parse(text)) {
  // msg->seq, msg->name (string_view into `text`), msg->value
}
```

`parse()` is strict: exactly four fields in this order, known version, valid name and numbers; anything else is
`std::nullopt`. Which metric names to accept is the receiver's policy and is not part of this library.

## Use it

PlatformIO (`platformio.ini`):

```ini
lib_deps = https://github.com/poberezhnyipavlo/metric-protocol.git#v1.0.0
```

CMake:

```cmake
include(FetchContent)
FetchContent_Declare(metric_protocol
  GIT_REPOSITORY https://github.com/poberezhnyipavlo/metric-protocol.git
  GIT_TAG v1.0.0)
FetchContent_MakeAvailable(metric_protocol)
target_link_libraries(your_target PRIVATE metric_protocol::metric_protocol)
```

Tests are built only when this repository is the top-level CMake project.

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Versioning

- The library follows semantic versioning; the git tag, `library.json` and `project(VERSION)` in `CMakeLists.txt` must match.
- The wire format version (`ver=`) changes only when the format on the wire changes, which is also a major library
  version. Library bug fixes do not change `ver`.

## License

MIT