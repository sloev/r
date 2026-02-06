#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include "RetiInterface.h"

namespace Reticulum {

class ESPNowInterface : public Interface {
private:
    static ESPNowInterface* instance;
    
public:
    // ESP-NOW MTU is 250 bytes
    ESPNowInterface() : Interface("ESP-NOW", 250) { instance = this; }

    bool begin() {
        // ESP-NOW requires WiFi to be active (STA or AP).
        // Crucial: All nodes in a cluster MUST be on the same WiFi Channel.
        if (WiFi.getMode() == WIFI_MODE_NULL) {
            WiFi.mode(WIFI_STA);
        }

        if (esp_now_init() != ESP_OK) {
            RNS_ERR("ESP-NOW Init Failed");
            return false;
        }

        // Add Broadcast Peer (FF:FF:FF:FF:FF:FF)
        esp_now_peer_info_t peerInfo = {};
        memset(&peerInfo, 0, sizeof(peerInfo));
        for (int i = 0; i < 6; i++) peerInfo.peer_addr[i] = 0xFF;
        
        // Important: Use current WiFi channel (0 means "current")
        peerInfo.channel = 0; 
        peerInfo.encrypt = false; // We use RNS encryption payload
        
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            RNS_ERR("ESP-NOW Add Peer Failed");
            return false;
        }

        esp_now_register_recv_cb(onDataRecv);
        return true;
    }

    // Static trampoline for the ESP-IDF callback
    static void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
        if (instance) {
            std::vector<uint8_t> frame(incomingData, incomingData + len);
            instance->receive(frame);
        }
    }

    void sendRaw(const std::vector<uint8_t>& data) override {
        const uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        
        // ESP-NOW strictly drops packets > 250 bytes
        // Our Interface::send() handles fragmentation, so this is safe.
        if (data.size() > 250) return;

        esp_now_send(broadcastAddr, data.data(), data.size());
    }
};

// Instance pointer init
ESPNowInterface* ESPNowInterface::instance = nullptr;

}