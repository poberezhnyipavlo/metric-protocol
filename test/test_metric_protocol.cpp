#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>

#include <metric_protocol/MetricProtocol.h>

using metric::Metric;

namespace {
std::string serialize(std::string_view name, double value, uint32_t seq) {
  std::array<char, metric::k_max_message_len> buf{};
  const size_t n = metric::serialize(Metric{name, value}, seq, buf.data(), buf.size());
  return std::string(buf.data(), n);
}
}


TEST(MetricName, AcceptsSnakeCase) {
  EXPECT_TRUE(metric::isValidName("uptime_s"));
  EXPECT_TRUE(metric::isValidName("rssi_dbm"));
  EXPECT_TRUE(metric::isValidName("t"));
  EXPECT_TRUE(metric::isValidName(std::string(metric::k_max_name_len, 'a')));
}

TEST(MetricName, RejectsEverythingElse) {
  EXPECT_FALSE(metric::isValidName(""));
  EXPECT_FALSE(metric::isValidName(std::string(metric::k_max_name_len + 1, 'a')));
  EXPECT_FALSE(metric::isValidName("Uptime"));
  EXPECT_FALSE(metric::isValidName("1st"));
  EXPECT_FALSE(metric::isValidName("_x"));
  EXPECT_FALSE(metric::isValidName("a b"));
  EXPECT_FALSE(metric::isValidName("a=b"));
  EXPECT_FALSE(metric::isValidName("temp\n"));
}

TEST(MetricSerialize, ExactWireFormat) {
  EXPECT_EQ(serialize("uptime_s", 123, 42), "ver=1 seq=42 name=uptime_s value=123\n");
  EXPECT_EQ(serialize("temp_c", 23.5, 0), "ver=1 seq=0 name=temp_c value=23.5\n");
  EXPECT_EQ(serialize("rssi_dbm", -58, 7), "ver=1 seq=7 name=rssi_dbm value=-58\n");
}

TEST(MetricSerialize, LargeIntegersKeepAllDigits) {
  EXPECT_EQ(serialize("x", 4294967295.0, 4294967295U),
            "ver=1 seq=4294967295 name=x value=4294967295\n");
}

TEST(MetricSerialize, RejectsBadInput) {
  EXPECT_EQ(serialize("Bad Name", 1, 0), "");
  EXPECT_EQ(serialize("x", std::nan(""), 0), "");
  EXPECT_EQ(serialize("x", std::numeric_limits<double>::infinity(), 0), "");
}

TEST(MetricSerialize, RejectsTooSmallBuffer) {
  std::array<char, 10> small{};
  EXPECT_EQ(metric::serialize(Metric{"uptime_s", 1}, 1, small.data(), small.size()), 0U);
}

TEST(MetricParse, RoundTrip) {
  const std::string wire = serialize("free_heap_bytes", 251344, 9);
  const auto m = metric::parse(wire);
  ASSERT_TRUE(m.has_value());
  EXPECT_EQ(m->seq, 9U);
  EXPECT_EQ(m->name, "free_heap_bytes");
  EXPECT_DOUBLE_EQ(m->value, 251344);
}

TEST(MetricParse, AcceptsWithOrWithoutLineEnd) {
  EXPECT_TRUE(metric::parse("ver=1 seq=1 name=a value=2").has_value());
  EXPECT_TRUE(metric::parse("ver=1 seq=1 name=a value=2\n").has_value());
  EXPECT_TRUE(metric::parse("ver=1 seq=1 name=a value=2\r\n").has_value());
}

TEST(MetricParse, RejectsWrongStructure) {
  EXPECT_FALSE(metric::parse("").has_value());
  EXPECT_FALSE(metric::parse("hello").has_value());
  EXPECT_FALSE(metric::parse("ver=1 seq=1 name=a").has_value());
  EXPECT_FALSE(metric::parse("ver=1 seq=1 name=a value=2 extra=3").has_value());
  EXPECT_FALSE(metric::parse("seq=1 ver=1 name=a value=2").has_value());
  EXPECT_FALSE(metric::parse("ver=1  seq=1 name=a value=2").has_value());
  EXPECT_FALSE(metric::parse("ver=1 seq= name=a value=2").has_value());
}

TEST(MetricParse, RejectsUnknownVersion) {
  EXPECT_FALSE(metric::parse("ver=2 seq=1 name=a value=2").has_value());
}

TEST(MetricParse, RejectsBadNumbers) {
  EXPECT_FALSE(metric::parse("ver=1 seq=-1 name=a value=2").has_value());
  EXPECT_FALSE(metric::parse("ver=1 seq=4294967296 name=a value=2").has_value());
  EXPECT_FALSE(metric::parse("ver=1 seq=1 name=a value=abc").has_value());
  EXPECT_FALSE(metric::parse("ver=1 seq=1 name=a value=2x").has_value());
  EXPECT_FALSE(metric::parse("ver=1 seq=1 name=a value=nan").has_value());
  EXPECT_FALSE(metric::parse("ver=1 seq=1 name=a value=inf").has_value());
}

TEST(MetricParse, RejectsBadNameAndOversizedInput) {
  EXPECT_FALSE(metric::parse("ver=1 seq=1 name=Bad value=2").has_value());
  EXPECT_FALSE(metric::parse(std::string(metric::k_max_message_len + 1, 'a')).has_value());
}

