#include "metric_protocol/MetricProtocol.h"

#include "FieldParser.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace metric {
using detail::isDigit;
using detail::isLower;
using detail::isLowerHex;
using detail::parseHex32;
using detail::parseUint32;
using detail::takeField;
using detail::trimLineEnd;
namespace {
constexpr char kHex[] = "0123456789abcdef";

std::optional<double> parseDouble(std::string_view s) {
    char tmp[32];
    if (s.empty() || s.size() >= sizeof(tmp)) {
        return std::nullopt;
    }
    for (size_t i = 0; i < s.size(); ++i) {
        tmp[i] = s[i];
    }
    tmp[s.size()] = '\0';

    char* end = nullptr;
    const double v = std::strtod(tmp, &end);
    if (end != tmp + s.size() || !std::isfinite(v)) {
        return std::nullopt;
    }

    return v;
}
}  // namespace

bool isValidName(std::string_view name) {
    if (name.empty() || name.size() > kMaxNameLen || !isLower(name[0])) {
        return false;
    }
    return std::all_of(name.begin(), name.end(), [](char c) {
        return isLower(c) || isDigit(c) || c == '_';
    });
}

bool isValidDev(std::string_view dev) {
    if (dev.size() != kDevLen) {
        return false;
    }
    for (size_t i = 0; i < dev.size(); ++i) {
        const bool colon_place = (i % 3) == 2;
        if (colon_place ? dev[i] != ':' : !isLowerHex(dev[i])) {
            return false;
        }
    }
    return true;
}

std::array<char, kDevLen + 1> formatDev(const std::array<uint8_t, 6>& mac) {
    std::array<char, kDevLen + 1> out{};
    size_t pos = 0;
    for (size_t i = 0; i < mac.size(); ++i) {
        if (i > 0) {
            out[pos++] = ':';
        }
        out[pos++] = kHex[mac[i] >> 4];
        out[pos++] = kHex[mac[i] & 0x0F];
    }
    out[pos] = '\0';

    return out;
}

size_t serialize(
    const Origin& origin,
    uint32_t seq,
    uint32_t age_ms,
    const Metric& m,
    char* buf,
    size_t cap
) {
    if (!isValidDev(origin.dev) || !isValidName(m.name) || !std::isfinite(m.value) ||
        buf == nullptr || cap == 0) {
        return 0;
    }

    const int n = std::snprintf(
        buf,
        cap,
        "ver=%lu dev=%.*s boot=%08lx seq=%lu age_ms=%lu name=%.*s value=%.10g\n",
        static_cast<unsigned long>(kVersion),
        static_cast<int>(origin.dev.size()),
        origin.dev.data(),
        static_cast<unsigned long>(origin.boot),
        static_cast<unsigned long>(seq),
        static_cast<unsigned long>(age_ms),
        static_cast<int>(m.name.size()),
        m.name.data(),
        m.value
    );

    if (n <= 0 || static_cast<size_t>(n) >= cap) {
        return 0;
    }

    return static_cast<size_t>(n);
}

std::optional<Message> parse(std::string_view text) {
    std::string_view rest = trimLineEnd(text);
    if (rest.empty() || rest.size() > kMaxMessageLen || rest.back() == ' ') {
        return std::nullopt;
    }

    const auto ver = takeField(rest, "ver");
    const auto dev = takeField(rest, "dev");
    const auto boot = takeField(rest, "boot");
    const auto seq = takeField(rest, "seq");
    const auto age = takeField(rest, "age_ms");
    const auto name = takeField(rest, "name");
    const auto value = takeField(rest, "value");

    if (!ver || !dev || !boot || !seq || !age || !name || !value || !rest.empty()) {
        return std::nullopt;  // не те поле, не той порядок, або щось зайве в кінці
    }

    const auto version = parseUint32(*ver);
    if (!version || *version != kVersion) {
        return std::nullopt;
    }

    const auto boot_num = parseHex32(*boot);
    const auto seq_num = parseUint32(*seq);
    const auto age_num = parseUint32(*age);
    const auto num = parseDouble(*value);
    if (!isValidDev(*dev) || !boot_num || !seq_num || !age_num || !isValidName(*name) || !num) {
        return std::nullopt;
    }

    return Message{*dev, *boot_num, *seq_num, *age_num, *name, *num};
}
}  // namespace metric
