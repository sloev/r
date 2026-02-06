#pragma once
#include "RetiCrypto.h"
#include "RetiPacket.h"

namespace Reticulum {
class Link {
public:
    bool active = false;
    std::vector<uint8_t> remote, encKey, authKey, reqHash, myPub, myPriv;

    Link(std::vector<uint8_t> r) : remote(r) { Crypto::genKeys(myPub, myPriv); }

    // Strict Spec: HKDF Salt must be the Request Packet Hash
    void accept(std::vector<uint8_t> peerPub, std::vector<uint8_t> hash) {
        reqHash = hash; 
        std::vector<uint8_t> s = Crypto::x25519_shared(myPriv, peerPub);
        std::vector<uint8_t> k = Crypto::hkdf(s, reqHash, 64);
        encKey.assign(k.begin(), k.begin()+32);
        authKey.assign(k.begin()+32, k.end());
        active = true;
    }

    // Packet createProof(Identity& id); // Implemented in Router to avoid circular dep

    Packet encrypt(std::vector<uint8_t> pl, uint8_t ctx=0) {
        if(!active) return Packet();
        
        // 1. Generate IV
        std::vector<uint8_t> iv(16); 
        for(int i=0;i<16;i++) iv[i]=(uint8_t)esp_random();
        
        // 2. Prepare Plaintext (Context + Data)
        std::vector<uint8_t> in = {ctx}; 
        in.insert(in.end(), pl.begin(), pl.end());
        
        // 3. AES Encryption
        std::vector<uint8_t> c = Crypto::aes_encrypt(encKey, iv, in);

        // 4. Construct Fernet Token
        // Format: [0x80] [Timestamp(8)] [IV(16)] [Ciphertext(N)] [HMAC(32)]
        std::vector<uint8_t> d;
        d.reserve(1 + 8 + 16 + c.size() + 32);

        d.push_back(0x80); // Version

        // Timestamp (8 bytes, Big Endian)
        // Note: Without RTC, we send 0. RNS implementations usually allow wide/infinite TTL 
        // for mesh links, but the field must exist.
        uint64_t ts = 0; // millis() / 1000;
        for(int i=7; i>=0; i--) d.push_back((ts >> (i*8)) & 0xFF);

        // IV
        d.insert(d.end(), iv.begin(), iv.end());

        // Ciphertext
        d.insert(d.end(), c.begin(), c.end());

        // 5. Calculate HMAC (Covers Version + TS + IV + Cipher)
        std::vector<uint8_t> m = Crypto::hmac_sha256(authKey, d);

        // 6. Append HMAC
        d.insert(d.end(), m.begin(), m.end());

        Packet p; 
        p.type = DATA; 
        p.destType = LINK; 
        p.addresses = remote; 
        p.data = d;
        return p;
    }
    
    // Decrypt logic (updated for Fernet)
    // Returns {Context, Data}
    // Returns {0xFF, empty} on failure
    std::pair<uint8_t, std::vector<uint8_t>> decrypt(std::vector<uint8_t> d) {
        // Min Size: Ver(1) + TS(8) + IV(16) + Block(16) + HMAC(32) = 73 bytes
        if(d.size() < 73) return {0xFF, {}};
        
        if(d[0] != 0x80) return {0xFF, {}}; // Version mismatch

        // Extract HMAC (Last 32 bytes)
        size_t split = d.size() - 32;
        std::vector<uint8_t> mac(d.begin() + split, d.end());
        std::vector<uint8_t> header(d.begin(), d.begin() + split);

        // Verify HMAC
        std::vector<uint8_t> calc = Crypto::hmac_sha256(authKey, header);
        if(calc != mac) return {0xFF, {}};

        // Extract IV (Starts at index 9, length 16)
        std::vector<uint8_t> iv(d.begin() + 9, d.begin() + 25);
        
        // Extract Cipher (Starts at index 25, ends at split)
        std::vector<uint8_t> cipher(d.begin() + 25, d.begin() + split);

        // Decrypt
        std::vector<uint8_t> plain = Crypto::aes_decrypt(encKey, iv, cipher);
        if(plain.size() < 1) return {0xFF, {}};

        uint8_t ctx = plain[0];
        std::vector<uint8_t> payload(plain.begin() + 1, plain.end());
        return {ctx, payload};
    }
};
}