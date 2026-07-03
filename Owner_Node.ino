/**
 * Project: CAN Bus Based Vehicle Anti-Theft System
 * File: Owner_Node.ino
 * Description: Remote pager. Receives GPS alerts via LoRa, sends Lock/Unlock commands.
 * Hardware: ESP32, LoRa module, 2x Push Buttons.
 */

#include <Arduino.h>
#include <LoRa.h>

// PIN DEFINITIONS
#define LORA_SS_PIN       18
#define LORA_RST_PIN      14
#define LORA_DIO0_PIN     26
#define BUTTON_LOCK_PIN   32
#define BUTTON_UNLOCK_PIN 33

// DEBOUNCE CONSTANTS
const unsigned long DEBOUNCE_DELAY = 300;
unsigned long lastLockTime = 0;
unsigned long lastUnlockTime = 0;

// FUNCTION PROTOTYPES
void setupLoRa();
void processLoRaReception();
void handleButtons();
void sendCommand(String cmd);

// SETUP ROUTINE
void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("\n--- Owner Node Initialization ---");

    pinMode(BUTTON_LOCK_PIN, INPUT_PULLUP);
    pinMode(BUTTON_UNLOCK_PIN, INPUT_PULLUP);

    setupLoRa();
    
    Serial.println("Owner Node Ready. Listening for alerts...");
    Serial.println("Press Lock(GPIO32) or Unlock(GPIO33) to send commands.");
}

// MAIN LOOP
void loop() {
    processLoRaReception();
    handleButtons();
}

// LORA INITIALIZATION
void setupLoRa() {
    LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
   
    if (!LoRa.begin(433E6)) {
        Serial.println("[ERROR] LoRa module not found. Check connections.");
        while (1) delay(100);
    }
    
    LoRa.setSyncWord(0xF3);
    Serial.println("[LoRa] Initialized successfully at 433MHz.");
}

// RECEPTION HANDLING
void processLoRaReception() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String incomingData = "";
        while (LoRa.available()) {
            incomingData += (char)LoRa.read();
        }
        
        Serial.println("\n====================================");
        Serial.println("    EMERGENCY ALERT RECEIVED!       ");
        Serial.println("====================================");
        Serial.print("Payload: ");
        Serial.println(incomingData);
        Serial.print("RSSI: ");
        Serial.println(LoRa.packetRssi());
        
        if (incomingData.startsWith("ALERT:")) {
            if (incomingData.indexOf("INTRUSION") != -1) {
                Serial.println(">> WARNING: UNAUTHORIZED DOOR/CABIN ENTRY <<");
            } 
            else if (incomingData.indexOf("IGNITION") != -1) {
                Serial.println(">> CRITICAL: UNAUTHORIZED IGNITION ATTEMPT <<");
            }

            int latIdx = incomingData.indexOf("LAT:");
            if (latIdx != -1) {
                String coordinates = incomingData.substring(latIdx);
                Serial.println("Vehicle Location: " + coordinates);
                Serial.println("Action Required: Check vehicle or press LOCK button.");
            }
        }
        Serial.println("====================================\n");
    }
}

// TRANSMISSION & BUTTON HANDLING
void handleButtons() {
    unsigned long currentMillis = millis();
   
    if (digitalRead(BUTTON_LOCK_PIN) == LOW) {
        if (currentMillis - lastLockTime > DEBOUNCE_DELAY) {
            Serial.println("[USER] Lock button pressed.");
            sendCommand("CMD:LOCK");
            lastLockTime = currentMillis;
        }
    }

    if (digitalRead(BUTTON_UNLOCK_PIN) == LOW) {
        if (currentMillis - lastUnlockTime > DEBOUNCE_DELAY) {
            Serial.println("[USER] Unlock button pressed.");
            sendCommand("CMD:UNLOCK");
            lastUnlockTime = currentMillis;
        }
    }
}

void sendCommand(String cmd) {
    Serial.print("[LoRa TX] Transmitting: ");
    Serial.println(cmd);
    
    LoRa.beginPacket();
    LoRa.print(cmd);
    LoRa.endPacket();
    
    Serial.println("[LoRa TX] Delivery complete.");
}
