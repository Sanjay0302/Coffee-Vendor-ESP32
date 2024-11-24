#include <WiFi.h>
#include <WiFiUdp.h>
#include <string>
#include <cstring>
#include <sstream>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char *ssid = "samarth";
const char *password = "samarth44";

#define HANDSHAKE_REQUEST "COFFEE_HANDSHAKE"
#define HANDSHAKE_ACK "COFFEE_ACK"
#define ORDER_ACK "ORDER_ACK"

WiFiUDP udp;
const int localPort = 8080;
char packetBuffer[255];

QueueHandle_t messageQueue;

struct Message
{
    String Beverage;
    int Quantity;
};

const int timePerBeverage = 10;

IPAddress serverIP;
uint16_t serverPort;

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR   0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup()
{
    Serial.begin(115200);

    // Initialize the OLED display
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Connecting to wifi");
    display.display();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        display.print(".");
        display.display();
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Update OLED display with connection status
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("WiFi Connected");
    display.setCursor(0, 10);
    display.print("IP: ");
    display.print(WiFi.localIP());
    display.display();

    udp.begin(localPort);
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    messageQueue = xQueueCreate(10, sizeof(Message));
    xTaskCreate(taskFunction, "TaskName", 2048, NULL, 1, NULL);
}

void loop()
{
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        serverIP = udp.remoteIP();
        serverPort = udp.remotePort();

        int len = udp.read(packetBuffer, 255);
        if (len > 0)
        {
            packetBuffer[len] = 0;
            String receivedData = String(packetBuffer);

            if (receivedData == HANDSHAKE_REQUEST)
            {
                Serial.println("Received handshake request from server");
                udp.beginPacket(serverIP, serverPort);
                udp.write((uint8_t *)HANDSHAKE_ACK, strlen(HANDSHAKE_ACK));
                udp.endPacket();
                Serial.println("Sent handshake acknowledgment");
                return;
            }

            Serial.println("====================================");
            Serial.println("Order Details : ");
            Serial.println(packetBuffer);
            Serial.println("====================================");

            udp.beginPacket(serverIP, serverPort);
            udp.write((uint8_t *)ORDER_ACK, strlen(ORDER_ACK));
            udp.endPacket();
            Serial.println("Sent order acknowledgment");

            Message msg;
            std::stringstream ss(packetBuffer);
            std::string part1, part2;
            std::string fullStr(packetBuffer);

            size_t commaPos = fullStr.find(',');
            if (commaPos != std::string::npos)
            {
                part1 = fullStr.substr(0, commaPos);
                part2 = fullStr.substr(commaPos + 1);

                msg.Beverage = part1.c_str();
                msg.Quantity = std::stoi(part2);

                if (xQueueSend(messageQueue, &msg, portMAX_DELAY) != pdTRUE)
                {
                    Serial.println("Failed to send message to queue.");
                }
            }
            else
            {
                Serial.println("Failed to parse the message.");
            }
        }
    }
}

void taskFunction(void *pvParameters)
{
    while (true)
    {
        Message msg;
        if (xQueueReceive(messageQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            // Update display with order details
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Order Received:");
            display.setCursor(0, 10);
            display.print("Beverage: ");
            display.print(msg.Beverage);
            display.setCursor(0, 20);
            display.print("Quantity: ");
            display.print(msg.Quantity);
            display.display();

            for (int i = 0; i < msg.Quantity; i++)
            {
                // Update display with preparation status
                display.clearDisplay();
                display.setCursor(0, 0);
                display.print("Preparing Beverage ");
                display.print(i + 1);
                display.print(" of ");
                display.print(msg.Quantity);
                display.display();

                // Simulate the time taken to prepare each beverage
                vTaskDelay(pdMS_TO_TICKS(timePerBeverage * 1000));

                // Optionally update completion status
                display.clearDisplay();
                display.setCursor(0, 0);
                display.print("Completed Beverage ");
                display.print(i + 1);
                display.print(" of ");
                display.print(msg.Quantity);
                display.display();
            }

            // Final completion message
            String completionMsg = "Completed " + msg.Beverage + " x" + String(msg.Quantity);
            udp.beginPacket(serverIP, serverPort);
            udp.print(completionMsg);
            udp.endPacket();

            // Update display with completion message
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print(completionMsg);
            display.display();
        }
    }
}

