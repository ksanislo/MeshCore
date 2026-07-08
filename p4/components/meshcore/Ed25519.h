/*
 * Ed25519.h — C++ wrapper around MeshCore's bundled ed25519 C library
 *
 * Identity.cpp uses Ed25519::verify() from the rweather/Crypto Arduino library.
 * This shim wraps the bundled C ed25519 library functions into the same
 * static class interface.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

extern "C" {
#include "ed_25519.h"
}

class Ed25519 {
public:
    static bool verify(const uint8_t signature[64], const uint8_t publicKey[32],
                       const void* message, size_t len) {
        return ed25519_verify(signature, (const uint8_t*)message, len, publicKey) == 1;
    }

    static void sign(uint8_t signature[64], const uint8_t privateKey[64],
                     const uint8_t publicKey[32], const void* message, size_t len) {
        ed25519_sign(signature, (const uint8_t*)message, len, publicKey, privateKey);
    }

    static void derivePublicKey(uint8_t publicKey[32], const uint8_t privateKey[64]) {
        ed25519_derive_pub(publicKey, privateKey);
    }
};