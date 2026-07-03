CAN Bus Based Vehicle Anti-Theft System with Engine Lockdown and GPS Alert
Overview
This project presents a smart vehicle anti-theft system that combines CAN Bus communication, ESP32 microcontrollers, LoRa wireless communication, GPS tracking, and an engine lockdown mechanism to improve vehicle security.

The system detects unauthorized access using an IR sensor, communicates events through a CAN Bus network, alerts the user through a web interface, tracks the vehicle using GPS, and prevents unauthorized engine operation using a relay module.

Features
CAN Bus communication between ESP32 nodes
Intrusion detection using an IR sensor
Engine lockdown using a relay module
GPS-based vehicle tracking
LoRa long-range communication
Web-based monitoring and control interface
Parking, Lock, and Unlock modes
Hardware Used
ESP32 (3)
MCP2551 CAN Transceiver (2)
LoRa Module
GPS NEO-6M
IR Sensor
Relay Module
Buzzer
Software Used
Arduino IDE
Embedded C
ESP32 Wi-Fi Web Server
System Architecture
(Add your block diagram here)

Circuit Diagram
(Add your circuit diagram here)

Project Structure
Code/
Hardware/
Images/
Documentation/
Report/
Working
IR sensor detects intrusion.
ESP32 processes the sensor data.
Data is transmitted through the CAN Bus.
LoRa communicates with the user module.
GPS provides vehicle location.
User controls Lock, Unlock, and Parking modes through the web interface.
Relay performs engine lockdown when required.
Results
The system successfully:

Detected unauthorized access
Communicated using CAN Bus
Enabled long-range communication using LoRa
Tracked vehicle location
Controlled engine operation through relay
Provided a web-based interface for user interaction
Future Improvements
Mobile application
Cloud connectivity
AI-based intrusion detection
CAN message encryption
SMS/Internet notifications
Authors
M. Yashwanth
M. Sai Charan Goud
M. Sanjay
k. shankar
License
This project is released under the MIT License.
