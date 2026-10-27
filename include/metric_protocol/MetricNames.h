#pragma once

#include <string_view>

namespace metric::names {
inline constexpr std::string_view k_temperature_c = "temperature_c";
inline constexpr std::string_view k_humidity_pct = "humidity_pct";
inline constexpr std::string_view k_temp_alert = "temp_alert";
inline constexpr std::string_view k_dht_ok = "dht_ok";
}  // namespace metric::names

namespace metric::temp_alert {
inline constexpr double k_too_cold = -1;
inline constexpr double k_normal = 0;
inline constexpr double k_too_hot = 1;
}  // namespace metric::temp_alert