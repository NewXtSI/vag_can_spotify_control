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



// Notifications
//
#define EVENT_JOYSTICK_UP               _BV(0)
#define EVENT_JOYSTICK_DOWN             _BV(1)
#define EVENT_JOYSTICK_LEFT             _BV(2)
#define EVENT_JOYSTICK_RIGHT            _BV(3)

#endif // __CONFIG_H__
