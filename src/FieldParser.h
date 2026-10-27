#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace metric::detail{
inline bool isLower(char c) {
    return c >= 'a' && c <= 'z';
}

inline bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

inline bool isLowerHex(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f');
}

inline std::string_view trimLineEnd(std::string_view s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.remove_suffix(1);
    }

    return s;
}

inline std::optional<std::string_view> takeField(std::string_view& rest, std::string_view key) {
    const std::size_t end = rest.find(' ');
    const std::string_view field = rest.substr(0, end);
    rest = (end == std::string_view::npos) ? std::string_view{} : rest.substr(end + 1);

    if (
        field.size() <= key.size()
        || field.substr(0, key.size()) != key
        || field[key.size()] != '='
    ) {
        return std::nullopt;
    }

    return field.substr(key.size() + 1);
}

inline std::optional<uint32_t> parseUint32(std::string_view s) {
    if (s.empty() || s.size() > 10) {
        return std::nullopt;
    }
    uint64_t v = 0;
    for (const char c : s) {
        if (!isDigit(c)) {
            return std::nullopt;
        }
        v = v * 10 + static_cast<uint64_t>(c - '0');
    }
    if (v > UINT32_MAX) {
        return std::nullopt;
    }

    return static_cast<uint32_t>(v);
}

inline std::optional<uint32_t> parseHex32(std::string_view s) {
    if (s.size() != 8) {
        return std::nullopt;
    }
    uint32_t v = 0;
    for (const char c : s) {
        if (!isLowerHex(c)) {
            return std::nullopt;
        }
        const uint32_t digit =
            isDigit(c) ? static_cast<uint32_t>(c - '0') : static_cast<uint32_t>(c - 'a' + 10);

        v = (v << 4) | digit;
    }

    return v;
}
} // namespace metric::detail
