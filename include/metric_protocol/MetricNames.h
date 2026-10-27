#pragma once

#include <string_view>

namespace metric::names {
inline constexpr std::string_view kTemperatureC = "temperature_c";
inline constexpr std::string_view kHumidityPct = "humidity_pct";
inline constexpr std::string_view kTempAlert = "temp_alert";
inline constexpr std::string_view kDhtOk = "dht_ok";
}  // namespace metric::names

namespace metric::temp_alert {
inline constexpr double kTooCold = -1;
inline constexpr double kNormal = 0;
inline constexpr double kTooHot = 1;
}  // namespace metric::temp_alert