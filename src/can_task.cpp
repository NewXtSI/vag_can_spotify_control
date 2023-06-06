#include <Arduino.h>
#include <ESP32CAN.h>
#include <CAN_config.h>

CAN_device_t CAN_cfg;             // CAN Config
unsigned long previousMillis = 0; // will store last time a CAN Message was send
const int interval = 1000;        // interval at which send CAN Messages (milliseconds)
const int rx_queue_size = 10;     // Receive Queue size

QueueHandle_t globalCanQueue = nullptr;

void can_task(void *param)
{
    CAN_cfg.speed = CAN_SPEED_100KBPS;
    CAN_cfg.tx_pin_id = GPIO_NUM_12;
    CAN_cfg.rx_pin_id = GPIO_NUM_13;
    CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
    globalCanQueue = CAN_cfg.rx_queue;
    // Init CAN Module
    ESP32Can.CANInit();

    while (1)
    {
        CAN_frame_t rx_frame;

        unsigned long currentMillis = millis();

        // Receive next CAN frame from queue
        if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
        {

            if (rx_frame.FIR.B.FF == CAN_frame_std)
            {
                printf("New standard frame");
            }
            else
            {
                printf("New extended frame");
            }

            if (rx_frame.FIR.B.RTR == CAN_RTR)
            {
                printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID, rx_frame.FIR.B.DLC);
            }
            else
            {
                printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID, rx_frame.FIR.B.DLC);
                for (int i = 0; i < rx_frame.FIR.B.DLC; i++)
                {
                    printf("0x%02X ", rx_frame.data.u8[i]);
                }
                printf("\n");
            }
        }
        delay(1);
    }
    vTaskDelete(NULL);
}
