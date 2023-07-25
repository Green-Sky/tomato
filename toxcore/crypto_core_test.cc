#include "crypto_core.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <vector>

#include "util.h"

namespace {

using HmacKey = std::array<uint8_t, CRYPTO_HMAC_KEY_SIZE>;
using Hmac = std::array<uint8_t, CRYPTO_HMAC_SIZE>;
using ExtPublicKey = std::array<uint8_t, EXT_PUBLIC_KEY_SIZE>;
using ExtSecretKey = std::array<uint8_t, EXT_SECRET_KEY_SIZE>;
using Signature = std::array<uint8_t, CRYPTO_SIGNATURE_SIZE>;
using Nonce = std::array<uint8_t, CRYPTO_NONCE_SIZE>;

TEST(CryptoCore, IncrementNonce)
{
    Nonce nonce{};
    increment_nonce(nonce.data());
    EXPECT_EQ(
        nonce, (Nonce{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}}));

    for (int i = 0; i < 0x1F4; ++i) {
        increment_nonce(nonce.data());
    }

    EXPECT_EQ(nonce,
        (Nonce{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0xF5}}));
}

TEST(CryptoCore, IncrementNonceNumber)
{
    Nonce nonce{};

    increment_nonce_number(nonce.data(), 0x1F5);
    EXPECT_EQ(nonce,
        (Nonce{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0xF5}}));

    increment_nonce_number(nonce.data(), 0x1F5);
    EXPECT_EQ(nonce,
        (Nonce{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x03, 0xEA}}));

    increment_nonce_number(nonce.data(), 0x12345678);
    EXPECT_EQ(nonce,
        (Nonce{
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x12, 0x34, 0x5A, 0x62}}));
}

TEST(CryptoCore, Signatures)
{
    const Random *rng = system_random();
    ASSERT_NE(rng, nullptr);

    ExtPublicKey pk;
    ExtSecretKey sk;

    EXPECT_TRUE(create_extended_keypair(pk.data(), sk.data()));

    std::vector<uint8_t> message;

    // Try a few different sizes, including empty 0 length message.
    for (uint8_t i = 0; i < 100; ++i) {
        Signature signature;
        EXPECT_TRUE(crypto_signature_create(
            signature.data(), message.data(), message.size(), get_sig_sk(sk.data())));
        EXPECT_TRUE(crypto_signature_verify(
            signature.data(), message.data(), message.size(), get_sig_pk(pk.data())));

        message.push_back(random_u08(rng));
    }
}

TEST(CryptoCore, Hmac)
{
    const Random *rng = system_random();
    ASSERT_NE(rng, nullptr);

    HmacKey sk;
    new_hmac_key(rng, sk.data());

    std::vector<uint8_t> message;

    // Try a few different sizes, including empty 0 length message.
    for (uint8_t i = 0; i < 100; ++i) {
        Hmac auth;
        crypto_hmac(auth.data(), sk.data(), message.data(), message.size());
        EXPECT_TRUE(crypto_hmac_verify(auth.data(), sk.data(), message.data(), message.size()));

        message.push_back(random_u08(rng));
    }
}

}  // namespace
