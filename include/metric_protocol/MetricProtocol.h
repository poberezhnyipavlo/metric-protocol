#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace metric {
  constexpr uint32_t k_version = 2;
  constexpr size_t k_max_name_len = 31;
  constexpr size_t k_dev_len = 17;
  constexpr size_t k_max_message_len = 160;

  struct Origin {
    std::string_view dev;
    uint32_t boot;
  };

  struct Metric {
    std::string_view name;
    double value;
  };

  struct Message {
    uint32_t version;
    std::optional<std::string_view> dev;
    std::optional<uint32_t> boot;
    uint32_t seq;
    std::optional<uint32_t> age_ms;
    std::string_view name;
    double value;
  };

  bool isValidName(std::string_view name);
  bool isValidDev(std::string_view dev);
  std::array<char, k_dev_len + 1> formatDev(const std::array<uint8_t, 6>& mac);
  size_t serialize(const Origin& origin, uint32_t seq, uint32_t age_ms, const Metric& m, char* buf, size_t cap);
  std::optional<Message> parse(std::string_view text);
}

