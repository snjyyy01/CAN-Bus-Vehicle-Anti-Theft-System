/**
 * Project: CAN Bus Based Vehicle Anti-Theft System
 * File: Controller_Node.ino
 * Description: Master node. Handles Web Server, LoRa, GPS, CAN RX, and Relay.
 * Hardware: ESP32, MCP2551, LoRa, NEO-6M GPS, Relay Module.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <ESP32-TWAI-CAN.h>

// PIN DEFINITIONS
#define CAN_TX_PIN        21
#define CAN_RX_PIN        22
#define LORA_SS_PIN       18
#define LORA_RST_PIN      14
#define LORA_DIO0_PIN     26
#define GPS_RX_PIN        16
#define GPS_TX_PIN        17
#define RELAY_PIN         27

// CONSTANTS & GLOBALS
enum SystemMode { MODE_UNLOCK, MODE_PARKING, MODE_LOCK };
SystemMode currentMode = MODE_UNLOCK;

bool intrusionDetected = false;
bool ignitionAttempted = false;

WebServer server(80);
TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);

// HTML DASHBOARD TEMPLATE
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Vehicle Security</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #121212; color: #ffffff; text-align: center; margin: 0; padding: 20px; }
    h1 { color: #00d2ff; }
    .status-box { background-color: #1e1e1e; border-radius: 10px; padding: 20px; margin: 20px auto; width: 80%; max-width: 400px; font-size: 24px; font-weight: bold; border: 2px solid #333; }
    .btn { padding: 15px 30px; font-size: 18px; margin: 10px; border: none; border-radius: 8px; cursor: pointer; width: 80%; max-width: 300px; transition: 0.3s; color: white; font-weight: bold; }
    .btn-parking { background-color: #f39c12; }
    .btn-lock { background-color: #e74c3c; }
    .btn-unlock { background-color: #2ecc71; }
    .btn:active { transform: scale(0.95); }
  </style>
</head>
<body>
  <h1>Anti-Theft Dashboard</h1>
  <div class="status-box" id="status">Mode: LOADING...</div>
  <button class="btn btn-parking" onclick="sendCommand('parking')">PARKING MODE</button><br>
  <button class="btn btn-lock" onclick="sendCommand('lock')">LOCK ENGINE</button><br>
  <button class="btn btn-unlock" onclick="sendCommand('unlock')">UNLOCK ENGINE</button>
  <script>
    function fetchStatus() {
      fetch('/status').then(res => res.text()).then(data => {
        document.getElementById('status').innerText = 'Mode: ' + data;
      });
    }
    function sendCommand(cmd) {
      fetch('/' + cmd).then(res => res.text()).then(data => {
        document.getElementById('status').innerText = 'Mode: ' + data;
      });
    }
    window.onload = fetchStatus;
    setInterval(fetchStatus, 5000);
  </script>
</body>
</html>
)rawliteral";

// FUNCTION PROTOTYPES
void setupWiFi();
void setupLoRa();
void setupCAN();
void processGPS();
void processCAN();
void processLoRa();
void triggerAlert(String alertType);
void applyModeSecurity();

// SETUP ROUTINE
void setup() {
    Serial.begin(115200);
  
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); 

    GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    setupWiFi();
    setupCAN();
    setupLoRa();
    
    Serial.println("Controller Node Ready.");
}

// MAIN LOOP
void loop() {
    server.handleClient();
    processGPS();
    processCAN();
    processLoRa();
    applyModeSecurity();
}

// SUBSYSTEM INITIALIZATION
void setupWiFi() {
    WiFi.softAP("Vehicle_Security", "admin1234");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: "); Serial.println(IP);

    server.on("/", []() { server.send(200, "text/html", htmlPage); });
    server.on("/status", []() {
        if(currentMode == MODE_UNLOCK) server.send(200, "text/plain", "UNLOCKED");
        else if(currentMode == MODE_PARKING) server.send(200, "text/plain", "PARKING");
        else server.send(200, "text/plain", "LOCKED");
    });
    server.on("/parking", []() { currentMode = MODE_PARKING; server.send(200, "text/plain", "PARKING"); Serial.println("Mode -> PARKING"); });
    server.on("/lock", []() { currentMode = MODE_LOCK; server.send(200, "text/plain", "LOCKED"); Serial.println("Mode -> LOCKED"); });
    server.on("/unlock", []() { 
        currentMode = MODE_UNLOCK; 
        intrusionDetected = false; 
        ignitionAttempted = false;
        server.send(200, "text/plain", "UNLOCKED"); 
        Serial.println("Mode -> UNLOCKED"); 
    });

    server.begin();
}

void setupLoRa() {
    LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa init failed!");
        while (1) delay(100);
    }
    Serial.println("LoRa initialized.");
}

void setupCAN() {
    ESP32Can.setPins(CAN_TX_PIN, CAN_RX_PIN);
    ESP32Can.setRxQueueSize(10);
    ESP32Can.setTxQueueSize(5);
    ESP32Can.setSpeed(ESP32Can.ConvertSpeed(500000));
    ESP32Can.begin();
}

// PROCESSING FUNCTIONS
void processGPS() {
    while (GPS_Serial.available() > 0) {
        gps.encode(GPS_Serial.read());
    }
}

void processCAN() {
    CanFrame rxFrame;
    if (ESP32Can.readFrame(rxFrame, 0)) {
        if (rxFrame.identifier == 0x100 && rxFrame.data[0] == 1) {
            Serial.println("[CAN RX] Intrusion Alert from Vehicle Node");
            if (currentMode == MODE_PARKING && !intrusionDetected) {
                intrusionDetected = true;
                triggerAlert("INTRUSION");
            }
        }
        else if (rxFrame.identifier == 0x101 && rxFrame.data[0] == 1) {
            Serial.println("[CAN RX] Ignition Alert from Vehicle Node");
            if (currentMode == MODE_PARKING) {
                ignitionAttempted = true;
                triggerAlert("IGNITION");
                currentMode = MODE_LOCK; // Auto-lock on hotwire attempt
            }
        }
    }
}

void processLoRa() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String incoming = "";
        while (LoRa.available()) {
            incoming += (char)LoRa.read();
        }
        Serial.println("[LoRa RX] Command Received: " + incoming);
        
        if (incoming == "CMD:LOCK") {
            currentMode = MODE_LOCK;
            Serial.println("Remote command: System LOCKED.");
        } else if (incoming == "CMD:UNLOCK") {
            currentMode = MODE_UNLOCK;
            intrusionDetected = false;
            ignitionAttempted = false;
            Serial.println("Remote command: System UNLOCKED.");
        }
    }
}

// ALERTS AND HARDWARE CONTROL
void triggerAlert(String alertType) {
    String latStr = String(gps.location.isValid() ? gps.location.lat() : 0.0, 6);
    String lngStr = String(gps.location.isValid() ? gps.location.lng() : 0.0, 6);
    
    String payload = "ALERT:" + alertType + ",LAT:" + latStr + ",LON:" + lngStr;
    
    Serial.println("[LoRa TX] Sending: " + payload);
    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();
}

void applyModeSecurity() {
    if (currentMode == MODE_LOCK) {
        digitalWrite(RELAY_PIN, HIGH); 
    } else {
        digitalWrite(RELAY_PIN, LOW);  
    }
}
