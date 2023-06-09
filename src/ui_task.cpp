#include <Arduino.h>

#include <ACAN_ESP32.h>

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
lv_obj_t *logbtn;

int getBaseColor(int x, int y)
{
  return ((x^y)&3 || ((x-xoffset)&31 && y&31) ? TFT_BLACK : ((!y || x == xoffset) ? TFT_WHITE : TFT_DARKGREEN));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
lv_obj_t *lbl;
lv_obj_t *lbl2;
void can_get_status(uint32_t &uiStandardFrames_, uint32_t &uiExtendedFrames_, uint32_t &error);

void lv_status_timer(lv_timer_t *timer) {
    char text[64];
    uint32_t uiFramesStandard;
    uint32_t uiFramesExtended;
    uint32_t uiError;
    can_get_status(uiFramesStandard, uiFramesExtended, uiError);
    sprintf(text, "Rcvd: %d/%d (Init: %02X)", uiFramesExtended+uiFramesStandard, uiFramesExtended, uiError);
    lv_label_set_text(lbl, text);
    EventBits_t bit = xEventGroupGetBits(notification_event);
    if (bit & EVENT_LOG_TO_LITTLEFS) {
        if ((lv_obj_get_state(logbtn) & LV_STATE_CHECKED) != LV_STATE_CHECKED) {
            lv_obj_add_state(logbtn, LV_STATE_CHECKED);
        }
    } else {
        if (lv_obj_get_state(logbtn) & LV_STATE_CHECKED) {
            lv_obj_clear_state(logbtn, LV_STATE_CHECKED);
        }
    }
    sprintf(text, "0x%02X", TWAI_STATUS_REG);
    lv_label_set_text(lbl2, text);
}

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

    lbl = lv_label_create(mainscreen);
    lv_obj_set_pos(lbl, 10, 100);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff),0);
    lv_label_set_text(lbl, "Test");

    lbl2 = lv_label_create(mainscreen);
    lv_obj_set_pos(lbl2, 10, 10);
    lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl2, lv_color_hex(0xffffff),0);
    lv_label_set_text(lbl2, "Status");

    lv_group_t *g;
    g = lv_group_create();
    logbtn = lv_btn_create(mainscreen);
    lv_obj_set_pos(logbtn, screenWidth - 60, 10);
    lv_obj_set_size(logbtn, 50, 30);
    lv_obj_add_flag(logbtn, LV_OBJ_FLAG_CHECKABLE);
    lv_group_add_obj(g, logbtn);
    lv_obj_add_event_cb(
      logbtn,
      [](lv_event_t *event) {
        if (lv_obj_get_state(event->target) & LV_STATE_CHECKED) {
            xEventGroupSetBits(notification_event, EVENT_LOG_TO_LITTLEFS);
        } else {
            xEventGroupClearBits(notification_event, EVENT_LOG_TO_LITTLEFS);
        }
      },
      LV_EVENT_VALUE_CHANGED, NULL);



    lv_obj_t *btn = lv_btn_create(mainscreen);
    lv_obj_set_pos(btn, screenWidth - 60, 45);
    lv_obj_set_size(btn, 50, 30);
    lv_group_add_obj(g, btn);
    lv_obj_add_event_cb(
      btn,
      [](lv_event_t *event) {
        xEventGroupSetBits(notification_event, EVENT_RESET_CAN);
      },
      LV_EVENT_CLICKED, NULL);

    lv_indev_set_group(lv_encoder_indev, g);
    lv_scr_load(mainscreen);

    lv_timer_t *timer = lv_timer_create(lv_status_timer, 500, NULL);
    lcd.setBrightness(255);
    while (1) {
        lv_timer_handler(); /* let the GUI do its work */
        if (globalCanQueue != nullptr) {

        }
        const portTickType xDelay = 10 / portTICK_RATE_MS;
        vTaskDelay(xDelay);
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
