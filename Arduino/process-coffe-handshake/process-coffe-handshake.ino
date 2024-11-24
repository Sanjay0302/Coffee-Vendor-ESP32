#include <WiFi.h>
#include <WiFiUdp.h>
#include <string>
#include <cstring>
#include <sstream>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

const char *ssid = "sanjay";
const char *password = "123456789";

#define HANDSHAKE_REQUEST "COFFEE_HANDSHAKE"
#define HANDSHAKE_ACK "COFFEE_ACK"

WiFiUDP udp;
const int localPort = 8080;
char packetBuffer[255];

// Define a queue to hold received messages
QueueHandle_t messageQueue;

// Structure to hold the message data
struct Message {
    String Beverage;
    int Quantity;
};

const int timePerBeverage = 10; // Time in seconds to prepare one Beverage

// Store server address for communication
IPAddress serverIP;
uint16_t serverPort;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    udp.begin(localPort);
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    messageQueue = xQueueCreate(10, sizeof(Message));
    xTaskCreate(taskFunction, "TaskName", 2048, NULL, 1, NULL);

}

void loop() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        // Store sender's IP and port for later communication
        serverIP = udp.remoteIP();
        serverPort = udp.remotePort();
        
        int len = udp.read(packetBuffer, 255);
        if (len > 0) {
            packetBuffer[len] = 0;
            String receivedData = String(packetBuffer);
            
            // Handle handshake request
            if (receivedData == HANDSHAKE_REQUEST) {
                Serial.println("Received handshake request from server");
                // Send acknowledgment back
                udp.beginPacket(serverIP, serverPort);
                udp.write((uint8_t*)HANDSHAKE_ACK, strlen(HANDSHAKE_ACK));
                udp.endPacket();
                Serial.println("Sent handshake acknowledgment");
                return;
            }
            
            // Handle beverage orders
            Serial.println("====================================");
            Serial.println("Order Details : ");
            Serial.println(packetBuffer);
            Serial.println("====================================");

            Message msg;
            std::stringstream ss(packetBuffer);
            std::string part1, part2;
            std::string fullStr(packetBuffer);
            
            size_t commaPos = fullStr.find(',');
            if (commaPos != std::string::npos) {
                part1 = fullStr.substr(0, commaPos);
                part2 = fullStr.substr(commaPos + 1);
                
                msg.Beverage = part1.c_str();
                msg.Quantity = std::stoi(part2);
                
                if (xQueueSend(messageQueue, &msg, portMAX_DELAY) != pdTRUE) {
                    Serial.println("Failed to send message to queue.");
                }
            } else {
                Serial.println("Failed to parse the message.");
            }
        }
    }
}

void taskFunction(void *pvParameters) {
    while (true) {
        Message msg;
        // Wait for a message to arrive in the queue
        if (xQueueReceive(messageQueue, &msg, portMAX_DELAY) == pdTRUE) {
            Serial.println("====================================");
            Serial.println("Task is processing the message:");
            Serial.print("Beverage : ");
            Serial.println(msg.Beverage.c_str());
            Serial.print("Quantity : ");
            Serial.println(msg.Quantity);
            
            // Simulate beverage preparation
            for (int i = 0; i < msg.Quantity; i++) {
                Serial.print("Preparing beverage ");
                Serial.print(i + 1);
                Serial.print(" of ");
                Serial.println(msg.Quantity);
                
                vTaskDelay(pdMS_TO_TICKS(timePerBeverage * 1000));
                
                Serial.print("Completed beverage ");
                Serial.print(i + 1);
                Serial.print(" of ");
                Serial.println(msg.Quantity);
            }
            
            // Send completion message back to server
            String completionMsg = "Completed " + msg.Beverage + " x" + String(msg.Quantity);
            udp.beginPacket(serverIP, serverPort);
            udp.print(completionMsg);
            udp.endPacket();
        }
    }
}