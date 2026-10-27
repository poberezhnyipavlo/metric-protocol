#include <gtest/gtest.h>
#include <metric_protocol/LogProtocol.h>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace {

using logproto::Level;
using logproto::Record;

constexpr std::string_view kDev = "20:9b:a9:61:15:6c";

Record record(Level level, std::string_view src, std::string_view text) {
    return Record{kDev, 0x3f2a91c4, 7, 1500, level, src, text};
}

std::string line(const Record& r) {
    std::array<char, logproto::kMaxLineLen> buf{};
    const std::size_t len = logproto::serialize(r, buf.data(), buf.size());
    return std::string(buf.data(), len);
}

// std::string withField(std::string_view key, std::string_view value) {
//     std::string out =
//         "ver=1 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=7 age_ms=1500 level=warn src=dht "
//         "text=sensor stopped";
//     const std::string prefix = std::string(key) + "=";
//     const std::size_t start = out.find(prefix);
//     const std::size_t value_start = start + prefix.size();
//     const std::size_t end = key == "text" ? out.size() : out.find(' ', value_start);
//     out.replace(value_start, end - value_start, value);
//     return out;
// }

}  // namespace

TEST(LogSerialize, ExactWireFormat) {
    EXPECT_EQ(
        line(record(Level::kWarn, "dht", "sensor stopped answering")),
        "ver=1 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=7 age_ms=1500 level=warn src=dht "
        "text=sensor stopped answering\n"
    );
}

TEST(LogSerialize, LevelNames) {
    EXPECT_EQ(logproto::toString(Level::kInfo), "info");
    EXPECT_EQ(logproto::toString(Level::kWarn), "warn");
    EXPECT_EQ(logproto::toString(Level::kError), "error");
}

TEST(LogSerialize, BootIsAlwaysEightHexDigits) {
    Record r = record(Level::kInfo, "boot", "started");
    r.boot = 0x1a;
    EXPECT_NE(line(r).find(" boot=0000001a "), std::string::npos);
}

TEST(LogSerialize, LineBreaksInTextBecomeSpaces) {
    EXPECT_EQ(
        line(record(Level::kError, "app", "first\nsecond\r\nthird")),
        "ver=1 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=7 age_ms=1500 level=error src=app "
        "text=first second  third\n"
    );
}

TEST(LogSerialize, LongTextIsCut) {
    const std::string long_text(200, 'x');
    const std::string out = line(record(Level::kInfo, "app", long_text));
    const std::string expected_text = "text=" + std::string(logproto::kMaxTextLen, 'x') + "\n";
    ASSERT_GE(out.size(), expected_text.size());
    EXPECT_EQ(out.substr(out.size() - expected_text.size()), expected_text);
}

TEST(LogSerialize, EmptyTextIsAllowed) {
    const std::string out = line(record(Level::kInfo, "app", ""));
    EXPECT_EQ(out.substr(out.size() - 6), "text=\n");
}

TEST(LogSerialize, SrcRules) {
    EXPECT_TRUE(logproto::isValidSrc("dht"));
    EXPECT_TRUE(logproto::isValidSrc("net_tcp2"));
    EXPECT_FALSE(logproto::isValidSrc(""));
    EXPECT_FALSE(logproto::isValidSrc("Dht"));
    EXPECT_FALSE(logproto::isValidSrc("d ht"));
    EXPECT_FALSE(logproto::isValidSrc(std::string(logproto::kMaxSrcLen + 1, 'a')));
    EXPECT_EQ(line(record(Level::kInfo, "bad src", "x")), "");
}

TEST(LogSerialize, LongestLineFits) {
    const std::string long_text(200, 'x');  // must outlive r: Record holds string_views
    Record r = record(Level::kError, "abcdefghijklmno", long_text);
    r.seq = 4294967295U;
    r.age_ms = 4294967295U;
    std::array<char, logproto::kMaxLineLen> buf{};
    EXPECT_GT(logproto::serialize(r, buf.data(), buf.size()), 0U);
}

TEST(LogSerialize, RejectsTooSmallBuffer) {
    std::array<char, 40> small{};
    EXPECT_EQ(
        logproto::serialize(record(Level::kInfo, "app", "x"), small.data(), small.size()),
        0U
    );
}

TEST(LogSerialize, BufferMustAlsoHoldTheTerminator) {
    const Record r = record(Level::kInfo, "app", "x");
    const std::size_t len = line(r).size();
    std::array<char, logproto::kMaxLineLen> buf{};
    EXPECT_EQ(logproto::serialize(r, buf.data(), len), 0U);  // no room for '\0'
    EXPECT_EQ(logproto::serialize(r, buf.data(), len + 1), len);
}

//
// TEST(LogParse, LevelFromText) {
//     EXPECT_EQ(logproto::parseLevel("info"), Level::kInfo);
//     EXPECT_EQ(logproto::parseLevel("warn"), Level::kWarn);
//     EXPECT_EQ(logproto::parseLevel("error"), Level::kError);
//     EXPECT_FALSE(logproto::parseLevel("").has_value());
//     EXPECT_FALSE(logproto::parseLevel("Info").has_value());
//     EXPECT_FALSE(logproto::parseLevel("fatal").has_value());
//     EXPECT_FALSE(logproto::parseLevel("inf").has_value());
//     EXPECT_FALSE(logproto::parseLevel("warning").has_value());
// }
//
// TEST(LogParse, RoundTrip) {
//     const std::string wire = line(record(Level::kError, "boot", "started, reason=panic boot=3"));
//     const auto r = logproto::parse(wire);
//     ASSERT_TRUE(r.has_value());
//     EXPECT_EQ(r->dev, kDev);
//     EXPECT_EQ(r->boot, 0x3f2a91c4U);
//     EXPECT_EQ(r->seq, 7U);
//     EXPECT_EQ(r->age_ms, 1500U);
//     EXPECT_EQ(r->level, Level::kError);
//     EXPECT_EQ(r->src, "boot");
//     EXPECT_EQ(r->text, "started, reason=panic boot=3");  // spaces and '=' stay in text
// }
//
// TEST(LogParse, AcceptsWithOrWithoutLineEnd) {
//     const std::string base = withField("text", "x");
//     EXPECT_TRUE(logproto::parse(base).has_value());
//     EXPECT_TRUE(logproto::parse(base + "\n").has_value());
//     EXPECT_TRUE(logproto::parse(base + "\r\n").has_value());
// }
//
// TEST(LogParse, EmptyTextIsAllowed) {
//     const auto r = logproto::parse(withField("text", ""));
//     ASSERT_TRUE(r.has_value());
//     EXPECT_EQ(r->text, "");
// }
//
// TEST(LogParse, TextKeepsInnerAndTrailingSpaces) {
//     const auto r = logproto::parse(withField("text", "a  b "));
//     ASSERT_TRUE(r.has_value());
//     EXPECT_EQ(r->text, "a  b ");
// }
//
// TEST(LogParse, RejectsWrongStructure) {
//     const std::string head = "ver=1 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=7 age_ms=1500 ";
//     EXPECT_FALSE(logproto::parse(head + "level=warn text=x").has_value());           // no src
//     EXPECT_FALSE(logproto::parse(head + "src=dht level=warn text=x").has_value());   // wrong order
//     EXPECT_FALSE(logproto::parse(head + "level=warn src=dht").has_value());          // no text
//     EXPECT_FALSE(logproto::parse(head + "level=warn src=dht msg=x").has_value());    // other key
//     EXPECT_FALSE(logproto::parse(head + "level=warn src=dht  text=x").has_value());  // two spaces
// }
//
// TEST(LogParse, RejectsBadFields) {
//     EXPECT_FALSE(logproto::parse(withField("ver", "2")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("ver", "x")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("dev", "20:9B:A9:61:15:6C")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("boot", "3f2a91c")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("boot", "3F2A91C4")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("seq", "-1")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("seq", "4294967296")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("age_ms", "")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("level", "fatal")).has_value());
//     EXPECT_FALSE(logproto::parse(withField("src", "Bad")).has_value());
//     EXPECT_FALSE(
//         logproto::parse(withField("src", std::string(logproto::kMaxSrcLen + 1, 'a'))).has_value()
//     );
// }
//
// TEST(LogParse, TextLengthLimit) {
//     const std::string max_text(logproto::kMaxTextLen, 'x');
//     EXPECT_TRUE(logproto::parse(withField("text", max_text)).has_value());
//     EXPECT_FALSE(logproto::parse(withField("text", max_text + "x")).has_value());
// }
//
// TEST(LogParse, RejectsGarbage) {
//     EXPECT_FALSE(logproto::parse("").has_value());
//     EXPECT_FALSE(logproto::parse("hello").has_value());
//     EXPECT_FALSE(logproto::parse(" " + withField("text", "x")).has_value());
//     EXPECT_FALSE(logproto::parse(std::string(logproto::kMaxLineLen, 'a')).has_value());
// }
//
// TEST(LogParse, RejectsMetricLine) {
//     EXPECT_FALSE(
//         logproto::parse("ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=1 age_ms=0 name=a value=2")
//             .has_value()
//     );
// }