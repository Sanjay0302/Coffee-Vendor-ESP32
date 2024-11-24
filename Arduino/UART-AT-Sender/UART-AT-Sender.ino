/*
 * This  will send the AT commands through UART1 (rx 16, tx 17)
 * Works with other ESP32 in AT mode
 */

#include <HardwareSerial.h>

HardwareSerial mySerial(1); // Use UART1 (you can also use UART0 or UART2)

#define AT_BAUD_RATE 115200

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    mySerial.begin(AT_BAUD_RATE, SERIAL_8N1, 16, 17); // RX, TX pins (change as needed)

    Serial.println("Sending AT commands...");

    sendATCommand("AT");
    delay(1000);
    sendATCommand("AT+GMR");
}

void loop() {}

void sendATCommand(const char *command)
{
    mySerial.println(command); // println includes both \r and \n
    // mySerial.print(command); // Send the command
    //  mySerial.print("\r\n");  // Send CR and LF
    Serial.print("Sent: ");
    Serial.println(command);

    // Wait for a response
    delay(1000);

    while (mySerial.available())
    {
        String response = mySerial.readStringUntil('\n');
        Serial.print("Received: ");
        Serial.println(response);
    }
}
