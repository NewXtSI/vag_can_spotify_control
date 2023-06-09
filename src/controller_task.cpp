#include <Arduino.h>

#include "../include/config.h"
#include <EventJoystick.h>

extern EventGroupHandle_t notification_event;

EventJoystick ej1(JOYSTICK_PIN_ANALOG_X, JOYSTICK_PIN_ANALOG_Y);

void onEj1XChanged(EventAnalog& ej) {
  if (ej.position() > 10)
    xEventGroupSetBits(notification_event, EVENT_JOYSTICK_RIGHT);
  else
    xEventGroupClearBits(notification_event, EVENT_JOYSTICK_RIGHT);
  if (ej.position() < -10)
    xEventGroupSetBits(notification_event, EVENT_JOYSTICK_LEFT);
  else
    xEventGroupClearBits(notification_event, EVENT_JOYSTICK_LEFT);
}

void onEj1YChanged(EventAnalog& ej) {
  if (ej.position() > 10)
    xEventGroupSetBits(notification_event, EVENT_JOYSTICK_UP);
  else
    xEventGroupClearBits(notification_event, EVENT_JOYSTICK_UP);
  if (ej.position() < -10)
    xEventGroupSetBits(notification_event, EVENT_JOYSTICK_DOWN);
  else
    xEventGroupClearBits(notification_event, EVENT_JOYSTICK_DOWN);
}

void controller_task(void *param) {
    ej1.x.setChangedHandler(onEj1XChanged);
    ej1.y.setChangedHandler(onEj1YChanged);
//    ej1.setNumIncrements(255);
    while (1) {
        ej1.update();
        const portTickType xDelay = 10 / portTICK_RATE_MS;
        vTaskDelay(xDelay);
    }
    vTaskDelete(NULL);
}


