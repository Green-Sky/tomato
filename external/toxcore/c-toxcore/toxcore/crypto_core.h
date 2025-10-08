/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2025 The TokTok team.
 * Copyright © 2013 Tox project.
 */

/** @file
 * @brief Functions for the core crypto.
 *
 * @note This code has to be perfect. We don't mess around with encryption.
 */
#ifndef C_TOXCORE_TOXCORE_CRYPTO_CORE_H
#define C_TOXCORE_TOXCORE_CRYPTO_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attributes.h"
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The number of bytes in a signature.
 */
#define CRYPTO_SIGNATURE_SIZE          64

/**
 * @brief The number of bytes in a Tox public key used for signatures.
 */
#define CRYPTO_SIGN_PUBLIC_KEY_SIZE    32

/**
 * @brief The number of bytes in a Tox secret key used for signatures.
 */
#define CRYPTO_SIGN_SECRET_KEY_SIZE    64

/**
 * @brief The number of bytes in a Tox public key used for encryption.
 */
#define CRYPTO_PUBLIC_KEY_SIZE         32

/**
 * @brief The number of bytes in a Tox secret key used for encryption.
 */
#define CRYPTO_SECRET_KEY_SIZE         32

/**
 * @brief The number of bytes in a shared key computed from public and secret keys.
 */
#define CRYPTO_SHARED_KEY_SIZE         32

/**
 * @brief The number of bytes in a symmetric key.
 */
#define CRYPTO_SYMMETRIC_KEY_SIZE      CRYPTO_SHARED_KEY_SIZE

/**
 * @brief The number of bytes needed for the MAC (message authentication code) in an
 *   encrypted message.
 */
#define CRYPTO_MAC_SIZE                16

/**
 * @brief The number of bytes in a nonce used for encryption/decryption.
 */
#define CRYPTO_NONCE_SIZE              24

/**
 * @brief The number of bytes in a SHA256 hash.
 */
#define CRYPTO_SHA256_SIZE             32

/**
 * @brief The number of bytes in a SHA512 hash.
 */
#define CRYPTO_SHA512_SIZE             64

/** @brief Fill a byte array with random bytes.
 *
 * This is the key generator callback and as such must be a cryptographically
 * secure pseudo-random number generator (CSPRNG). The security of Tox heavily
 * depends on the security of this RNG.
 */
typedef void crypto_random_bytes_cb(void *_Nullable obj, uint8_t *_Nonnull bytes, size_t length);

/** @brief Generate a random integer between 0 and @p upper_bound.
 *
 * Should produce a uniform random distribution, but Tox security does not
 * depend on this being correct. In principle, it could even be a non-CSPRNG.
 */
typedef uint32_t crypto_random_uniform_cb(void *_Nullable obj, uint32_t upper_bound);

/** @brief Virtual function table for Random. */
typedef struct Random_Funcs {
    crypto_random_bytes_cb *_Nullable random_bytes;
    crypto_random_uniform_cb *_Nullable random_uniform;
} Random_Funcs;

/** @brief Random number generator object.
 *
 * Can be used by test code and fuzzers to make toxcore behave in specific
 * well-defined (non-random) ways. Production code ought to use libsodium's
 * CSPRNG and use `os_random` below.
 */
typedef struct Random {
    const Random_Funcs *_Nullable funcs;
    void *_Nullable obj;
} Random;

/** @brief System random number generator.
 *
 * Uses libsodium's CSPRNG (on Linux, `/dev/urandom`).
 */
const Random *_Nullable os_random(void);

/**
 * @brief The number of bytes in an encryption public key used by DHT group chats.
 */
#define ENC_PUBLIC_KEY_SIZE            CRYPTO_PUBLIC_KEY_SIZE

/**
 * @brief The number of bytes in an encryption secret key used by DHT group chats.
 */
#define ENC_SECRET_KEY_SIZE            CRYPTO_SECRET_KEY_SIZE

/**
 * @brief The number of bytes in a signature public key.
 */
#define SIG_PUBLIC_KEY_SIZE            CRYPTO_SIGN_PUBLIC_KEY_SIZE

/**
 * @brief The number of bytes in a signature secret key.
 */
#define SIG_SECRET_KEY_SIZE            CRYPTO_SIGN_SECRET_KEY_SIZE

/**
 * @brief The number of bytes in a DHT group chat public key identifier.
 */
#define CHAT_ID_SIZE                   SIG_PUBLIC_KEY_SIZE

/**
 * @brief The number of bytes in an extended public key used by DHT group chats.
 */
#define EXT_PUBLIC_KEY_SIZE            (ENC_PUBLIC_KEY_SIZE + SIG_PUBLIC_KEY_SIZE)

/**
 * @brief The number of bytes in an extended secret key used by DHT group chats.
 */
#define EXT_SECRET_KEY_SIZE            (ENC_SECRET_KEY_SIZE + SIG_SECRET_KEY_SIZE)

/**
 * @brief The number of bytes in an HMAC authenticator.
 */
#define CRYPTO_HMAC_SIZE               32

/**
 * @brief The number of bytes in an HMAC secret key.
 */
#define CRYPTO_HMAC_KEY_SIZE           32

/**
 * @brief A `bzero`-like function which won't be optimised away by the compiler.
 *
 * Some compilers will inline `bzero` or `memset` if they can prove that there
 * will be no reads to the written data. Use this function if you want to be
 * sure the memory is indeed zeroed.
 */
void crypto_memzero(void *_Nonnull data, size_t length);

/**
 * @brief Compute a SHA256 hash (32 bytes).
 *
 * @param[out] hash The SHA256 hash of @p data will be written to this byte array.
 */
void crypto_sha256(uint8_t hash[_Nonnull CRYPTO_SHA256_SIZE], const uint8_t *_Nonnull data, size_t length);

/**
 * @brief Compute a SHA512 hash (64 bytes).
 *
 * @param[out] hash The SHA512 hash of @p data will be written to this byte array.
 */
void crypto_sha512(uint8_t hash[_Nonnull CRYPTO_SHA512_SIZE], const uint8_t *_Nonnull data, size_t length);

/**
 * @brief Compute an HMAC authenticator (32 bytes).
 *
 * @param[out] auth Resulting authenticator.
 * @param key Secret key, as generated by `new_hmac_key()`.
 */
void crypto_hmac(uint8_t auth[_Nonnull CRYPTO_HMAC_SIZE], const uint8_t key[_Nonnull CRYPTO_HMAC_KEY_SIZE], const uint8_t *_Nonnull data, size_t length);

/**
 * @brief Verify an HMAC authenticator.
 */
bool crypto_hmac_verify(const uint8_t auth[_Nonnull CRYPTO_HMAC_SIZE], const uint8_t key[_Nonnull CRYPTO_HMAC_KEY_SIZE], const uint8_t *_Nonnull data, size_t length);

/**
 * @brief Compare 2 public keys of length @ref CRYPTO_PUBLIC_KEY_SIZE, not vulnerable to
 *   timing attacks.
 *
 * @retval true if both mem locations of length are equal
 * @retval false if they are not
 */
bool pk_equal(const uint8_t pk1[_Nonnull CRYPTO_PUBLIC_KEY_SIZE], const uint8_t pk2[_Nonnull CRYPTO_PUBLIC_KEY_SIZE]);

/**
 * @brief Copy a public key from `src` to `dest`.
 */
void pk_copy(uint8_t dest[_Nonnull CRYPTO_PUBLIC_KEY_SIZE], const uint8_t src[_Nonnull CRYPTO_PUBLIC_KEY_SIZE]);

/**
 * @brief Compare 2 SHA512 checksums of length CRYPTO_SHA512_SIZE, not vulnerable to
 *   timing attacks.
 *
 * @return true if both mem locations of length are equal, false if they are not.
 */
bool crypto_sha512_eq(const uint8_t cksum1[_Nonnull CRYPTO_SHA512_SIZE], const uint8_t cksum2[_Nonnull CRYPTO_SHA512_SIZE]);

/**
 * @brief Compare 2 SHA256 checksums of length CRYPTO_SHA256_SIZE, not vulnerable to
 *   timing attacks.
 *
 * @return true if both mem locations of length are equal, false if they are not.
 */
bool crypto_sha256_eq(const uint8_t cksum1[_Nonnull CRYPTO_SHA256_SIZE], const uint8_t cksum2[_Nonnull CRYPTO_SHA256_SIZE]);

/**
 * @brief Return a random 8 bit integer.
 */
uint8_t random_u08(const Random *_Nonnull rng);

/**
 * @brief Return a random 16 bit integer.
 */
uint16_t random_u16(const Random *_Nonnull rng);

/**
 * @brief Return a random 32 bit integer.
 */
uint32_t random_u32(const Random *_Nonnull rng);

/**
 * @brief Return a random 64 bit integer.
 */
uint64_t random_u64(const Random *_Nonnull rng);

/**
 * @brief Return a random 32 bit integer between 0 and upper_bound (excluded).
 *
 * This function guarantees a uniform distribution of possible outputs.
 */
uint32_t random_range_u32(const Random *_Nonnull rng, uint32_t upper_bound);

/**
 * @brief Cryptographically signs a message using the supplied secret key and puts the resulting signature
 *   in the supplied buffer.
 *
 * @param[out] signature The buffer for the resulting signature, which must have room for at
 *   least CRYPTO_SIGNATURE_SIZE bytes.
 * @param message The message being signed.
 * @param message_length The length in bytes of the message being signed.
 * @param secret_key The secret key used to create the signature. The key should be
 *   produced by either `create_extended_keypair` or the libsodium function `crypto_sign_keypair`.
 *
 * @retval true on success.
 */
bool crypto_signature_create(uint8_t signature[_Nonnull CRYPTO_SIGNATURE_SIZE], const uint8_t *_Nonnull message, uint64_t message_length, const uint8_t secret_key[_Nonnull SIG_SECRET_KEY_SIZE]);

/** @brief Verifies that the given signature was produced by a given message and public key.
 *
 * @param signature The signature we wish to verify.
 * @param message The message we wish to verify.
 * @param message_length The length of the message.
 * @param public_key The public key counterpart of the secret key that was used to
 *   create the signature.
 *
 * @retval true on success.
 */
bool crypto_signature_verify(const uint8_t signature[_Nonnull CRYPTO_SIGNATURE_SIZE], const uint8_t *_Nonnull message, uint64_t message_length,
                             const uint8_t public_key[_Nonnull SIG_PUBLIC_KEY_SIZE]);

/**
 * @brief Fill the given nonce with random bytes.
 */
void random_nonce(const Random *_Nonnull rng, uint8_t nonce[_Nonnull CRYPTO_NONCE_SIZE]);

/**
 * @brief Fill an array of bytes with random values.
 */
void random_bytes(const Random *_Nonnull rng, uint8_t *_Nonnull bytes, size_t length);

/**
 * @brief Check if a Tox public key CRYPTO_PUBLIC_KEY_SIZE is valid or not.
 *
 * This should only be used for input validation.
 *
 * @return false if it isn't, true if it is.
 */
bool public_key_valid(const uint8_t public_key[_Nonnull CRYPTO_PUBLIC_KEY_SIZE]);

typedef struct Extended_Public_Key {
    uint8_t enc[CRYPTO_PUBLIC_KEY_SIZE];
    uint8_t sig[CRYPTO_SIGN_PUBLIC_KEY_SIZE];
} Extended_Public_Key;

typedef struct Extended_Secret_Key {
    uint8_t enc[CRYPTO_SECRET_KEY_SIZE];
    uint8_t sig[CRYPTO_SIGN_SECRET_KEY_SIZE];
} Extended_Secret_Key;

/**
 * @brief Creates an extended keypair: curve25519 and ed25519 for encryption and signing
 *   respectively. The Encryption keys are derived from the signature keys.
 *
 * NOTE: This does *not* use Random, so any code using this will not be fuzzable.
 * TODO: Make it use Random.
 *
 * @param[out] pk The buffer where the public key will be stored. Must have room for EXT_PUBLIC_KEY_SIZE bytes.
 * @param[out] sk The buffer where the secret key will be stored. Must have room for EXT_SECRET_KEY_SIZE bytes.
 * @param rng The random number generator to use for the key generator seed.
 *
 * @retval true on success.
 */
bool create_extended_keypair(Extended_Public_Key *_Nonnull pk, Extended_Secret_Key *_Nonnull sk, const Random *_Nonnull rng);

/** Functions for groupchat extended keys */
const uint8_t *_Nonnull get_enc_key(const Extended_Public_Key *_Nonnull key);
const uint8_t *_Nonnull get_sig_pk(const Extended_Public_Key *_Nonnull key);
void set_sig_pk(Extended_Public_Key *_Nonnull key, const uint8_t *_Nonnull sig_pk);
const uint8_t *_Nonnull get_sig_sk(const Extended_Secret_Key *_Nonnull key);
const uint8_t *_Nonnull get_chat_id(const Extended_Public_Key *_Nonnull key);

/**
 * @brief Generate a new random keypair.
 *
 * Every call to this function is likely to generate a different keypair.
 */
int32_t crypto_new_keypair(const Random *_Nonnull rng, uint8_t public_key[_Nonnull CRYPTO_PUBLIC_KEY_SIZE], uint8_t secret_key[_Nonnull CRYPTO_SECRET_KEY_SIZE]);

/**
 * @brief Derive the public key from a given secret key.
 */
void crypto_derive_public_key(uint8_t public_key[_Nonnull CRYPTO_PUBLIC_KEY_SIZE], const uint8_t secret_key[_Nonnull CRYPTO_SECRET_KEY_SIZE]);

/**
 * @brief Encrypt message to send from secret key to public key.
 *
 * Encrypt plain text of the given length to encrypted of
 * `length + CRYPTO_MAC_SIZE` using the public key (@ref CRYPTO_PUBLIC_KEY_SIZE
 * bytes) of the receiver and the secret key of the sender and a
 * @ref CRYPTO_NONCE_SIZE byte nonce.
 *
 * @retval -1 if there was a problem.
 * @return length of encrypted data if everything was fine.
 */
int32_t encrypt_data(const Memory *_Nonnull mem, const uint8_t public_key[_Nonnull CRYPTO_PUBLIC_KEY_SIZE], const uint8_t secret_key[_Nonnull CRYPTO_SECRET_KEY_SIZE],
                     const uint8_t nonce[_Nonnull CRYPTO_NONCE_SIZE], const uint8_t *_Nonnull plain, size_t length, uint8_t *_Nonnull encrypted);

/**
 * @brief Decrypt message from public key to secret key.
 *
 * Decrypt encrypted text of the given @p length to plain text of the given
 * `length - CRYPTO_MAC_SIZE` using the public key (@ref CRYPTO_PUBLIC_KEY_SIZE
 * bytes) of the sender, the secret key of the receiver and a
 * @ref CRYPTO_NONCE_SIZE byte nonce.
 *
 * @retval -1 if there was a problem (decryption failed).
 * @return length of plain text data if everything was fine.
 */
int32_t decrypt_data(const Memory *_Nonnull mem, const uint8_t public_key[_Nonnull CRYPTO_PUBLIC_KEY_SIZE], const uint8_t secret_key[_Nonnull CRYPTO_SECRET_KEY_SIZE],
                     const uint8_t nonce[_Nonnull CRYPTO_NONCE_SIZE], const uint8_t *_Nonnull encrypted, size_t length, uint8_t *_Nonnull plain);

/**
 * @brief Fast encrypt/decrypt operations.
 *
 * Use if this is not a one-time communication. `encrypt_precompute` does the
 * shared-key generation once so it does not have to be performed on every
 * encrypt/decrypt.
 */
int32_t encrypt_precompute(const uint8_t public_key[_Nonnull CRYPTO_PUBLIC_KEY_SIZE], const uint8_t secret_key[_Nonnull CRYPTO_SECRET_KEY_SIZE],
                           uint8_t shared_key[_Nonnull CRYPTO_SHARED_KEY_SIZE]);

/**
 * @brief Encrypt message with precomputed shared key.
 *
 * Encrypts plain of length length to encrypted of length + @ref CRYPTO_MAC_SIZE
 * using a shared key @ref CRYPTO_SYMMETRIC_KEY_SIZE big and a @ref CRYPTO_NONCE_SIZE
 * byte nonce.
 *
 * @retval -1 if there was a problem.
 * @return length of encrypted data if everything was fine.
 */
int32_t encrypt_data_symmetric(const Memory *_Nonnull mem, const uint8_t shared_key[_Nonnull CRYPTO_SHARED_KEY_SIZE], const uint8_t nonce[_Nonnull CRYPTO_NONCE_SIZE],
                               const uint8_t *_Nonnull plain, size_t length, uint8_t *_Nonnull encrypted);

/**
 * @brief Decrypt message with precomputed shared key.
 *
 * Decrypts encrypted of length length to plain of length
 * `length - CRYPTO_MAC_SIZE` using a shared key @ref CRYPTO_SYMMETRIC_KEY_SIZE
 * big and a @ref CRYPTO_NONCE_SIZE byte nonce.
 *
 * @retval -1 if there was a problem (decryption failed).
 * @return length of plain data if everything was fine.
 */
int32_t decrypt_data_symmetric(const Memory *_Nonnull mem, const uint8_t shared_key[_Nonnull CRYPTO_SHARED_KEY_SIZE], const uint8_t nonce[_Nonnull CRYPTO_NONCE_SIZE],
                               const uint8_t *_Nonnull encrypted, size_t length, uint8_t *_Nonnull plain);

/**
 * @brief Increment the given nonce by 1 in big endian (rightmost byte incremented first).
 */
void increment_nonce(uint8_t nonce[_Nonnull CRYPTO_NONCE_SIZE]);

/**
 * @brief Increment the given nonce by a given number.
 *
 * The number should be in host byte order.
 */
void increment_nonce_number(uint8_t nonce[_Nonnull CRYPTO_NONCE_SIZE], uint32_t increment);

/**
 * @brief Fill a key @ref CRYPTO_SYMMETRIC_KEY_SIZE big with random bytes.
 */
void new_symmetric_key(const Random *_Nonnull rng, uint8_t key[_Nonnull CRYPTO_SYMMETRIC_KEY_SIZE]);

/**
 * @brief Locks `length` bytes of memory pointed to by `data`.
 *
 * This will attempt to prevent the specified memory region from being swapped
 * to disk.
 *
 * @return true on success.
 */
bool crypto_memlock(void *_Nonnull data, size_t length);

/**
 * @brief Unlocks `length` bytes of memory pointed to by `data`.
 *
 * This allows the specified memory region to be swapped to disk.
 *
 * This function call has the side effect of zeroing the specified memory region
 * whether or not it succeeds. Therefore it should only be used once the memory
 * is no longer in use.
 *
 * @return true on success.
 */
bool crypto_memunlock(void *_Nonnull data, size_t length);

/**
 * @brief Generate a random secret HMAC key.
 */
void new_hmac_key(const Random *_Nonnull rng, uint8_t key[_Nonnull CRYPTO_HMAC_KEY_SIZE]);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_CRYPTO_CORE_H */
