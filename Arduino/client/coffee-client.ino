#include <WiFi.h>
#include <WiFiUdp.h>

const char *ssid = "sanjay";
const char *password = "123456789";

WiFiUDP udp;
const int localPort = 8080;
char packetBuffer[255];

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
            Serial.println("Received Control Signal:");
            Serial.println(packetBuffer);
        }
    }
}