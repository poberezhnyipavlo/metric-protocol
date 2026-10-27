#include "metric_protocol/LogProtocol.h"

#include <algorithm>
#include <array>
#include <cstdio>

#include "FieldParser.h"
#include "metric_protocol/MetricProtocol.h"

using metric::detail::isDigit;
using metric::detail::isLower;
using metric::detail::trimLineEnd;
using metric::detail::parseHex32;
using metric::detail::parseUint32;

namespace {
constexpr std::string_view kTextKey = "text=";
}

std::string_view logproto::toString(Level level) {
    switch (level) {
        case Level::kError:
            return "error";
        case Level::kWarn:
            return "warn";
        case Level::kInfo:
            return "info";
    }

    return "error";
}

std::optional<logproto::Level> logproto::parseLevel(std::string_view s) {
    for (const Level level : {Level::kInfo, Level::kWarn, Level::kError}) {
        if (s == toString(level)) {
            return level;
        }
    }
    return std::nullopt;
}

bool logproto::isValidSrc(std::string_view src) {
    if (src.empty() || src.length() > kMaxSrcLen) {
        return false;
    }

    return std::all_of(src.begin(), src.end(), [](char c) {
        return isLower(c) || isDigit(c) || c == '_';
    });
}

std::size_t logproto::serialize(const Record& record, char* buf, std::size_t cap) {
    if (!isValidSrc(record.src)) {
        return 0;
    }

    const std::size_t text_len = std::min(record.text.size(), kMaxTextLen);
    std::array<char, kMaxTextLen> text{};

    std::transform(
        record.text.begin(),
        record.text.begin() + static_cast<std::ptrdiff_t>(text_len),
        text.begin(),
        [](char c) { return c == '\n' || c == '\r' ? ' ' : c; }
    );

    const std::string_view level = toString(record.level);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    const int written = std::snprintf(
        buf,
        cap,
        "ver=%lu dev=%.*s boot=%08lx seq=%lu age_ms=%lu level=%.*s src=%.*s text=%.*s\n",
        static_cast<unsigned long>(kVersion),
        static_cast<int>(record.dev.size()),
        record.dev.data(),
        static_cast<unsigned long>(record.boot),
        static_cast<unsigned long>(record.seq),
        static_cast<unsigned long>(record.age_ms),
        static_cast<int>(level.size()),
        level.data(),
        static_cast<int>(record.src.size()),
        record.src.data(),
        static_cast<int>(text_len),
        text.data()
    );

    if (written <= 0 || static_cast<std::size_t>(written) >= cap) {
        return 0;
    }

    return static_cast<std::size_t>(written);
}

std::optional<logproto::Record> logproto::parse(std::string_view line) {
    std::string_view rest = trimLineEnd(line);
    if (rest.empty() || rest.size() > kMaxLineLen) {
        return std::nullopt;
    }

    const auto ver = metric::detail::takeField(rest, "ver");
    const auto dev = metric::detail::takeField(rest, "dev");
    const auto boot = metric::detail::takeField(rest, "boot");
    const auto seq = metric::detail::takeField(rest, "seq");
    const auto age_ms = metric::detail::takeField(rest, "age_ms");
    const auto level = metric::detail::takeField(rest, "level");
    const auto src = metric::detail::takeField(rest, "src");

    if (!ver || !dev || !boot || !seq || !age_ms || !src || !level) {
        return std::nullopt;
    }

    if (rest.substr(0, kTextKey.size()) != kTextKey) {
        return std::nullopt;
    }

    const auto text = rest.substr(kTextKey.size());

    const auto version = parseUint32(*ver);
    if (!version || *version != kVersion) {
        return std::nullopt;
    }

    const auto boot_num = parseHex32(*boot);
    const auto seq_num = parseUint32(*seq);
    const auto age_num = parseUint32(*age_ms);
    const auto level_value = parseLevel(*level);
    const auto src_num = isValidSrc(*src);

    if (
        !metric::isValidDev(*dev)
        || !boot_num
        || !seq_num
        || !age_num
        || !src_num
        || !level_value
        || text.size() > kMaxTextLen
    ) {
        return std::nullopt;
    }

    return Record{*dev, *boot_num, *seq_num, *age_num, *level_value, *src, text};
}
