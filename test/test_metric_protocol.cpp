#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include <metric_protocol/MetricProtocol.h>

using metric::Metric;
using metric::Origin;

namespace {
    constexpr std::string_view k_dev = "20:9b:a9:61:15:6c";
    const Origin k_origin{k_dev, 0x3f2a91c4U};

    std::string serialize(std::string_view name, double value, uint32_t seq, uint32_t age_ms = 0,
                          Origin origin = k_origin) {
        std::array<char, metric::k_max_message_len> buf{};
        const size_t n =
                metric::serialize(origin, seq, age_ms, Metric{name, value}, buf.data(), buf.size());
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

TEST(MetricDev, AcceptsLowercaseMacWithColons) {
    EXPECT_TRUE(metric::isValidDev("20:9b:a9:61:15:6c"));
    EXPECT_TRUE(metric::isValidDev("00:00:00:00:00:00"));
    EXPECT_TRUE(metric::isValidDev("ff:ff:ff:ff:ff:ff"));
}

TEST(MetricDev, RejectsOtherSpellings) {
    EXPECT_FALSE(metric::isValidDev("20:9B:A9:61:15:6C"));
    EXPECT_FALSE(metric::isValidDev("209ba961156c"));
    EXPECT_FALSE(metric::isValidDev("20-9b-a9-61-15-6c"));
    EXPECT_FALSE(metric::isValidDev("20:9b:a9:61:15"));
    EXPECT_FALSE(metric::isValidDev("20:9b:a9:61:15:6c:00"));
    EXPECT_FALSE(metric::isValidDev("20:9b:a9:61:15:6g"));
    EXPECT_FALSE(metric::isValidDev("2:09b:a9:61:15:6c"));
    EXPECT_FALSE(metric::isValidDev(""));
}

TEST(MetricDev, FormatsBytesInOrder) {
    const auto s = metric::formatDev({0x20, 0x9b, 0xa9, 0x61, 0x15, 0x6c});
    EXPECT_EQ(std::string_view(s.data()), "20:9b:a9:61:15:6c");
    EXPECT_EQ(std::string_view(metric::formatDev({0, 0, 0, 0, 0, 0x0f}).data()), "00:00:00:00:00:0f");
    EXPECT_TRUE(metric::isValidDev(s.data()));
}


TEST(MetricSerialize, ExactWireFormat) {
    EXPECT_EQ(serialize("uptime_s", 123, 42, 1500),
              "ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=42 age_ms=1500 name=uptime_s "
              "value=123\n");
    EXPECT_EQ(serialize("temp_c", 23.5, 0),
              "ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=0 age_ms=0 name=temp_c value=23.5\n");
}

TEST(MetricSerialize, BootIsAlwaysEightHexDigits) {
    const std::string wire = serialize("x", 1, 0, 0, Origin{k_dev, 0x2aU});
    EXPECT_NE(wire.find(" boot=0000002a "), std::string::npos);
}

TEST(MetricSerialize, LargeIntegersKeepAllDigits) {
    EXPECT_EQ(serialize("x", 4294967295.0, 4294967295U, 4294967295U, Origin{k_dev, 0xffffffffU}),
              "ver=2 dev=20:9b:a9:61:15:6c boot=ffffffff seq=4294967295 age_ms=4294967295 name=x "
              "value=4294967295\n");
}

TEST(MetricSerialize, LongestMessageFits) {
    const std::string name(metric::k_max_name_len, 'a');
    const std::string wire = serialize(name, -1.234567891e-300, 4294967295U, 4294967295U,
                                       Origin{k_dev, 0xffffffffU});
    ASSERT_FALSE(wire.empty());
    EXPECT_LT(wire.size(), metric::k_max_message_len);
    EXPECT_TRUE(metric::parse(wire).has_value());
}

TEST(MetricSerialize, RejectsBadInput) {
    EXPECT_EQ(serialize("Bad Name", 1, 0), "");
    EXPECT_EQ(serialize("x", std::nan(""), 0), "");
    EXPECT_EQ(serialize("x", std::numeric_limits<double>::infinity(), 0), "");
    EXPECT_EQ(serialize("x", 1, 0, 0, Origin{"20:9B:A9:61:15:6C", 1}), "");
    EXPECT_EQ(serialize("x", 1, 0, 0, Origin{"", 1}), "");
}

TEST(MetricSerialize, RejectsTooSmallBuffer) {
    std::array<char, 40> small{};
    EXPECT_EQ(metric::serialize(k_origin, 1, 0, Metric{"uptime_s", 1}, small.data(), small.size()),
              0U);
}

TEST(MetricParse, RoundTripV2) {
    const std::string wire = serialize("free_heap_bytes", 251344, 9, 30000);
    const auto m = metric::parse(wire);
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->dev, k_dev);
    EXPECT_EQ(m->boot, 0x3f2a91c4U);
    EXPECT_EQ(m->seq, 9U);
    EXPECT_EQ(m->age_ms, 30000U);
    EXPECT_EQ(m->name, "free_heap_bytes");
    EXPECT_DOUBLE_EQ(m->value, 251344);
}

TEST(MetricParse, V2RejectsWrongStructure) {
    const std::string head = "ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4 ";
    EXPECT_TRUE(metric::parse(head + "seq=1 age_ms=0 name=a value=2").has_value());
    EXPECT_FALSE(metric::parse(head + "seq=1 name=a value=2").has_value());
    EXPECT_FALSE(metric::parse(head + "age_ms=0 seq=1 name=a value=2").has_value());
    EXPECT_FALSE(metric::parse(head + "seq=1 age_ms=0 name=a value=2 x=1").has_value());
    EXPECT_FALSE(metric::parse("ver=2 boot=3f2a91c4 seq=1 age_ms=0 name=a value=2").has_value());
}

TEST(MetricParse, V2RejectsBadFields) {
    const std::string tail = " seq=1 age_ms=0 name=a value=2";
    EXPECT_FALSE(metric::parse("ver=2 dev=20:9B:A9:61:15:6C boot=3f2a91c4" + tail).has_value());
    EXPECT_FALSE(metric::parse("ver=2 dev=20:9b:a9:61:15:6c boot=3F2A91C4" + tail).has_value());
    EXPECT_FALSE(metric::parse("ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c" + tail).has_value());
    EXPECT_FALSE(metric::parse("ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4z" + tail).has_value());
    EXPECT_FALSE(metric::parse("ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91g4" + tail).has_value());
    EXPECT_FALSE(
        metric::parse("ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=1 age_ms=4294967296 name=a "
            "value=2")
        .has_value());
}

TEST(MetricParse, RejectsOldVersion) {
    EXPECT_FALSE(metric::parse("ver=1 seq=42 name=uptime_s value=123\n").has_value());
    EXPECT_FALSE(
        metric::parse("ver=1 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=1 age_ms=0 name=a value=2")
        .has_value());
}

TEST(MetricParse, RejectsUnknownVersion) {
    EXPECT_FALSE(
        metric::parse("ver=3 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=1 age_ms=0 name=a value=2")
        .has_value());
    EXPECT_FALSE(
        metric::parse("ver=x dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=1 age_ms=0 name=a value=2")
        .has_value());
}

namespace {
    std::string line(std::string_view seq, std::string_view name, std::string_view value) {
        return "ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=" + std::string(seq) +
               " age_ms=0 name=" + std::string(name) + " value=" + std::string(value);
    }
}

TEST(MetricParse, AcceptsWithOrWithoutLineEnd) {
    EXPECT_TRUE(metric::parse(line("1", "a", "2")).has_value());
    EXPECT_TRUE(metric::parse(line("1", "a", "2") + "\n").has_value());
    EXPECT_TRUE(metric::parse(line("1", "a", "2") + "\r\n").has_value());
}

TEST(MetricParse, RejectsGarbage) {
    EXPECT_FALSE(metric::parse("").has_value());
    EXPECT_FALSE(metric::parse("hello").has_value());
    EXPECT_FALSE(metric::parse(line("1", "a", "2") + " ").has_value());
    EXPECT_FALSE(metric::parse(" " + line("1", "a", "2")).has_value());
    EXPECT_FALSE(metric::parse(line("", "a", "2")).has_value());
}

TEST(MetricParse, RejectsBadNumbers) {
    EXPECT_FALSE(metric::parse(line("-1", "a", "2")).has_value());
    EXPECT_FALSE(metric::parse(line("4294967296", "a", "2")).has_value());
    EXPECT_FALSE(metric::parse(line("1", "a", "abc")).has_value());
    EXPECT_FALSE(metric::parse(line("1", "a", "2x")).has_value());
    EXPECT_FALSE(metric::parse(line("1", "a", "nan")).has_value());
    EXPECT_FALSE(metric::parse(line("1", "a", "inf")).has_value());
}

TEST(MetricParse, RejectsBadNameAndOversizedInput) {
    EXPECT_FALSE(metric::parse(line("1", "Bad", "2")).has_value());
    EXPECT_FALSE(metric::parse(std::string(metric::k_max_message_len + 1, 'a')).has_value());
}
