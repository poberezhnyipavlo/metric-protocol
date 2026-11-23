#include "metric_protocol/Signature.h"

#include "FieldParser.h"

namespace {
constexpr char kHex[] = "0123456789abcdef";

uint8_t hexValue(char c) {
    return metric::detail::isDigit(c)
        ? static_cast<uint8_t>(c - '0')
        : static_cast<uint8_t>(c - 'a' + 10);
}

// hex.size() == out.size() * 2 lowercase hex digits -> bytes. false: not hex (out is garbage then).
template <std::size_t N>
bool decodeHex(std::string_view hex, std::array<uint8_t, N>& out) {
    if (hex.length() != N * 2) {
        return false;
    }

    for (std::size_t i = 0; i < N; ++i) {
        const char high = hex[i * 2];
        const char low = hex[(i * 2) + 1];

        if (!metric::detail::isLowerHex(high) || !metric::detail::isLowerHex(low)) {
            return false;
        }

        out[i] = static_cast<uint8_t>((hexValue(high) << 4) | hexValue(low));
    }

    return true;
}
}  // namespace

std::size_t sig::appendSignature(
    char* buf,
    std::size_t payload_len,
    std::size_t cap,
    const Mac& mac
) {
    const std::size_t total = payload_len + kSuffixLen + 1; // +\n
    if (buf == nullptr || payload_len == 0 || total > cap) {
        return 0;
    }
    char* out = buf + payload_len;
    for (const char c : kField) {
        *out++ = c;
    }
    for (const uint8_t byte : mac) {
        *out++ = kHex[byte >> 4];
        *out++ = kHex[byte & 0x0F];
    }

    *out = '\n';
    return total;
}

std::optional<sig::Signed> sig::splitSignature(std::string_view line) {
    line = metric::detail::trimLineEnd(line);

    if (line.size() <= kSuffixLen) {
        return std::nullopt;
    }

    const std::size_t field_at = line.size() - kSuffixLen;
    if (line.substr(field_at, kField.size()) != kField) {
        return std::nullopt;
    }

    Signed out{line.substr(0, field_at), {}};

    if (!decodeHex(line.substr(field_at + kField.size()), out.mac)) {
        return std::nullopt;
    }

    return out;
}

std::optional<sig::Key> sig::parseKey(std::string_view hex) {
    Key key{};
    if (!decodeHex(hex, key)) {
        return std::nullopt;
    }

    return key;
}
