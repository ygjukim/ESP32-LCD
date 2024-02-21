/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>
//#include "LGFX_ESP32S3_RGB_ParallelTFTwithTouch40.h"
#include "LGFX_ILI9488_SPI_setting.h"
#include <GT911.h>

/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 *You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 *Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
 *as the examples and demos are now part of the main LVGL library. */

//#include <examples/lv_examples.h>
//#include <demos/lv_demos.h>

/*Set to your screen resolution*/
// #define TFT_HOR_RES   480
// #define TFT_VER_RES   480
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

void *draw_buf;
unsigned long lastTickMillis = 0;

LGFX tft;

// touch input device setup
#define TOUCH_INT_PIN    27
#define TOUCH_RST_PIN   -1
GT911 ts = GT911();

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
    /*Copy `px map` to the `area`*/

    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    tft.startWrite();
    // my_set_window(area->x1, area->y1, w, h);
    // my_draw_bitmaps(px_map, w * h);
    tft.setAddrWindow( area->x1, area->y1, w, h );
//    tft.writePixels((lgfx::rgb565_t *)px_map, w * h);
    tft.writePixels((uint16_t *)px_map, w * h);
    tft.endWrite();

    /*Call it to tell LVGL you are ready*/
    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    // uint16_t touchX, touchY;
    // bool touched = tft.getTouch( &touchX, &touchY );
    // if( !touched )
    // {
    //     data->state = LV_INDEV_STATE_REL;
    // }
    // else
    // {
    //     data->state = LV_INDEV_STATE_PR;

    //     /*Set the coordinates*/
    //     data->point.x = touchX;
    //     data->point.y = touchY;

    //     Serial.print( "Data x " );
    //     Serial.println( touchX );
    //     Serial.print( "Data y " );
    //     Serial.println( touchY );
    // }
    uint8_t touches = ts.touched(GT911_MODE_POLLING);
    if (touches <= 0) {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    else {
        GTPoint* tp = ts.getPoints();

        data->state = LV_INDEV_STATE_PRESSED;        

        /*Set the coordinates*/
        data->point.x = tp->x;
        data->point.y = tp->y;

        Serial.print( "Data x " );
        Serial.println( tp->x );
        Serial.print( "Data y " );
        Serial.println( tp->y );
    }
}

void setup()
{
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.begin( 115200 );
    Serial.println( LVGL_Arduino );

    tft.begin();            /* TFT init */
    tft.setRotation( 1 );   /* Landscape orientation, flipped */
    tft.setBrightness(255);

    ts.begin(TOUCH_INT_PIN, TOUCH_RST_PIN, GT911_I2C_ADDR_28);
    ts.setRotation(GT911::Rotate::_90);  
  
    lv_init();

    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    /*Else create a display yourself*/
    lv_display_t * disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    /*Initialize the touch input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

    /* Create a simple label
     * ---------------------
     lv_obj_t *label = lv_label_create( lv_scr_act() );
     lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
     lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

     * Try an example. See all the examples
     *  - Online: https://docs.lvgl.io/master/examples.html
     *  - Source codes: https://github.com/lvgl/lvgl/tree/master/examples
     * ----------------------------------------------------------------

     lv_example_btn_1();

     * Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMOS_WIDGETS
     * -------------------------------------------------------------------------------------------

     lv_demo_widgets();
     */

    lv_obj_t *label = lv_label_create( lv_scr_act() );
    lv_label_set_text( label, LVGL_Arduino.c_str() );
    lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

    //lv_demo_benchmark();

    Serial.println( "Setup done" );
}

void loop()
{
    uint32_t tickPeriod = millis() - lastTickMillis;
    lv_tick_inc(tickPeriod);
    lastTickMillis = millis();

    lv_timer_handler(); /* let the UI do its work */
}

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target_obj(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

/**
 * Create a button with a label and react on click event.
 */
void ui_init(void)
{
    lv_obj_t * btn = lv_btn_create(lv_scr_act());     /*Add a button the current screen*/
    // lv_obj_set_pos(btn, 10, 10);                   /*Set its position*/
    lv_obj_set_size(btn, 120, 50);                    /*Set its size*/
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL); /*Assign a callback to the button*/

    lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
    lv_label_set_text(label, "Button");               /*Set the labels text*/
    lv_obj_center(label);
}
