#pragma once
#include "RetiCrypto.h"
#include <LittleFS.h>
#include <vector>
#include <Arduino.h>

namespace Reticulum {

class Identity {
private:
    std::vector<uint8_t> seed;       // 32 bytes (Persisted to disk)
    std::vector<uint8_t> privateKey; // 64 bytes (Expanded Secret Key)
    std::vector<uint8_t> publicKey;  // 32 bytes (Public Key)
    std::vector<uint8_t> address;    // 16 bytes (RNS Address)

public:
    Identity() {
        if(LittleFS.exists("/id.key")) {
            File f = LittleFS.open("/id.key", "r");
            if(f && f.size() == 32) {
                seed.resize(32);
                f.read(seed.data(), 32);
                f.close();
                RNS_LOG("[ID] Loaded existing seed.");
            } else {
                RNS_LOG("[ID] Key file corrupt. Regenerating.");
                generate();
            }
        } else {
            RNS_LOG("[ID] No identity found. Generating new.");
            generate();
        }
        derive();
    }

    void generate() {
        seed.resize(32);
        // Use ESP32 Hardware RNG (True Random)
        for(int i=0; i<32; i++) seed[i] = (uint8_t)esp_random();
        
        File f = LittleFS.open("/id.key", "w");
        if(f) {
            f.write(seed.data(), 32);
            f.close();
        }
    }

    void derive() {
        // RNS Spec v1.1.3: Identity derivation
        publicKey.resize(32);
        privateKey.resize(64); 
        
        // 1. Expand 32-byte Seed -> 64-byte Ed25519 Secret + 32-byte Public
        // Using vendored Monocypher v4
        crypto_ed25519_key_pair(privateKey.data(), publicKey.data(), seed.data());
        
        // 2. Address = SHA256(PublicKey) truncated to 16 bytes
        std::vector<uint8_t> hash = Crypto::sha256(publicKey);
        address.assign(hash.begin(), hash.begin()+16);
    }

    // Sign data using Ed25519 (Schnorr)
    std::vector<uint8_t> sign(const std::vector<uint8_t>& msg) {
        std::vector<uint8_t> sig(64);
        // Uses the expanded 64-byte secret key
        crypto_ed25519_sign(sig.data(), privateKey.data(), msg.data(), msg.size());
        return sig;
    }
    
    std::vector<uint8_t> getAddress() const { return address; }
    std::vector<uint8_t> getPublicKey() const { return publicKey; }
};
}