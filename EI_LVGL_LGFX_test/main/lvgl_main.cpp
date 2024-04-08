/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include <lvgl.h>
#include "LGFX_ESP32S3_ILI9488_GT911_35.h"

//#include <examples/lv_examples.h>
//#include <demos/lv_demos.h>

static const char* TAG = "LVGL";

LGFX tft;

/*Set to your screen resolution*/
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

void *draw_buf;
unsigned long lastTickMillis = 0;

SemaphoreHandle_t lvgl_mux;

// LVGL
#define LVGL_TASK_DELAY_MS   10
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY   5 // tskIDLE_PRIORITY   // 2

#define millis() (unsigned long)(esp_timer_get_time() / 1000ULL)

extern "C" {

static void lvgl_port_task(void *arg);
void ui_init();

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map )
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    lv_draw_sw_rgb565_swap(px_map, w*h);
       
    // if (tft.getStartCount() == 0)
    // {   // Processing if not yet started
    //     tft.startWrite();
    // }

    // tft.pushImageDMA( area->x1
    //         , area->y1
    //         , area->x2 - area->x1 + 1
    //         , area->y2 - area->y1 + 1
    //         ,(uint16_t*) px_map); 
    tft.pushImageDMA( area->x1, area->y1, w, h, (uint16_t*) px_map); 
    // tft.pushImage(area->x1, area->y1, w, h, px_map);

    // tft.startWrite();
    // tft.setAddrWindow( area->x1, area->y1, w, h );
    // tft.writePixels((lgfx::rgb565_t *)px_map, w * h);
    // tft.endWrite();

    /*Call it to tell LVGL you are ready*/
    lv_disp_flush_ready(disp);
}

void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    int32_t x, y;
    if (tft.getTouch(&x, &y)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
//        Serial.printf("x: %d, y: %d\n", data->point.x, data->point.y);
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void setup()
{
    lvgl_mux = xSemaphoreCreateRecursiveMutex();

    // initialize TFT-LCD driver
    tft.begin();
    tft.setRotation(3);
    tft.setBrightness(255);
    tft.startWrite();
    tft.setTouchCalibrate(tsCalibrationData);
  
    lv_init();

    /*Set a tick source so that LVGL will know how much time elapsed. */
    //lv_tick_set_cb(millis);

    /* register print function for debugging */
    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    lv_display_t * disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    /*Initialize the (dummy) input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

//    lv_demo_benchmark();
//    lv_example_get_started_4();
//    lv_example_event_4();
//    lv_example_observer_4();
    ui_init();

    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
}

static void lvgl_port_task(void *arg)
{
    unsigned long tickPeriod;

    ESP_LOGI(TAG, "Starting LVGL task");

    while (1) {
        tickPeriod = millis() - lastTickMillis;
        lv_tick_inc(tickPeriod);
        lastTickMillis = millis();

        xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
        lv_task_handler(); /* let the GUI do its work */
        xSemaphoreGiveRecursive(lvgl_mux);

        vTaskDelay(10/portTICK_PERIOD_MS);      // for feeding watchdog timer in IDLE task
    }
}

static uint32_t size = 0;
static bool size_dec = false;

static void timer_cb(lv_timer_t * timer)
{
  xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
  lv_obj_invalidate((const lv_obj_t *)timer->user_data);
  xSemaphoreGiveRecursive(lvgl_mux);

  if (size_dec) size--;
  else size++;

  if (size == 100) size_dec = true;
  else if (size == 0)  size_dec = false; 
}

static void event_cb(lv_event_t * e) 
{
  lv_obj_t * obj = lv_event_get_target_obj(e);
  lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t *)draw_task->draw_dsc;
  if (base_dsc->part == LV_PART_MAIN) {
    lv_draw_rect_dsc_t draw_dsc;
    lv_draw_rect_dsc_init(&draw_dsc);
    draw_dsc.bg_color = lv_color_hex(0xffaaaa);
    draw_dsc.radius = LV_RADIUS_CIRCLE;
    draw_dsc.border_color = lv_color_hex(0xff5555);
    draw_dsc.border_width = 2;
    draw_dsc.outline_color = lv_color_hex(0xff0000);
    draw_dsc.outline_width = 2;
    draw_dsc.outline_pad = 3;

    lv_area_t a;
    a.x1 = 0;
    a.y1 = 0;
    a.x2 = size;
    a.y2 = size;
    lv_area_align(&obj->coords, &a, LV_ALIGN_CENTER, 0, 0);

    xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
    lv_draw_rect(base_dsc->layer, &draw_dsc, &a);
    xSemaphoreGiveRecursive(lvgl_mux);
  }
}

void ui_init() 
{
  lv_obj_t * cont = lv_obj_create(lv_screen_active());
  lv_obj_set_size(cont, 200, 200);
  lv_obj_center(cont);
  lv_obj_add_event_cb(cont, event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_flag(cont, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

  lv_timer_create(timer_cb, 30, cont);
}

void app_main(void)
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    setup();
}

}   // extern "C"