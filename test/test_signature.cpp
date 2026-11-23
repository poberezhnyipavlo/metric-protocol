#include <gtest/gtest.h>
#include <metric_protocol/Signature.h>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kPayload =
    "ver=2 dev=20:9b:a9:61:15:6c boot=3f2a91c4 seq=42 age_ms=1500 name=temperature_c value=24.3";

// Bytes 0x00, 0x01, ..., 0x1f: every hex digit appears, both as high and low nibble.
sig::Mac countingMac() {
    sig::Mac mac{};
    for (std::size_t i = 0; i < mac.size(); ++i) {
        mac[i] = static_cast<uint8_t>(i);
    }
    return mac;
}

constexpr std::string_view kCountingHex =
    "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";

std::string signedLine(std::string_view payload, const sig::Mac& mac) {
    std::array<char, 300> buf{};
    payload.copy(buf.data(), payload.size());
    const std::size_t len = sig::appendSignature(buf.data(), payload.size(), buf.size(), mac);
    return std::string(buf.data(), len);
}

}  // namespace

TEST(Signature, AppendWritesHexAndLineEnd) {
    const std::string line = signedLine(kPayload, countingMac());
    EXPECT_EQ(line, std::string(kPayload) + " sig=" + std::string(kCountingHex) + "\n");
    EXPECT_EQ(line.size(), kPayload.size() + sig::kSuffixLen + 1);
}

TEST(Signature, AppendHighBytes) {
    sig::Mac mac{};
    mac.fill(0xff);
    const std::string line = signedLine("x=1", mac);
    EXPECT_EQ(line, "x=1 sig=" + std::string(64, 'f') + "\n");
}

TEST(Signature, AppendNeedsRoomForSuffixAndNewline) {
    std::array<char, 3 + sig::kSuffixLen + 1> buf{'x', '=', '1'};
    EXPECT_EQ(sig::appendSignature(buf.data(), 3, buf.size(), countingMac()), buf.size());
    EXPECT_EQ(sig::appendSignature(buf.data(), 3, buf.size() - 1, countingMac()), 0U);
    EXPECT_EQ(sig::appendSignature(buf.data(), 0, buf.size(), countingMac()), 0U);
    EXPECT_EQ(sig::appendSignature(nullptr, 3, buf.size(), countingMac()), 0U);
}

TEST(Signature, SplitIsInverseOfAppend) {
    // The line must outlive split: payload is a view into it.
    const std::string line = signedLine(kPayload, countingMac());
    const auto split = sig::splitSignature(line);
    ASSERT_TRUE(split.has_value());
    EXPECT_EQ(split->payload, kPayload);
    EXPECT_EQ(split->mac, countingMac());
}

TEST(Signature, SplitIgnoresLineEnd) {
    const std::string line = std::string(kPayload) + " sig=" + std::string(kCountingHex);
    for (const std::string& variant : {line, line + "\n", line + "\r\n"}) {
        const auto split = sig::splitSignature(variant);
        ASSERT_TRUE(split.has_value()) << variant;
        EXPECT_EQ(split->payload, kPayload);
    }
}

// A log text may itself contain " sig=": only the fixed-size suffix at the very end counts.
TEST(Signature, OnlyTheLastFieldIsTheSignature) {
    const std::string payload = "ver=1 ... text=user wrote sig=abc";
    const std::string line = signedLine(payload, countingMac());
    const auto split = sig::splitSignature(line);
    ASSERT_TRUE(split.has_value());
    EXPECT_EQ(split->payload, payload);
}

TEST(Signature, SplitRejectsUnsignedAndMalformed) {
    const std::string hex(kCountingHex);
    const std::string payload(kPayload);
    EXPECT_FALSE(sig::splitSignature(payload));                             // unsigned
    EXPECT_FALSE(sig::splitSignature(" sig=" + hex));                       // nothing signed
    EXPECT_FALSE(sig::splitSignature(payload + " sig=" + hex.substr(1)));   // 63 digits
    EXPECT_FALSE(sig::splitSignature(payload + " sig=" + hex + "0"));       // 65 digits
    EXPECT_FALSE(sig::splitSignature(payload + " sog=" + hex));             // wrong key
    EXPECT_FALSE(sig::splitSignature(payload + "  sig=" + hex.substr(1)));  // shifted
    std::string upper = payload + " sig=" + hex;
    upper.back() = 'F';
    EXPECT_FALSE(sig::splitSignature(upper));  // uppercase
    std::string not_hex = payload + " sig=" + hex;
    not_hex[not_hex.size() - 10] = 'g';
    EXPECT_FALSE(sig::splitSignature(not_hex));  // not hex
}

TEST(Signature, ParseKey) {
    const auto key = sig::parseKey(kCountingHex);
    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(*key, countingMac());
    EXPECT_FALSE(sig::parseKey(kCountingHex.substr(2)));
    EXPECT_FALSE(sig::parseKey(""));
    EXPECT_FALSE(sig::parseKey(std::string(63, 'a') + "G"));
}