#include "metric_protocol/MetricProtocol.h"

#include <cmath>
#include <cstdio>
#include <algorithm>

namespace metric {
  namespace {
    bool isLower(char c) {
      return c >= 'a' && c <= 'z';
    }

    bool isDigit(char c) {
      return c >= '0' && c <= '9';
    }

    std::string_view trimLineEnd(std::string_view s) {
      while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.remove_suffix(1);
      }
      return s;
    }

    std::optional<std::string_view> takeField(std::string_view& rest, std::string_view key) {
      const size_t end = rest.find(' ');
      const std::string_view field = rest.substr(0, end);
      rest = (end == std::string_view::npos) ? std::string_view{} : rest.substr(end + 1);

      if (field.size() <= key.size() || field.substr(0, key.size()) != key || field[key.size()] != '=') {
        return std::nullopt;
      }

      return field.substr(key.size() + 1);
    }

    std::optional<uint32_t> parseUint32(std::string_view s) {
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
  }

  bool isValidName(std::string_view name) {
    if (name.empty() || name.size() > k_max_name_len || !isLower(name[0])) {
      return false;
    }
    return std::all_of(
      name.begin(),
      name.end(),
      [](char c) {
        return isLower(c) || isDigit(c) || c == '_';
      }
    );
  }

  size_t serialize(const Metric& m, uint32_t seq, char* buf, size_t cap) {
   if (!isValidName(m.name) || !std::isfinite(m.value) || buf == nullptr || cap == 0) {
     return 0;
   }

    const int n = std::snprintf(
      buf,
      cap,
      "ver=%lu seq=%lu name=%.*s value=%.10g\n",
      static_cast<unsigned long>(k_version),
      static_cast<unsigned long>(seq),
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
    if (rest.empty() || rest.size() > k_max_message_len) {
      return std::nullopt;
    }

    const auto ver = takeField(rest, "ver");
    const auto seq = takeField(rest, "seq");
    const auto name = takeField(rest, "name");
    const auto value = takeField(rest, "value");

    if (!ver || !seq || !name || !value || !rest.empty()) {
      return std::nullopt;
    }

    const auto version = parseUint32(*ver);
    if (!version || *version != k_version) {
      return std::nullopt;
    }

    const auto seqNum = parseUint32(*seq);
    const auto num = parseDouble(*value);
    if (!seqNum || !isValidName(*name) || !num) {
      return std::nullopt;
    }

    return Message{.seq = *seqNum, .name = *name, .value = *num};
  }
}
