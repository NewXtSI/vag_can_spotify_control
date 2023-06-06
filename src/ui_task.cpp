#include <Arduino.h>

#if !defined LGFX_AUTODETECT
  #define LGFX_AUTODETECT
#endif
#define LGFX_USE_V1     // set to use new version of library
#define LGFX_TTGO_TDISPLAY 

#include <EventJoystick.h>

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
#include <lvgl.h>
#include "lv_conf.h"

#include <vector>

#define screenWidth 240
#define screenHeight 135

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

extern QueueHandle_t globalCanQueue;

/*** Function declaration ***/
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

#define LINE_COUNT 3

static LGFX lcd; // declare display variable

static std::vector<int> points[LINE_COUNT];
static int colors[] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW };
static int xoffset, yoffset, point_count;

EventJoystick ej1(32,33);
#include <ESP32CAN.h>

/**
 * A function to handle the X axis 'changed' event
 * Can be called anything but requires EventAnalog& 
 * as its only parameter.
 * Note: Here we are specifying an EventAnalog type
 * not an EventJoystick.
 */
void onEj1XChanged(EventAnalog& ej) {
  Serial.print("ej1 changed. X position: ");
  Serial.println(ej.position());
}

/**
 * A function to handle the Y axis 'changed' event
 * Can be called anything but requires EventAnalog& 
 * as its only parameter.
 * Note: Here we are specifying an EventAnalog type
 * not an EventJoystick.
 */
void onEj1YChanged(EventAnalog& ej) {
  Serial.print("ej1 changed. Y position: ");
  Serial.println(ej.position());
}


int getBaseColor(int x, int y)
{
  return ((x^y)&3 || ((x-xoffset)&31 && y&31) ? TFT_BLACK : ((!y || x == xoffset) ? TFT_WHITE : TFT_DARKGREEN));
}

void test(LGFX_Device &lcd)
{
  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(128);
  lcd.setColorDepth(24);
  lcd.drawPixel(0, 0, 0xFFFF);
  lcd.drawFastVLine(2, 0, 100, lcd.color888(255,   0,   0));
  lcd.drawFastVLine(4, 0, 100, lcd.color565(  0, 255,   0));
  lcd.drawFastVLine(6, 0, 100, lcd.color332(  0,   0, 255));
  delay(200);
  uint32_t red = 0xFF0000;
  lcd.drawFastHLine(0, 2, 100, red);
  lcd.drawFastHLine(0, 4, 100, 0x00FF00U);
  lcd.drawFastHLine(0, 6, 100, (uint32_t)0xFF);
  delay(200);
  uint16_t green = 0x07E0;
  lcd.drawRect(10, 10, 50, 50, 0xF800);
  lcd.drawRect(12, 12, 50, 50, green);
  lcd.drawRect(14, 14, 50, 50, (uint16_t)0x1F);
  delay(200);
  uint8_t blue = 0x03;
  lcd.fillRect(20, 20, 20, 20, (uint8_t)0xE0);
  lcd.fillRect(30, 30, 20, 20, (uint8_t)0x1C);
  lcd.fillRect(40, 40, 20, 20, blue);
  lcd.setColor(0xFF0000U);
  lcd.fillCircle ( 40, 80, 20    );
  lcd.fillEllipse( 80, 40, 10, 20);
  lcd.fillArc    ( 80, 80, 20, 10, 0, 90);
  lcd.fillTriangle(80, 80, 60, 80, 80, 60);
  lcd.setColor(0x0000FFU);
  delay(200);
  lcd.drawCircle ( 40, 80, 20    );
  lcd.drawEllipse( 80, 40, 10, 20);
  lcd.drawArc    ( 80, 80, 20, 10, 0, 90);
  lcd.drawTriangle(60, 80, 80, 80, 80, 60);
  lcd.setColor(0x00FF00U);
  lcd.drawBezier( 60, 80, 80, 80, 80, 60);
  delay(200);
  lcd.drawBezier( 60, 80, 80, 20, 20, 80, 80, 60);
  lcd.drawGradientLine( 0, 80, 80, 0, 0xFF0000U, 0x0000FFU);

  delay(200);
  lcd.fillScreen(0xFFFFFFu);
  lcd.setColor(0x00FF00u);
  delay(200);
  lcd.fillScreen();
  lcd.clear(0xFFFFFFu);
  delay(200);
  lcd.setBaseColor(0x000000u);
  lcd.clear();
  lcd.drawLine(0, 1, 39, 40, red);
  lcd.drawLine(1, 0, 40, 39, blue);
  lcd.startWrite();
  lcd.drawLine(38, 0, 0, 38, 0xFFFF00U);
  lcd.drawLine(39, 1, 1, 39, 0xFF00FFU);
  lcd.drawLine(40, 2, 2, 40, 0x00FFFFU);
  lcd.endWrite();
  delay(200);
  lcd.startWrite();
  lcd.startWrite();
  lcd.startWrite();
  lcd.endWrite();
  lcd.endWrite();
  lcd.endWrite();
  lcd.endWrite();
  lcd.startWrite();
  lcd.startWrite();
  lcd.drawPixel(0, 0);
  lcd.endTransaction();
  lcd.beginTransaction();
  lcd.drawPixel(0, 0);
  lcd.endWrite();
  lcd.endWrite();
  lcd.startWrite();
  for (uint32_t x = 0; x < 128; ++x) {
    for (uint32_t y = 0; y < 128; ++y) {
      lcd.writePixel(x, y, lcd.color888( x*2, x + y, y*2));
    }
  }
  lcd.endWrite();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ui_task(void *param) {
    lcd.init();
    ej1.x.setChangedHandler(onEj1XChanged);
    ej1.y.setChangedHandler(onEj1YChanged);
//    ej1.setNumIncrements(255);
    lv_init();   // Initialize lvgl
    if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);
    /* LVGL : Setting up buffer to use for display */
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    /*** LVGL : Setup & Initialize the display device driver ***/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    lv_obj_t *mainscreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(mainscreen, lv_color_hex(0x303030),0);

    lv_obj_t *lbl = lv_label_create(mainscreen);
    lv_obj_set_pos(lbl, 10, 10);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff),0);
    lv_label_set_text(lbl, "Test");


    lv_scr_load(mainscreen);
      lcd.setBrightness(255);

    while (1) {
        ej1.update();    
        lv_timer_handler(); /* let the GUI do its work */
        if (globalCanQueue != nullptr) {

        }
    }
    vTaskDelete(NULL);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ui_task2(void *param) {
  lcd.init();
  ej1.x.setChangedHandler(onEj1XChanged);
  ej1.y.setChangedHandler(onEj1YChanged);
    ej1.setNumIncrements(255);
  if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);

  yoffset = lcd.height() >> 1;
  xoffset = lcd.width()  >> 1;
  point_count = lcd.width() + 1;

  for (int i = 0; i < LINE_COUNT; i++)
  {
    points[i].resize(point_count);
  }

  lcd.startWrite();
  lcd.setAddrWindow(0, 0, lcd.width(), lcd.height());
  for (int y = 0; y < lcd.height(); y++)
  {
    for (int x = 0; x < lcd.width(); x++)
    {
      lcd.writeColor(getBaseColor(x, y - yoffset), 1);
    }
  }
  lcd.endWrite();
      lcd.setBrightness(255);
  //  test(lcd);
    while(1) {
    ej1.update();    

  static int prev_sec;
  static int fps;
  ++fps;
  int sec = millis() / 1000;
  if (prev_sec != sec)
  {
    prev_sec = sec;
    lcd.setCursor(0,0);
    lcd.printf("fps:%03d", fps);
    fps = 0;
  }

  static int count;

  for (int i = 0; i < LINE_COUNT; i++)
  {
    points[i][count % point_count] = (sinf((float)count / (10 + 30 * i))+sinf((float)count / (13 + 37 * i))) * (lcd.height() >> 2);
  }

  ++count;

  lcd.startWrite();
  int index1 = count % point_count;
  for (int x = 0; x < point_count-1; x++)
  {
    int index0 = index1;
    index1 = (index0 +1) % point_count;
    for (int i = 0; i < LINE_COUNT; i++)
    {
      int y = points[i][index0];
      if (y != points[i][index1])
      {
        lcd.writePixel(x, y + yoffset, getBaseColor(x, y));
      }
    }

    for (int i = 0; i < LINE_COUNT; i++)
    {
      int y = points[i][index1];
      lcd.writePixel(x, y + yoffset, colors[i]);
    }
  }
  lcd.endWrite();    
  }
    vTaskDelete(NULL);
}

/*** Display callback to flush the buffer to screen ***/
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.pushColors((uint16_t *)&color_p->full, w * h, true);
    lcd.endWrite();

    lv_disp_flush_ready(disp);
}
