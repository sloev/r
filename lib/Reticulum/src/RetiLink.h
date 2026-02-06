#pragma once
#include "RetiCrypto.h"
#include "RetiPacket.h"
#include <time.h>

namespace Reticulum {
class Link {
public:
    bool active = false;
    std::vector<uint8_t> remote, encKey, authKey, reqHash, myPub, myPriv;

    Link(std::vector<uint8_t> r) : remote(r) { Crypto::genKeys(myPub, myPriv); }

    void accept(std::vector<uint8_t> peerPub, std::vector<uint8_t> hash) {
        reqHash = hash; 
        std::vector<uint8_t> s = Crypto::x25519_shared(myPriv, peerPub);
        std::vector<uint8_t> k = Crypto::hkdf(s, reqHash, 64);
        encKey.assign(k.begin(), k.begin()+32);
        authKey.assign(k.begin()+32, k.end());
        active = true;
    }

    Packet createProof(Identity& id); // Impl in Router to avoid circular dep

    Packet encrypt(std::vector<uint8_t> pl, uint8_t ctx=0) {
        if(!active) return Packet();
        
        // 1. IV
        std::vector<uint8_t> iv(16); 
        for(int i=0;i<16;i++) iv[i]=(uint8_t)esp_random();
        
        // 2. AES Body
        std::vector<uint8_t> in = {ctx}; 
        in.insert(in.end(), pl.begin(), pl.end());
        std::vector<uint8_t> c = Crypto::aes_encrypt(encKey, iv, in);

        // 3. Fernet Token Construction
        // [0x80] [TS(8)] [IV(16)] [Cipher] [HMAC]
        std::vector<uint8_t> d;
        d.push_back(0x80); // Version

        // Timestamp (8 bytes, Big Endian)
        time_t now;
        time(&now);
        // Valid RNS epoch check ( > 2023). If NTP failed, this is 0.
        uint64_t ts = (now > 1672531200) ? (uint64_t)now : 0;
        
        for(int i=7; i>=0; i--) d.push_back((ts >> (i*8)) & 0xFF);

        d.insert(d.end(), iv.begin(), iv.end());
        d.insert(d.end(), c.begin(), c.end());

        // 4. HMAC
        std::vector<uint8_t> m = Crypto::hmac_sha256(authKey, d);
        d.insert(d.end(), m.begin(), m.end());

        Packet p; 
        p.type=DATA; p.destType=LINK; p.addresses=remote; p.data=d;
        return p;
    }
    
    // Decrypt omitted for brevity (same as previous v0.1)
};
}