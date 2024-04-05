/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>
#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#include <GT911.h>
#else
#include "LGFX_ESP32S3_ILI9488_GT911_35.h"
#endif

/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 *You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 *Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
 *as the examples and demos are now part of the main LVGL library. */

//#include <examples/lv_examples.h>
#include <demos/lv_demos.h>

#if !LV_USE_TFT_ESPI
LGFX tft;
#endif

/*Set to your screen resolution*/
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

void *draw_buf;
unsigned long lastTickMillis = 0;

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map )
{
    /*Copy `px map` to the `area`*/

    /*For example ("my_..." functions needs to be implemented by you)
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    my_set_window(area->x1, area->y1, w, h);
    my_draw_bitmaps(px_map, w * h);
     */
#if !LV_USE_TFT_ESPI
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
#endif

    /*Call it to tell LVGL you are ready*/
    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
#if LV_USE_TFT_ESPI
/* GT911 I2C Interface */
#define TS_I2C_ADDR     0x5D
#define TS_I2C_IRQ      -1
#define TS_I2C_RST      7
#define TS_I2C_SDA      8
#define TS_I2C_SCL      9

// Declare touch device
GT911 ts = GT911();

void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    uint8_t touches = ts.touched(GT911_MODE_INTERRUPT);

    if (touches) {
        GTPoint* tp = ts.getPoints();
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = tp->x;
        data->point.y = tp->y;
        // Serial.printf("x: %d, y: %d\n", data->point.x, data->point.y);
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
#else
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
#endif

void setup()
{
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.begin( 115200 );
    Serial.println( LVGL_Arduino );

#if !LV_USE_TFT_ESPI
    // initialize TFT-LCD driver
    tft.begin();
    tft.setRotation(3);
    tft.setBrightness(255);
    tft.startWrite();

    tft.setTouchCalibrate(tsCalibrationData);
#else
    // initialize GT911 touch driver
    if (ts.begin(TS_I2C_IRQ, TS_I2C_RST, TS_I2C_ADDR, TS_I2C_SDA, TS_I2C_SCL)) {
      ts.setRotation(GT911::Rotate::_270);
      Serial.println("GT911 ts initialized successfully.");
    }
    else {
      Serial.println("GT911 ts doesn't be initialized.");
    }
#endif
  
    lv_init();

    /*Set a tick source so that LVGL will know how much time elapsed. */
    //lv_tick_set_cb(millis);

    /* register print function for debugging */
    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
#if LV_USE_TFT_ESPI
    lv_display_t * disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
#else
    lv_display_t * disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

    /*Initialize the (dummy) input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

      lv_demo_benchmark();
//    lv_example_get_started_4();
//    lv_example_event_4();
//    lv_example_observer_4();
//    ui_init();

    Serial.println( "Setup done" );
}

void loop()
{
    unsigned long tickPeriod = millis() - lastTickMillis;
    lv_tick_inc(tickPeriod);
    lastTickMillis = millis();

    lv_task_handler(); /* let the GUI do its work */
}

static uint32_t size = 0;
static bool size_dec = false;

static void timer_cb(lv_timer_t * timer)
{
  lv_obj_invalidate((const lv_obj_t *)timer->user_data);
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

    lv_draw_rect(base_dsc->layer, &draw_dsc, &a);
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
