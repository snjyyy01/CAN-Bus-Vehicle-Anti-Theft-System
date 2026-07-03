/**
 * Project: CAN Bus Based Vehicle Anti-Theft System
 * File: Vehicle_Node.ino
 * Description: Monitors IR and Ignition sensors, sends data via CAN bus.
 * Hardware: ESP32, MCP2551 CAN Transceiver, IR Sensor, Ignition Switch.
 */

#include <Arduino.h>
#include <ESP32-TWAI-CAN.h> 

// PIN DEFINITIONS
#define IR_SENSOR_PIN      4
#define IGNITION_PIN       5
#define CAN_TX_PIN         21
#define CAN_RX_PIN         22

// CAN BUS CONSTANTS
#define CAN_ID_INTRUSION   0x100
#define CAN_ID_IGNITION    0x101
#define CAN_ID_HEARTBEAT   0x102
#define CAN_BAUD_RATE      500000

// DEBOUNCE VARIABLES
const unsigned long DEBOUNCE_DELAY = 200; 

int lastIrState = HIGH; 
int currentIrState = HIGH;
unsigned long lastIrTime = 0;

int lastIgnitionState = LOW;
int currentIgnitionState = LOW;
unsigned long lastIgnitionTime = 0;

// Heartbeat timer
unsigned long lastHeartbeatTime = 0;
const unsigned long HEARTBEAT_INTERVAL = 5000;

// FUNCTION PROTOTYPES
void setupCAN();
void sendCANMessage(uint32_t id, uint8_t payload);
void readSensors();
void handleHeartbeat();

// SETUP ROUTINE
void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("\n--- Vehicle Node Initialization ---");

    pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
    pinMode(IGNITION_PIN, INPUT_PULLDOWN);
   
    setupCAN();
    
    Serial.println("Vehicle Node Ready.");
}

// MAIN LOOP
void loop() {
    readSensors();
    handleHeartbeat();
}

// SENSOR READING & LOGIC
void readSensors() {
    unsigned long currentMillis = millis();

    int irReading = digitalRead(IR_SENSOR_PIN);
    if (irReading != lastIrState) {
        lastIrTime = currentMillis;
    }
    
    if ((currentMillis - lastIrTime) > DEBOUNCE_DELAY) {
        if (irReading != currentIrState) {
            currentIrState = irReading;
            Serial.print("[SENSOR] IR State Changed: ");
            Serial.println(currentIrState == LOW ? "INTRUSION DETECTED" : "CLEAR");
            
            if (currentIrState == LOW) {
                sendCANMessage(CAN_ID_INTRUSION, 1);
            } else {
                sendCANMessage(CAN_ID_INTRUSION, 0);
            }
        }
    }
    lastIrState = irReading;

    int ignReading = digitalRead(IGNITION_PIN);
    if (ignReading != lastIgnitionState) {
        lastIgnitionTime = currentMillis;
    }

    if ((currentMillis - lastIgnitionTime) > DEBOUNCE_DELAY) {
        if (ignReading != currentIgnitionState) {
            currentIgnitionState = ignReading;
            Serial.print("[SENSOR] Ignition State Changed: ");
            Serial.println(currentIgnitionState == HIGH ? "ON" : "OFF");
            
            if (currentIgnitionState == HIGH) {
                sendCANMessage(CAN_ID_IGNITION, 1);
            } else {
                sendCANMessage(CAN_ID_IGNITION, 0);
            }
        }
    }
    lastIgnitionState = ignReading;
}

// CAN COMMUNICATION HELPERS
void setupCAN() {
    Serial.print("Initializing CAN Bus...");
    ESP32Can.setPins(CAN_TX_PIN, CAN_RX_PIN);
    ESP32Can.setRxQueueSize(5);
    ESP32Can.setTxQueueSize(5);
    ESP32Can.setSpeed(ESP32Can.ConvertSpeed(CAN_BAUD_RATE));
    
    if (ESP32Can.begin()) {
        Serial.println(" SUCCESS.");
    } else {
        Serial.println(" FAILED. Check wiring.");
        while(1) delay(1000); 
    }
}

void sendCANMessage(uint32_t id, uint8_t payload) {
    CanFrame txFrame;
    txFrame.identifier = id;
    txFrame.extd = 0;
    txFrame.data_length_code = 1;
    txFrame.data[0] = payload;

    if (ESP32Can.writeFrame(txFrame)) {
        Serial.printf("[CAN TX] ID: 0x%03X | Data: %d\n", id, payload);
    } else {
        Serial.println("[CAN TX] Failed to send frame.");
    }
}

// SYSTEM HEALTH
void handleHeartbeat() {
    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
        lastHeartbeatTime = millis();
        sendCANMessage(CAN_ID_HEARTBEAT, 0xFF);
        Serial.println("[SYS] Sent Heartbeat.");
    }
}
