# 🛡️ RNS-C Compliance & Audit Matrix

**Firmware Version:** 0.3.0-RC1 (33C3 Edition)
**Target Hardware:** Heltec WiFi LoRa 32 V3 (ESP32-S3)
**Spec Reference:** [Reticulum Network Stack Manual v1.1.3](https://reticulum.network/manual/)

## 1. Cryptography & Identity (The Iron Core)
*Ref: Manual v1.1.3, Chapter 6 (Identities)*

We employ a **vendored** implementation of `Monocypher 4.0.2` to ensure binary reproducibility and strictly enforce the primitives defined in the RNS spec.

| Component | RNS Spec v1.1.3 | RNS-C Implementation | Status |
| :--- | :--- | :--- | :--- |
| **Identity Generation** | Ed25519 Public Key derived from 32-byte seed. | `lib/Monocypher`: `crypto_ed25519_key_pair` used on a persisted 32-byte random seed. | ✅ **STRICT** |
| **Addressing** | SHA-256 hash of Public Key (truncated to 16 bytes). | `RetiIdentity.h`: `sha256(pub_key)[0..16]`. Matches `rnsd` hashing exactly. | ✅ **MATCH** |
| **Signatures** | Ed25519 (Schnorr). | `monocypher-ed25519`: `crypto_ed25519_sign`. | ✅ **MATCH** |
| **Key Exchange** | X25519 (Curve25519 ECDH). | `monocypher`: `crypto_x25519`. Ephemeral keys generated per Link Request. | ✅ **MATCH** |
| **Encryption** | AES-128-CBC (PKCS7 Padding). | `mbedtls` hardware accelerated. Compatible with RNS default `Fernet` logic. | ✅ **MATCH** |

## 2. Interface Specifications
*Ref: Manual v1.1.3, Chapter 8 (Interfaces)*

### A. RNode LoRa Interface (PHY)
The firmware implements the raw `RNode` framing protocol to ensure interoperability with standard RNS Python nodes.

* **Header:** `[ 1 byte ]` $\to$ `(SequenceNumber << 4) | (Flags & 0x0F)`
* **MTU:** 500 Bytes (Strict enforcement).
* **Frequency:** 867.2 MHz (Default EU) / 915.0 MHz (US). Configurable via `config.json`.
* **Modulation:** SF7, BW 125kHz, CR 4/5 (The "Fast Mesh" profile).

### B. Transport & Routing
* **Announce Propagation:** Implements the jittered rebroadcast mechanism (200ms-1000ms delay) to prevent packet storms in the mesh.
* **Hop Limits:** Decrements TTL on all forwarded packets; drops if 0.

## 3. Supply Chain Security (Vendor Statement)
**Why Monocypher?**
We have explicitly **vendored** (embedded) the Monocypher v4.0.2 source code into `lib/Monocypher`. 
1.  **Auditability:** The code is committed to the repo. No "npm-style" dynamic fetching of security-critical code.
2.  **API Stability:** Protects against upstream API breaks (e.g., v3 vs v4 signature function changes).
3.  **Size:** Monocypher compiles to <40KB, fitting comfortably alongside the WiFi stack.

## 4. Field Verification Procedure

**Objective:** Prove interoperability with a Linux Laptop running the official `rnsd`.

**Setup:**
1.  **Laptop:** Install RNS (`pip install rns`). Run `rnsd -vv`.
2.  **Device:** Flash Heltec V3. Connect via USB.
3.  **Observation:**
    * `rnsd` log: `[SerialInterface] Connected to ...`
    * `rnsd` log: `[Transport] Interface SerialInterface is now active`
    * **Success Criteria:** The device's Identity Hash (printed on OLED/Serial) appears in the laptop's `Announce` table.