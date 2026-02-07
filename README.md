# RNS-C: Embedded Reticulum Stack (Heltec V3)

![License](https://img.shields.io/badge/License-MIT-orange)

A bare-metal C++ implementation of the **Reticulum Network Stack (v1.1.3)** for ESP32-S3. 

> "Security is not a library you download. It's a process you own."

## 🏗️ Architecture
* **Kernel:** FreeRTOS (via Arduino Framework).
* **Cryptography:** **Vendored Monocypher 4.0.2** (Ed25519/X25519/Chacha20). No external crypto dependencies.
* **Storage:** LittleFS (Crash-safe persistence for Identities and Config).
* **Interfaces:** LoRa (SX1262), WiFi (UDP), ESP-NOW (Cluster), Serial (KISS).

## 🚀 Quick Start
1.  **Download:** Grab `firmware_merged.bin` from Releases.
2.  **Flash:** Use [Espressif Web Tool](https://espressif.github.io/esptool-js/).
    * Baud: `921600`
    * Offset `0x0000`: `firmware_merged.bin`
3.  **Join:** The device defaults to `867.2 MHz` (EU868).

## 🛠️ Compilation 
We do not rely on hidden registry magic for core components.

```bash
# 1. Clone the repo (contains vendored crypto)
cd rns-c

# 2. Build 
# This compiles the vendored lib/Monocypher sources directly.
pio run

# 3. Flash & Monitor
pio run -t upload -t monitor
```

## 🐛 Debugging & Panic Recovery
Serial Output: 115200 baud.

Panic Mode: If critical hardware (SPI/Flash) fails, the LED blinks rapidly (5Hz). The watchdog will reset the device after 10s to attempt self-recovery.


## Copyright:

* [./lib/Monocypher/](./lib/Monocypher) see license: [./MONOCYPHER_LICENSE.md](MONOCYPHER_LICENSE.md)