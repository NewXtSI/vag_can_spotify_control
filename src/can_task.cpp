#include <Arduino.h>

#include "../include/config.h"

#include <CAN.h>

#include "FS.h"
#include <LITTLEFS.h>

unsigned long previousMillis = 0; // will store last time a CAN Message was send
const int interval = 1000;        // interval at which send CAN Messages (milliseconds)
const int rx_queue_size = 10;     // Receive Queue size

QueueHandle_t globalCanQueue = nullptr;
extern EventGroupHandle_t notification_event;

bool bIsLogging = false;

uint32_t uiStandardFrames = 0;
uint32_t uiExtendedFrames = 0;

SemaphoreHandle_t candataSemaphore = NULL;

#if 0
void dumpFrame(CAN_frame_t rx_frame) {
    static File logfile;
    if (rx_frame.FIR.B.FF == CAN_frame_std) {
        if (xSemaphoreTake(candataSemaphore, (TickType_t)portMAX_DELAY) == pdTRUE) {
            uiStandardFrames++;
            xSemaphoreGive(candataSemaphore);
        }
    } else {
        if (xSemaphoreTake(candataSemaphore, (TickType_t)portMAX_DELAY) == pdTRUE) {
            uiExtendedFrames++;
            xSemaphoreGive(candataSemaphore);
        }
    }
    EventBits_t bit = xEventGroupGetBits(notification_event);
    if (bit & EVENT_LOG_TO_LITTLEFS) {
        if (!bIsLogging) {
            logfile = LittleFS.open("/candump.txt", "a");
            bIsLogging = true;
        }
        logfile.printf("%10d %d %d %d 0x%04x ", millis(), rx_frame.FIR.B.DLC, rx_frame.FIR.B.RTR, rx_frame.FIR.B.FF, rx_frame.MsgID);
        for (int i = 0; i < rx_frame.FIR.B.DLC; i++) {
          logfile.printf("%02X ", rx_frame.data.u8[i]);
        }
        logfile.printf("\n");
    } else {
        if (bIsLogging) {
            logfile.close();
            bIsLogging = false;
        }
    }
}

#endif

void can_get_status(uint32_t &uiStandardFrames_, uint32_t &uiExtendedFrames_) {
    if (xSemaphoreTake(candataSemaphore, (TickType_t)portMAX_DELAY) == pdTRUE) {
        uiStandardFrames_ = uiStandardFrames;
        uiExtendedFrames_ = uiExtendedFrames;
        xSemaphoreGive(candataSemaphore);
    }
}
typedef struct {
    uint32_t    uiID;
    bool        bExtended;
    bool        bRtr;
    uint8_t     uiDLC;
    uint8_t     uiData[8];
} CANFrame_t;

void dumpFrame() {
    static File logfile;
    CANFrame_t  frame;
    frame.uiID = CAN.packetId();
    frame.bExtended = CAN.packetExtended();
    frame.bRtr = CAN.packetRtr();
    frame.uiDLC = CAN.packetDlc();
    if (!frame.bRtr) {
        for (int i=0; i < frame.uiDLC; ++i) {
            frame.uiData[i] = CAN.read();
        }
    }
    if (xSemaphoreTake(candataSemaphore, (TickType_t)portMAX_DELAY) == pdTRUE) {
        if (frame.bExtended)
            uiExtendedFrames++;
        else
            uiStandardFrames++;
    }
    EventBits_t bit = xEventGroupGetBits(notification_event);
    if (bit & EVENT_LOG_TO_LITTLEFS) {
        if (!bIsLogging) {
            logfile = LittleFS.open("/candump.txt", "a");
            bIsLogging = true;
        }
        logfile.printf("%10d %d %d %d 0x%04x ", millis(), frame.uiDLC, frame.bRtr, frame.bExtended, frame.uiID);
        if (!frame.bRtr) {
            for (int i = 0; i < frame.uiDLC; i++) {
              logfile.printf("%02X ", frame.uiData[i]);
            }
        }
        logfile.printf("\n");
    } else {
        if (bIsLogging) {
            logfile.close();
            bIsLogging = false;
        }
    }
}

void startCANBus() {
    CAN.setPins(CAN_PIN_RX, CAN_PIN_TX);
    CAN.begin(500E3);
}

void stopCANBus() {
    CAN.end();
}

void can_task(void *param)
{
    candataSemaphore = xSemaphoreCreateMutex();
 
//    globalCanQueue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
    // Init CAN Module
    startCANBus();

    while (1) {
        int packetSize = CAN.parsePacket();
        if (packetSize) {
            dumpFrame();
        }
#if 0        
        CAN_frame_t rx_frame;

        unsigned long currentMillis = millis();

        // Receive next CAN frame from queue
        if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
            if (rx_frame.FIR.B.FF == CAN_frame_std) {
                printf("New standard frame");
            } else {
                printf("New extended frame");
            }

            if (rx_frame.FIR.B.RTR == CAN_RTR) {
                printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID, rx_frame.FIR.B.DLC);
            } else {
                printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID, rx_frame.FIR.B.DLC);
                for (int i = 0; i < rx_frame.FIR.B.DLC; i++) {
                    printf("0x%02X ", rx_frame.data.u8[i]);
                }
                printf("\n");
            }
            dumpFrame(rx_frame);
        }
#endif        
    }
    vTaskDelete(NULL);
}
