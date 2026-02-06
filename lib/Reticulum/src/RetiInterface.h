#pragma once
#include "RetiCommon.h"
#include <functional>
#include <map>

namespace Reticulum {

class Interface {
protected:
    struct FragState {
        unsigned long ts;
        std::vector<uint8_t> buffer;
    };
    // Map Sequence Number -> Fragment State
    std::map<uint8_t, FragState> reassemblyMap;

public:
    String name;
    size_t mtu;
    std::function<void(const std::vector<uint8_t>&, Interface*)> onPacket;

    Interface(String n, size_t m) : name(n), mtu(m) {}
    virtual void sendRaw(const std::vector<uint8_t>& data) = 0;

    void send(const std::vector<uint8_t>& packet) {
        // RNode Logic:
        // Small packets are sent raw.
        // Large packets (> MTU) are split into two frames with a 1-byte header.
        // Header: [ Seq (4 bits) | SplitFlag (4 bits) ]
        // We use Bit 0 as "Is First Fragment".
        
        if (packet.size() <= mtu) {
            sendRaw(packet);
        } else {
            // Split into 2 parts
            uint8_t seq = esp_random() & 0x0F;
            size_t splitPoint = mtu - 1; // Reserve 1 byte for header
            
            // Part 1: Header (Seq | 0x01) + Data
            std::vector<uint8_t> p1; 
            p1.push_back((seq << 4) | 0x01); // 0x01 = First Part
            p1.insert(p1.end(), packet.begin(), packet.begin() + splitPoint);
            sendRaw(p1);
            
            delay(25); // Allow airtime clearing
            
            // Part 2: Header (Seq | 0x00) + Data
            std::vector<uint8_t> p2;
            p2.push_back((seq << 4) | 0x00); // 0x00 = Last Part
            p2.insert(p2.end(), packet.begin() + splitPoint, packet.end());
            sendRaw(p2);
        }
    }

    void receive(const std::vector<uint8_t>& data) {
        if (data.empty()) return;
        
        // Check for Split Header
        // Heuristic: RNode splits are usually max-length. 
        // If we see a weird header on a short packet, assume it's just data.
        
        uint8_t header = data[0];
        uint8_t seq = (header >> 4) & 0x0F;
        bool isSplitStart = (header & 0x01) == 1;
        
        // RNode Logic: If Part 1, it must be exactly MTU sized (filled frame)
        if (isSplitStart && data.size() == mtu) {
            // Start Reassembly
            FragState& fs = reassemblyMap[seq];
            fs.ts = millis();
            fs.buffer.assign(data.begin() + 1, data.end());
        } 
        else if (!isSplitStart && reassemblyMap.count(seq)) {
            // Found Part 2 matching Sequence
            FragState& fs = reassemblyMap[seq];
            if (millis() - fs.ts < 3000) { // 3s Reassembly Timeout
                fs.buffer.insert(fs.buffer.end(), data.begin() + 1, data.end());
                // Packet Complete - Pass Up
                if (onPacket) onPacket(fs.buffer, this);
            }
            reassemblyMap.erase(seq);
        } else {
            // Standard Packet (Not part of a split we are tracking)
            if(onPacket) onPacket(data, this);
        }
    }
};
}