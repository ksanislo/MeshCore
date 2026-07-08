/*
 * AES.h — mbedtls-backed shim for rweather/Crypto AES classes
 *
 * MeshCore's Utils.cpp uses AESSmall128 from the rweather/Crypto Arduino library
 * for block-level AES encrypt/decrypt. MeshCore implements its own CTR mode on
 * top of these block operations.
 *
 * rweather/Crypto API used by MeshCore:
 *   AESSmall128 aes;
 *   aes.setKey(key, 16);
 *   aes.encryptBlock(output, input);
 *   aes.decryptBlock(output, input);
 *
 * Also provides AES128, AES256, AESSmall256 as aliases since some MeshCore
 * code paths may reference them.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "mbedtls/aes.h"

class AESCommon {
public:
    AESCommon() {
        mbedtls_aes_init(&_enc_ctx);
        mbedtls_aes_init(&_dec_ctx);
        _keyBits = 0;
    }

    virtual ~AESCommon() {
        mbedtls_aes_free(&_enc_ctx);
        mbedtls_aes_free(&_dec_ctx);
    }

    bool setKey(const uint8_t* key, size_t len) {
        // len is in bytes: 16=AES128, 24=AES192, 32=AES256
        _keyBits = len * 8;
        int ret1 = mbedtls_aes_setkey_enc(&_enc_ctx, key, _keyBits);
        int ret2 = mbedtls_aes_setkey_dec(&_dec_ctx, key, _keyBits);
        return (ret1 == 0 && ret2 == 0);
    }

    void encryptBlock(uint8_t* output, const uint8_t* input) {
        mbedtls_aes_crypt_ecb(&_enc_ctx, MBEDTLS_AES_ENCRYPT, input, output);
    }

    void decryptBlock(uint8_t* output, const uint8_t* input) {
        mbedtls_aes_crypt_ecb(&_dec_ctx, MBEDTLS_AES_DECRYPT, input, output);
    }

    size_t blockSize() const { return 16; }
    size_t keySize() const { return _keyBits / 8; }

    void clear() {
        mbedtls_aes_free(&_enc_ctx);
        mbedtls_aes_free(&_dec_ctx);
        mbedtls_aes_init(&_enc_ctx);
        mbedtls_aes_init(&_dec_ctx);
        _keyBits = 0;
    }

private:
    mbedtls_aes_context _enc_ctx;
    mbedtls_aes_context _dec_ctx;
    unsigned int _keyBits;
};

// rweather/Crypto provides multiple AES class names.
// MeshCore primarily uses AESSmall128 but may reference others.
// They all have the same interface — the "Small" variants in rweather
// use less RAM but are slower. With mbedtls (hardware AES on ESP32-P4)
// there's no distinction needed.

class AES128 : public AESCommon {
public:
    bool setKey(const uint8_t* key, size_t len) {
        (void)len;
        return AESCommon::setKey(key, 16);
    }
};

class AES256 : public AESCommon {
public:
    bool setKey(const uint8_t* key, size_t len) {
        (void)len;
        return AESCommon::setKey(key, 32);
    }
};

// "Small" variants — identical to full variants when backed by hardware AES
typedef AES128 AESSmall128;
typedef AES256 AESSmall256;

// AESTiny variants (even smaller RAM in rweather, encrypt-only)
// Provide as aliases in case MeshCore references them
class AESTiny128 : public AES128 {};
class AESTiny256 : public AES256 {};
