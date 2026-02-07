/**
 * RNS-C Firmware | 33C3 Edition
 * Compliant with Reticulum Manual v1.1.3
 */
#include <Arduino.h>
#include <LittleFS.h>
#include <SPI.h>
#include "Reti.h"

// --- Hardware Definitions (Heltec V3) ---
#define PIN_LORA_SCK  10
#define PIN_LORA_MISO 11
#define PIN_LORA_MOSI 13
#define PIN_LORA_NSS  8
#define PIN_LORA_RST  12
#define PIN_LORA_DIO1 14
#define PIN_LED       35

// --- Globals ---
Reticulum::Identity* id = nullptr;
Reticulum::Config config;

// Interface Instances
Reticulum::LoRaInterface* lora = nullptr;
Reticulum::SerialInterface* usb = nullptr;
Reticulum::WiFiDriver* wifi = nullptr;
Reticulum::ESPNowInterface* espnow = nullptr;

// ISR: Trampoline to class method
void IRAM_ATTR isr_lora() { 
    if(lora) lora->setFlag(); 
}

void setup() {
    // 1. Bootstrapping
    Serial.begin(115200);
    
    // Wait for Serial (Max 2s) so we don't hang headless boots
    unsigned long boot_start = millis();
    while(!Serial && (millis() - boot_start < 2000));

    Serial.println("\n\n>>> RNS-C Firmware Booting (Spec v1.1.3) <<<");

    // 2. Filesystem (Persistence Layer)
    if(!LittleFS.begin(true)) {
        Serial.println("[CRIT] Filesystem Mount Failed! Config/Identity lost.");
        // We continue anyway, running in ephemeral mode
    }
    config.load();

    // 3. Identity Core (Vendored Monocypher)
    id = new Reticulum::Identity();
    Serial.print("[CORE] Identity Loaded: ");
    for(auto b : id->getAddress()) Serial.printf("%02x", b);
    Serial.println();

    // 4. LoRa Interface (SX1262)
    SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
    lora = new Reticulum::LoRaInterface(&radio); // radio instance from RadioLib HAL
    
    // CRITICAL FIX: Do not dead-loop on radio failure. 
    // The device should still function as a WiFi/Serial node.
    if(!lora->begin(config.loraFreq)) {
        Serial.println("[ERR] LoRa Radio Init Failed! Check Hardware.");
    } else {
        lora->start(isr_lora);
        Serial.printf("[PHY] LoRa Active @ %.1f MHz (SF7/BW125)\n", config.loraFreq);
    }

    // 5. Auxiliary Interfaces
    usb = new Reticulum::SerialInterface(&Serial);
    
    // WiFi & Cluster Mode
    if(config.wifiEnabled) {
        wifi = new Reticulum::WiFiDriver();
        wifi->begin(config);
        
        espnow = new Reticulum::ESPNowInterface();
        espnow->begin();
        Serial.println("[PHY] WiFi & ESP-NOW Active");
    }

    // 6. Status LED
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH); 
}

void loop() {
    // The Reactor Loop
    // Polls all interfaces for incoming frames and routes them
    
    if(lora) lora->loop();
    if(usb) usb->loop();
    if(wifi) wifi->loop();
    if(espnow) espnow->loop();
    
    // Tiny yield for FreeRTOS scheduler stability
    vTaskDelay(1); 
}