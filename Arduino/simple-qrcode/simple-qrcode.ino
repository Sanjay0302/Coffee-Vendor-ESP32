/* *********************************************************************************
 * ESP8266 QRcode
 * dependency library :
 *   ESP8266 Oled Driver for SSD1306 display by Daniel Eichborn, Fabrice Weinberg
 *
 * SDA --> D6
 * SCL --> D7
 ***********************************************************************************/
/*
 * https://github.com/yoprogramo/QRcodeOled
 */

#include <qrcodeoled.h>
#include <SSD1306.h>

#define SDA 21
#define SCL 22

SSD1306 display(0x3c, SDA, SCL); // Only change

QRcodeOled qrcode(&display);

void setup()
{

    Serial.begin(115200);
    Serial.println("");
    Serial.println("Starting...");

    display.init();
    display.clear();
    display.display();

    // enable debug qrcode
    // qrcode.debug();
    
    qrcode.init();
    // create qrcode
    qrcode.create("Hello world.");
}

void loop() {}