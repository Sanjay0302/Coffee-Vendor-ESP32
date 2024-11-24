#include <WiFi.h>
#include <WiFiUdp.h>
#include <string>
#include <cstring>
#include <sstream>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

const char *ssid = "sanjay";
const char *password = "123456789";

WiFiUDP udp;
const int localPort = 8080;

char packetBuffer[255];

QueueHandle_t messageQueue;

struct Message
{
    String Beverage;
    int Quantity;
};

using namespace std;

const int timePerBeverage = 10;

void setup()
{
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
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

void loop()
{
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        int len = udp.read(packetBuffer, 255);
        if (len > 0)
        {
            packetBuffer[len] = 0;
            Serial.println("====================================");
            Serial.println("Order Details : ");
            Serial.println(packetBuffer);
            Serial.println("====================================");

            Message msg;

            stringstream ss(packetBuffer);
            string part1, part2;

            if (getline(ss, part1, ',') && getline(ss, part2, ','))
            {
                msg.Beverage = part1.c_str();
                msg.Quantity = stoi(part2);
            }
            else
            {
                Serial.println("Failed to split the message.");
                return;
            }

            if (xQueueSend(messageQueue, &msg, portMAX_DELAY) != pdTRUE)
            {
                Serial.println("Failed to send message to queue.");
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
            Serial.println("====================================");
            Serial.println("Task is processing the message:");
            Serial.print("Beverage : ");
            Serial.println(msg.Beverage.c_str());
            Serial.print("Quantity : ");
            Serial.println(msg.Quantity);
            vTaskDelay(pdMS_TO_TICKS(msg.Quantity * timePerBeverage * 1000));
        }
    }
}
