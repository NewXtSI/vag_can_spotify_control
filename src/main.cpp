#include <WiFi.h>

#include "../include/config.h"

#include "FS.h"
#include <LITTLEFS.h>

EventGroupHandle_t notification_event;

#define USE_FTP 1
#if USE_FTP
#  include "ESP-FTP-Server-Lib.h"
#  include "FTPFilesystem.h"
FTPServer *featureftp;
#endif


// Forward declarations
void spotify_task(void *param);
void ui_task(void *param);
void can_task(void *param);
void controller_task(void *param);

void setup() {
    featureftp = new FTPServer();
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    notification_event = xEventGroupCreate();

    // versuchen, ein vorhandenes Dateisystem einzubinden
    if (!LittleFS.begin(false)) {
        Serial.println("LITTLEFS Mount fehlgeschlagen");
        Serial.println("Kein Dateisystemsystem gefunden; wird formatiert");
        // falls es nicht klappt, erneut mit Neu-Formatierung versuchen
        if (!LittleFS.begin(true)) {
            Serial.println("LITTLEFS Mount fehlgeschlagen");
            Serial.println("Formatierung nicht möglich");
        } else {
            Serial.println("Formatierung des Dateisystems erfolgt");
        }
    }
 
    Serial.println("Informationen zum Dateisystem:");
    Serial.printf("- Bytes total:   %ld\n", LittleFS.totalBytes());
    Serial.printf("- Bytes genutzt: %ld\n\n", LittleFS.usedBytes());
#if USE_FTP
    featureftp->addUser("user", "user");
    featureftp->addFilesystem("LittlFS", &LittleFS);
#endif
#if 1
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
#if USE_FTP
      featureftp->begin();
#endif
#endif

//    xTaskCreatePinnedToCore(spotify_task, "spotify_task", 1024 * 7, NULL, 1, nullptr, 0);
    xTaskCreatePinnedToCore(ui_task, "ui_task", 1024 * 14, NULL, 1, nullptr, 1);
    xTaskCreatePinnedToCore(can_task, "can_task", 1024 * 10, NULL, 1, nullptr, 1);
    xTaskCreatePinnedToCore(controller_task, "controller_task", 1024 * 5, NULL, 1, nullptr, 1);
}

void loop() {
#if USE_FTP
    featureftp->handle();
#endif        
    delay(5);
}
