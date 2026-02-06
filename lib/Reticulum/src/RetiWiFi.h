#pragma once
#include <WiFi.h>
#include <WiFiUdp.h>
#include "RetiInterface.h"
#include "RetiConfig.h"

namespace Reticulum {
class WiFiDriver : public Interface {
    WiFiUDP udp;
    bool active = false;
public:
    // MTU 1200 is standard for RNS UDP
    WiFiDriver() : Interface("WiFi_UDP", 1200) {}
    
    void begin(Config& cfg) {
        if(cfg.networks.empty()) return;
        WiFi.mode(WIFI_STA);
        for(auto& n : cfg.networks) WiFi.begin(n.ssid.c_str(), n.pass.c_str());
        
        // Start NTP to fix Fernet Timestamps
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        
        udp.begin(4242);
        active = true;
    }
    
    void sendRaw(const std::vector<uint8_t>& d) override {
        if(WiFi.status() == WL_CONNECTED && active) {
            udp.beginPacket(IPAddress(255,255,255,255), 4242);
            udp.write(d.data(), d.size());
            udp.endPacket();
        }
    }
    
    void loop() {
        if(WiFi.status() == WL_CONNECTED && active) {
            int len = udp.parsePacket();
            if(len) {
                std::vector<uint8_t> b(len);
                udp.read(b.data(), len);
                receive(b);
            }
        }
    }
};
}