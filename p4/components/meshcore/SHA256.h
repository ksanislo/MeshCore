/*
 * SHA256.h — mbedtls-backed shim for rweather/Crypto SHA256 class
 *
 * MeshCore's Packet.cpp and Utils.cpp use the rweather/Crypto Arduino library.
 * This provides the same class API using ESP-IDF's mbedtls (hardware-accelerated
 * on ESP32-P4).
 *
 * rweather/Crypto API used by MeshCore:
 *   SHA256 sha;
 *   sha.reset();
 *   sha.update(data, len);
 *   sha.finalize(hash, hashLen);
 *   sha.resetHMAC(key, keyLen);
 *   sha.finalizeHMAC(key, keyLen, hash, hashLen);
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "mbedtls/sha256.h"
#include "mbedtls/md.h"

class SHA256 {
public:
    SHA256() {
        mbedtls_sha256_init(&_ctx);
        reset();
    }

    ~SHA256() {
        mbedtls_sha256_free(&_ctx);
    }

    void reset() {
        mbedtls_sha256_free(&_ctx);
        mbedtls_sha256_init(&_ctx);
        mbedtls_sha256_starts(&_ctx, 0);  // 0 = SHA-256 (not SHA-224)
        _hmacMode = false;
    }

    void update(const void* data, size_t len) {
        mbedtls_sha256_update(&_ctx, (const uint8_t*)data, len);
    }

    void finalize(void* hash, size_t hashLen) {
        uint8_t full[32];
        mbedtls_sha256_finish(&_ctx, full);
        size_t copy = (hashLen < 32) ? hashLen : 32;
        memcpy(hash, full, copy);
    }

    // HMAC support — MeshCore uses this for packet authentication
    void resetHMAC(const void* key, size_t keyLen) {
        _hmacMode = true;
        _hmacKeyLen = (keyLen > 64) ? 64 : keyLen;
        memset(_hmacKey, 0, 64);
        memcpy(_hmacKey, key, _hmacKeyLen);

        // If key > block size, hash it first
        if (keyLen > 64) {
            uint8_t hashedKey[32];
            mbedtls_sha256_context tmpCtx;
            mbedtls_sha256_init(&tmpCtx);
            mbedtls_sha256_starts(&tmpCtx, 0);
            mbedtls_sha256_update(&tmpCtx, (const uint8_t*)key, keyLen);
            mbedtls_sha256_finish(&tmpCtx, hashedKey);
            mbedtls_sha256_free(&tmpCtx);
            memset(_hmacKey, 0, 64);
            memcpy(_hmacKey, hashedKey, 32);
            _hmacKeyLen = 32;
        }

        // Inner padding
        uint8_t iPad[64];
        for (int i = 0; i < 64; i++) iPad[i] = _hmacKey[i] ^ 0x36;

        mbedtls_sha256_free(&_ctx);
        mbedtls_sha256_init(&_ctx);
        mbedtls_sha256_starts(&_ctx, 0);
        mbedtls_sha256_update(&_ctx, iPad, 64);
    }

    void finalizeHMAC(const void* key, size_t keyLen, void* hash, size_t hashLen) {
        (void)key;  // key already stored from resetHMAC
        (void)keyLen;

        // Finish inner hash
        uint8_t innerHash[32];
        mbedtls_sha256_finish(&_ctx, innerHash);

        // Outer padding
        uint8_t oPad[64];
        for (int i = 0; i < 64; i++) oPad[i] = _hmacKey[i] ^ 0x5C;

        // Outer hash
        mbedtls_sha256_free(&_ctx);
        mbedtls_sha256_init(&_ctx);
        mbedtls_sha256_starts(&_ctx, 0);
        mbedtls_sha256_update(&_ctx, oPad, 64);
        mbedtls_sha256_update(&_ctx, innerHash, 32);

        uint8_t full[32];
        mbedtls_sha256_finish(&_ctx, full);
        size_t copy = (hashLen < 32) ? hashLen : 32;
        memcpy(hash, full, copy);
    }

    enum { HASH_SIZE = 32, BLOCK_SIZE = 64 };

    size_t hashSize() const { return 32; }
    size_t blockSize() const { return 64; }

private:
    mbedtls_sha256_context _ctx;
    bool _hmacMode;
    uint8_t _hmacKey[64];
    size_t _hmacKeyLen;
};
