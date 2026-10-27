#pragma once

// Wire format for node events ("black box" logs), one line per event:
//   ver=1 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=0 age_ms=2843 level=error src=boot text=started
// dev, boot, age_ms mean the same as in metric::Message. seq is a separate counter from metrics.


#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace logproto {
inline constexpr uint32_t kVersion = 1;
inline constexpr std::size_t kMaxSrcLen = 15;
inline constexpr std::size_t kMaxTextLen = 96;
// Buffer for serialize(): the longest line with '\n' and '\0' fits.
inline constexpr std::size_t kMaxLineLen = 220;

enum class Level : uint8_t {
    kInfo,
    kWarn,
    kError,
};

[[nodiscard]] std::string_view toString(Level level);

[[nodiscard]] std::optional<Level> parseLevel(std::string_view s);

struct Record {
    std::string_view dev;
    uint32_t boot;
    uint32_t seq;
    uint32_t age_ms;
    Level level;
    std::string_view src;
    std::string_view text;
};

// [a-z0-9_]{1,15}
[[nodiscard]] bool isValidSrc(std::string_view src);

[[nodiscard]] std::size_t serialize(const Record& record, char* buf, std::size_t cap);

[[nodiscard]] std::optional<Record> parse(std::string_view line);
} // namespace logproto
