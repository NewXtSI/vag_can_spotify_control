# vag_can_spotify_control

[Project blog](BLOG.md)

# Problem....
My Skoda Superb (2008) has Bluetooth Streaming to the cars multimedia system. But no chance to skip, pause, metadata display or something like this.

So I decided to build my own "controller" based on a ESP32 board with CAN access to get messages from the steering wheel remote and display the metadata.

First version is on a LiLyGo T-Display to check directly the car canbus communications.

# Used components
- LiLyGo T-Display (16MB version) (https://www.lilygo.cc/products/lilygo%C2%AE-ttgo-t-display-1-14-inch-lcd-esp32-control-board)
- TJA1050 CAN transceiver (https://www.amazon.de/dp/B07DK93W6F?psc=1&ref=ppx_yo2ov_dt_b_product_details)
- LM2596S Stepdown Module (https://www.azdelivery.de/products/lm2596s-dc-dc-step-down-modul-1)
- Old OBD2 connector
- PSP2000 thumbstick

# Libraries used
- EventJoystick (https://github.com/Stutchbury/EventJoystick)
- LovyanGFX (https://github.com/lovyan03/LovyanGFX)
- lvgl (https://lvgl.io/)
- ESP32CAN (https://github.com/miwagner/ESP32-Arduino-CAN)

# Useful resources
- https://github.com/notyal/vwcanread
- https://habr.com/ru/articles/442184/
