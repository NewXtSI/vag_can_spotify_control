#include <Arduino.h>

#if !defined LGFX_AUTODETECT
  #define LGFX_AUTODETECT
#endif
#define LGFX_USE_V1     // set to use new version of library
#define LGFX_TTGO_TDISPLAY 

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
#include <lvgl.h>
#include "lv_conf.h"

#include "../include/config.h"
#include <vector>

#define screenWidth 240
#define screenHeight 135

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_indev_t *lv_encoder_indev;

extern QueueHandle_t globalCanQueue;
extern EventGroupHandle_t notification_event;

/*** Function declaration ***/
static void lv_encoder_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

#define LINE_COUNT 3

static LGFX lcd;    // declare display variable

static std::vector<int> points[LINE_COUNT];
static int colors[] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW };
static int xoffset, yoffset, point_count;

#include <ESP32CAN.h>

int getBaseColor(int x, int y)
{
  return ((x^y)&3 || ((x-xoffset)&31 && y&31) ? TFT_BLACK : ((!y || x == xoffset) ? TFT_WHITE : TFT_DARKGREEN));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void ui_task(void *param) {

    lcd.init();
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

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = lv_encoder_read;
    indev_drv.user_data = notification_event;
    lv_encoder_indev = lv_indev_drv_register(&indev_drv);


    lv_obj_t *mainscreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(mainscreen, lv_color_hex(0x303030),0);

    lv_obj_t *lbl = lv_label_create(mainscreen);
    lv_obj_set_pos(lbl, 10, 10);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff),0);
    lv_label_set_text(lbl, "Test");

    lv_group_t *g;
    g = lv_group_create();
    lv_obj_t *btn = lv_btn_create(mainscreen);
    lv_obj_set_pos(btn, screenWidth - 60, 10);
    lv_obj_set_size(btn, 50, 30);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_group_add_obj(g, btn);
    btn = lv_btn_create(mainscreen);
    lv_obj_set_pos(btn, screenWidth - 60, 45);
    lv_obj_set_size(btn, 50, 30);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_group_add_obj(g, btn);

    lv_indev_set_group(lv_encoder_indev, g);
    lv_scr_load(mainscreen);
      lcd.setBrightness(255);

    while (1) {
        lv_timer_handler(); /* let the GUI do its work */
        if (globalCanQueue != nullptr) {

        }
    }
    vTaskDelete(NULL);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

static void lv_encoder_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    EventGroupHandle_t *lv_input_event =
      (EventGroupHandle_t *)indev_drv->user_data;
    EventBits_t bit = xEventGroupGetBits(lv_input_event);
    static uint32_t last_btn = 0;
    int btnpressed = -1;
    if (bit & EVENT_JOYSTICK_UP) {
        btnpressed = LV_KEY_ENTER;
    } else {
        if (bit & EVENT_JOYSTICK_LEFT) {
            btnpressed = LV_KEY_LEFT;
        } else {
            if (bit & EVENT_JOYSTICK_RIGHT) {
                btnpressed = LV_KEY_RIGHT;
            }
        }
    }
    if (btnpressed >= 0) {
       last_btn = btnpressed;
       data->state = LV_INDEV_STATE_PR;
    } else {
       data->state = LV_INDEV_STATE_REL;
    }

    data->key = last_btn;
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
