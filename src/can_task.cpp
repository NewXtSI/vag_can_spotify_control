#include <Arduino.h>
#include <core_version.h> // For ARDUINO_ESP32_RELEASE

#include "../include/config.h"

#include <ACAN_ESP32.h>

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

#define LOOPBACKMODE            0

#if 1
void dumpFrame(CANMessage rx_frame) {
    static File logfile;
    if (rx_frame.ext) {
        if (xSemaphoreTake(candataSemaphore, (TickType_t)portMAX_DELAY) == pdTRUE) {
            uiExtendedFrames++;
            xSemaphoreGive(candataSemaphore);
        }
    } else {
        if (xSemaphoreTake(candataSemaphore, (TickType_t)portMAX_DELAY) == pdTRUE) {
            uiStandardFrames++;
            xSemaphoreGive(candataSemaphore);
        }
    }
    EventBits_t bit = xEventGroupGetBits(notification_event);
    if (bit & EVENT_LOG_TO_LITTLEFS) {
        if (!bIsLogging) {
            logfile = LittleFS.open("/candump.txt", "a");
            bIsLogging = true;
        }
        logfile.printf("%10d %d %d %d 0x%04x ", millis(), rx_frame.len, rx_frame.rtr, rx_frame.ext, rx_frame.id);
        if (!rx_frame.rtr) {
            for (int i = 0; i < rx_frame.len; i++) {
                logfile.printf("%02X ", rx_frame.data[i]);
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

#endif
uint32_t errorCode = 0x00;

void can_get_status(uint32_t &uiStandardFrames_, uint32_t &uiExtendedFrames_, uint32_t &error) {
    if (xSemaphoreTake(candataSemaphore, (TickType_t)portMAX_DELAY) == pdTRUE) {
        uiStandardFrames_ = uiStandardFrames;
        uiExtendedFrames_ = uiExtendedFrames;
        error = errorCode;
        xSemaphoreGive(candataSemaphore);
    }
}
#if 0
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
#endif

void startCANBus() {
    Serial.println("Configure ESP32 CAN");
    xEventGroupSetBits(notification_event, EVENT_LOG_TO_LITTLEFS);
    ACAN_ESP32_Settings settings(CAN_SPEED * 1000);
#if LOOPBACKMODE    
    settings.mRequestedCANMode = ACAN_ESP32_Settings::LoopBackMode;
#else
//    settings.mRequestedCANMode = ACAN_ESP32_Settings::ListenOnlyMode;
#endif    
    settings.mRxPin = CAN_PIN_RX;   // Optional, default Tx pin is GPIO_NUM_4
    settings.mTxPin = CAN_PIN_TX;   // Optional, default Rx pin is GPIO_NUM_5
    errorCode = ACAN_ESP32::can.begin(settings);
    if (errorCode == 0) {
        Serial.print("Bit Rate prescaler: ");
        Serial.println(settings.mBitRatePrescaler);
        Serial.print("Time Segment 1:     ");
        Serial.println(settings.mTimeSegment1);
        Serial.print("Time Segment 2:     ");
        Serial.println(settings.mTimeSegment2);
        Serial.print("RJW:                ");
        Serial.println(settings.mRJW);
        Serial.print("Triple Sampling:    ");
        Serial.println(settings.mTripleSampling ? "yes" : "no");
        Serial.print("Actual bit rate:    ");
        Serial.print(settings.actualBitRate());
        Serial.println(" bit/s");
        Serial.print("Exact bit rate ?    ");
        Serial.println(settings.exactBitRate() ? "yes" : "no");
        Serial.print("Distance            ");
        Serial.print(settings.ppmFromDesiredBitRate());
        Serial.println(" ppm");
        Serial.print("Sample point:       ");
        Serial.print(settings.samplePointFromBitStart());
        Serial.println("%");
        Serial.println("Configuration OK!");
    } else {
        Serial.print("Configuration error 0x");
        Serial.println(errorCode, HEX);
    }
}

void stopCANBus() {
    
}

static uint32_t gBlinkLedDate = 0;
static uint32_t gReceivedFrameCount = 0;
static uint32_t gSentFrameCount = 0;

void can_task(void *param)
{
    candataSemaphore = xSemaphoreCreateMutex();
    //--- Display ESP32 Chip Info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    Serial.print("ESP32 Arduino Release: ");
    Serial.println(ARDUINO_ESP32_RELEASE);
    Serial.print("ESP32 Chip Revision: ");
    Serial.println(chip_info.revision);
    Serial.print("ESP32 SDK: ");
    Serial.println(ESP.getSdkVersion());
    Serial.print("ESP32 Flash: ");
    Serial.print(spi_flash_get_chip_size() / (1024 * 1024));
    Serial.print(" MB ");
    Serial.println(((chip_info.features & CHIP_FEATURE_EMB_FLASH) != 0) ? "(embeded)" : "(external)");
    Serial.print("APB CLOCK: ");
    Serial.print(APB_CLK_FREQ);
    Serial.println(" Hz");


    delay(10000);
    //    globalCanQueue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
    // Init CAN Module
    startCANBus();

    while (1) {
    CANMessage frame;
#if LOOPBACKMODE    
    if (gBlinkLedDate < millis()) {
        gBlinkLedDate += 500;
        //    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
        Serial.print("Sent: ");
        Serial.print(gSentFrameCount);
        Serial.print(" ");
        Serial.print("Receive: ");
        Serial.print(gReceivedFrameCount);
        Serial.print(" ");
        Serial.print(" STATUS 0x");
        Serial.print(TWAI_STATUS_REG, HEX);
        Serial.print(" RXERR ");
        Serial.print(TWAI_RX_ERR_CNT_REG);
        Serial.print(" TXERR ");
        Serial.println(TWAI_TX_ERR_CNT_REG);
        frame.id = 0x21;
        frame.len = 4;
        frame.data[0] = 0xA1;
        frame.data[1] = 0xB2;
        frame.data[2] = 0xC3;
        frame.data[3] = 0xD4;
        const bool ok = ACAN_ESP32::can.tryToSend(frame);
        if (ok) {
            gSentFrameCount += 1;
        }
    }
#endif    
    EventBits_t bit = xEventGroupGetBits(notification_event);
    if (bit & EVENT_RESET_CAN) {
        xEventGroupClearBits(notification_event, EVENT_RESET_CAN);
        ACAN_ESP32::can.recoverFromBusOff();
    } else {
        while (ACAN_ESP32::can.receive(frame)) {
            dumpFrame(frame);
            gReceivedFrameCount += 1;
            const portTickType xDelay = 1 / portTICK_RATE_MS;
            vTaskDelay(xDelay);
        }
    }
    const portTickType xDelay = 2 / portTICK_RATE_MS;
    vTaskDelay(xDelay);
#if 0        
        int packetSize = CAN.parsePacket();
        if (packetSize) {
            dumpFrame();
        }
#endif        
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
