# RNS-C: Embedded Reticulum Node

![Status](https://img.shields.io/badge/Status-Beta%20v0.2-green) ![Platform](https://img.shields.io/badge/Platform-ESP32-blue)

A clean-room, C++ implementation of the **[Reticulum Network Stack](https://reticulum.network/)** for ESP32 microcontrollers. 

This firmware transforms a **Heltec WiFi LoRa 32 V3** into a feature-complete Reticulum node that bridges **LoRa**, **WiFi**, **BLE**, **ESP-NOW**, and **Serial/USB**.

## ✨ Key Features

* **Universal Bridge**: Transparently routes packets between all interfaces.
* **RNode Compatible**: Implements physical layer fragmentation to talk to official RNode hardware.
* **ESP-NOW Cluster**: Devices in the same room automatically form a high-speed, zero-config mesh.
* **Store & Forward**: Captures packets for offline destinations; auto-delivers when they return.
* **Flash Protection**: RAM Write-Back Cache prevents Flash wear.
* **Fernet Crypto**: Full handshake and encryption compliance (using NTP time sync).

## 🛠 Validation & Test Setup

To verify compliance, set up the following 3-node environment:

### 1. The Gateway (Node A)
* **Hardware**: Heltec V3
* **Config**: Flash with WiFi Credentials.
* **Role**: 
    1. Connects to WiFi and syncs time via NTP (Crucial for Crypto).
    2. Acts as the bridge between LoRa and the PC (via UDP).

### 2. The Remote (Node B)
* **Hardware**: Heltec V3 (or T-Beam)
* **Config**: Flash *without* WiFi credentials.
* **Role**: Pure LoRa node.
* **Test**: Send a large message (400+ bytes) from here. It will trigger the RNode fragmentation logic.

### 3. The Inspector (PC)
* **Software**: Official Reticulum (`pip install rns`).
* **Config**: Edit `~/.reticulum/config` to listen to Node A:
    ```ini
    [[Gateway_Connection]]
      type = UDPInterface
      listen_ip = 0.0.0.0
      listen_port = 4242
    ```
* **Test Command**:
    ```bash
    # Try to identify Node B over the mesh
    rnx --identify <NODE_B_DESTINATION_HASH>
    
    # Try to establish an Encrypted Link
    rnx --link <NODE_B_DESTINATION_HASH>
    ```

### ✅ Success Criteria
1.  **Identification**: If `rnx` sees Node B, basic routing and LoRa PHY are working.
2.  **Linking**: If `rnx` successfully establishes a link (Green status), your **Crypto Handshake** and **Fernet Timestamps** are perfectly compliant.
3.  **Throughput**: If you can send a 500-byte page, **RNode Fragmentation** is working.

## 🚀 Quick Start

1.  **Clone Repo**:
    ```bash
    git clone [https://github.com/sloev/rns-c.git](https://github.com/sloev/rns-c.git)
    cd rns-c
    ```
2.  **Build & Upload**:
    ```bash
    pio run -t upload
    ```
3.  **Configure WiFi** (Optional):
    * Hold the **BOOT** button on the Heltec device while powering on.
    * Connect to WiFi `RNS-Config-Node`.
    * Browse to `192.168.4.1` to set SSID/Pass.

## 📦 Dependencies

* `RadioLib` (LoRa)
* `Monocypher` (Ed25519/X25519)
* `ArduinoJson` (Config)
* `NimBLE-Arduino` (Sideband)

---
*Based on Reticulum Spec 0.7.x*