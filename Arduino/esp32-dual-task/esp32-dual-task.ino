/*
* this program creates a task where the loop and task created are independent and timesharing is achieved
* loop can pase the udp packet
* task can process the vending
*/

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
            Serial.println("Received Control Signal:");
            Serial.println(packetBuffer);
        }
    }
}

void taskFunction(void *pvParameters) {
    while (true) {
        // Your task code here
        Serial.println("Task is running");
        delay(5000); // Simulate work
    }
}



/*
# Task Creation: The xTaskCreate function is used to create a new task. 

The parameters include:
- The function that will run as the task (taskFunction).
- A name for the task (for debugging purposes).
- The stack size for the task (in bytes).
- Parameters to pass to the task (can be NULL if not needed).
- The priority of the task (1 is a low priority).
- A handle to the task (can be NULL if not needed).
- Task Function: The taskFunction runs in its own context. You can put your task-specific code here. The while (true) loop allows the task to run indefinitely.
- Delay: Use vTaskDelay() or delay() to yield control back to the FreeRTOS scheduler, allowing other tasks to run.


! Important Notes:

- Stack Size: Be mindful of the stack size you allocate for each task. The ESP32 has limited RAM, so allocate only what you need.
- Task Priorities: FreeRTOS allows you to set different priorities for tasks. Higher priority tasks will preempt lower priority ones.
- Synchronization: If you need to share data between tasks, consider using FreeRTOS mechanisms like queues, semaphores, or mutexes.
*/