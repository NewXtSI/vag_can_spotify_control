#include <WiFi.h>

#include "../include/config.h"

// Forward declarations
void spotify_task(void *param);
void ui_task(void *param);
void can_task(void *param);

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
//    xTaskCreatePinnedToCore(spotify_task, "spotify_task", 1024 * 7, NULL, 1, nullptr, 0);
    xTaskCreatePinnedToCore(ui_task, "ui_task", 1024 * 14, NULL, 1, nullptr, 1);
    xTaskCreatePinnedToCore(can_task, "can_task", 1024 * 10, NULL, 1, nullptr, 1);
}

void loop() {
    delay(10);
}
