#pragma once

// Signed lines: any line of the protocol (metrics or logs) with an HMAC-SHA256 suffix:
//   ver=2 dev=20:9b:a9:61:15:6c ... value=24.3 sig=<64 lowercase hex>
// The MAC covers every byte before " sig=" (the payload), exactly as sent. This library only
// formats and splits the suffix; computing the MAC is up to each side (mbedTLS on the node,
// OpenSSL on the server), so the library stays free of crypto dependencies.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sig {
inline constexpr std::size_t kMacLen = 32; // HMAC-SHA256 output
inline constexpr std::size_t kKeyLen = 32; // shared key, same size as the MAC
inline constexpr std::string_view kField = " sig=";
inline constexpr std::size_t kHexLen = kMacLen * 2;
// What a signature adds to a line: " sig=" + 64 hex digits.
inline constexpr std::size_t kSuffixLen = kField.size() + kHexLen;

using Mac = std::array<uint8_t, kMacLen>;
using Key = std::array<uint8_t, kKeyLen>;

struct Signed {
    std::string_view payload; // the line without " sig=..." and without the line end
    Mac mac;
};

// buf holds a payload of payload_len bytes (no '\n'). Writes " sig=<hex>\n" right after it.
// Returns the new length (with '\n'), 0 if it does not fit into cap.
[[nodiscard]] std::size_t appendSignature(
    char* buf,
    std::size_t payload_len,
    std::size_t cap,
    const Mac& mac
);

// "payload sig=<64 lowercase hex>" (a trailing "\n" or "\r\n" is ignored) -> payload and MAC.
// nullopt: no signature at the end, wrong length, or not lowercase hex.
[[nodiscard]] std::optional<Signed> splitSignature(std::string_view line);

// 64 lowercase hex digits -> 32 bytes. For the shared key in configs (`openssl rand -hex 32`).
[[nodiscard]] std::optional<Key> parseKey(std::string_view hex);

}  // namespace sig
