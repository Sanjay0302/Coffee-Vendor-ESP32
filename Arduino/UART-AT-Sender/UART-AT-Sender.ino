/*
 * This  will send the AT commands through UART1 (rx 16, tx 17)
 * Works with other ESP32 in AT mode
 */

#include <HardwareSerial.h>

// Define the serial port for AT commands
HardwareSerial mySerial(1); // Use UART1 (you can also use UART0 or UART2)

// Define the baud rate for the AT commands
#define AT_BAUD_RATE 115200

void setup()
{
    // Start the built-in Serial for debugging
    Serial.begin(115200);
    while (!Serial)
    {
        ; // Wait for serial port to connect. Needed for native USB
    }

    // Start the serial port for AT commands
    mySerial.begin(AT_BAUD_RATE, SERIAL_8N1, 16, 17); // RX, TX pins (change as needed)

    Serial.println("Sending AT commands...");

    // Send an example AT command
    sendATCommand("AT");
    delay(1000);
    sendATCommand("AT+GMR");
}

void loop()
{
    // You can add more logic here if needed
}

// Function to send AT command and read response
void sendATCommand(const char *command)
{
    // Send the AT command
    mySerial.println(command); // println includes both \r and \n
    // mySerial.print(command); // Send the command
    //  mySerial.print("\r\n");  // Send CR and LF
    Serial.print("Sent: ");
    Serial.println(command);

    // Wait for a response
    delay(1000); // Adjust delay as needed

    // Read the response
    while (mySerial.available())
    {
        String response = mySerial.readStringUntil('\n');
        Serial.print("Received: ");
        Serial.println(response);
    }
}
