#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "../include/user_config.h"

#ifndef WIFI_SSID
#  pragma error "Define your WiFi SSID in user_config.h"
#endif
#ifndef SPOTIFY_CLIENT_ID
#  pragma error "Define your spotify clientid in user_config.h"
#endif
#ifndef SPOTIFY_CLIENT_SECRET
#  pragma error "Define your spotify client secret in user_config.h"
#endif
#ifndef SPOTIFY_MARKET
#  define SPOTIFY_MARKET "DE"
#endif
#ifndef SPOTIFY_REFRESH_TOKEN
#  pragma error "Define your spotify refreshtoken in user_config.h"
#endif

#define     CAN_SPEED                   500
#define     CAN_PIN_TX                  GPIO_NUM_25
#define     CAN_PIN_RX                  GPIO_NUM_13
#define     JOYSTICK_PIN_ANALOG_X       GPIO_NUM_32
#define     JOYSTICK_PIN_ANALOG_Y       GPIO_NUM_33

// Notifications
//
#define EVENT_JOYSTICK_UP               _BV(0)
#define EVENT_JOYSTICK_DOWN             _BV(1)
#define EVENT_JOYSTICK_LEFT             _BV(2)
#define EVENT_JOYSTICK_RIGHT            _BV(3)

#define EVENT_LOG_TO_LITTLEFS           _BV(10)

#endif // __CONFIG_H__
