#include "core/Hash.hpp"

#include <openssl/evp.h>
#include <stdexcept>

namespace vanta::hash {

Sha256Digest sha256(const void* data, std::size_t len) {
    Sha256Digest out{};
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
    unsigned int sz = 0;
    bool ok =
        EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(ctx, data, len) == 1 &&
        EVP_DigestFinal_ex(ctx, out.data(), &sz) == 1 &&
        sz == out.size();
    EVP_MD_CTX_free(ctx);
    if (!ok) throw std::runtime_error("SHA-256 digest failed");
    return out;
}

static constexpr char hex_chars[] = "0123456789abcdef";

std::string to_hex(const Sha256Digest& d) {
    std::string s(d.size() * 2, '0');
    for (std::size_t i = 0; i < d.size(); ++i) {
        s[i * 2]     = hex_chars[(d[i] >> 4) & 0xF];
        s[i * 2 + 1] = hex_chars[d[i] & 0xF];
    }
    return s;
}

} // namespace vanta::hash
