#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string_view>

namespace metric {
  constexpr uint32_t k_version = 1;
  constexpr size_t k_max_name_len = 31;
  constexpr size_t k_max_message_len = 96;

  struct Metric {
    std::string_view name;
    double value;
  };

  struct Message {
    uint32_t seq;
    std::string_view name;
    double value;
  };

  bool isValidName(std::string_view name);
  size_t serialize(const Metric& m, uint32_t seq, char* buf, size_t cap);
  std::optional<Message> parse(std::string_view text);
}

