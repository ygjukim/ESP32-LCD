/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>

#include "LGFX_ESP32S3_RGB_SUNTONE_70.h"
//#include <examples/lv_examples.h>
//#include <demos/lv_demos.h>

/*Set to your screen resolution*/
#define TFT_HOR_RES   800
#define TFT_VER_RES   480

LGFX tft;

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
void *draw_buf;
unsigned long lastTickMillis = 0;

static lv_obj_t * kb;

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
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

/*Read the touchpad*/
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
   uint16_t x, y;
   bool touched = tft.getTouch( &x, &y);

   if( !touched ) {
      data->state = LV_INDEV_STATE_RELEASED;
   }
   else {
      data->state = LV_INDEV_STATE_PRESSED;

      /*Set the coordinates*/
      data->point.x = x;
      data->point.y = y;

    //  Serial.printf( "x: %d, y: %d\n ", x, y);
   }
}

void setup()
{
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.begin( 115200 );
    Serial.println( LVGL_Arduino );

    tft.begin();
    tft.setRotation(0);
    tft.setBrightness(255);
    tft.startWrite();

    lv_init();

    // /*Set a tick source so that LVGL will know how much time elapsed. */
    // lv_tick_set_cb(millis);

    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    
    lv_display_t * disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    /*Initialize the (dummy) input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

    // lv_obj_t *label = lv_label_create( lv_scr_act() );
    // lv_label_set_text( label, LVGL_Arduino.c_str() );
    // lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

    // lv_demo_benchmark();

    ui_init();

//    Serial.println( "Setup done" );
}

void loop()
{
    unsigned int tickPeriod = millis() - lastTickMillis;
    lv_tick_inc(tickPeriod);
    lastTickMillis = millis();

    lv_task_handler(); /* let the GUI do its work */
}

void ui_init()
{
    static lv_style_t style_base;
    lv_style_init(&style_base);
    lv_style_set_border_width(&style_base, 0);

    lv_obj_t * screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(screen, TFT_HOR_RES, TFT_VER_RES);
    lv_obj_center(screen);
    lv_obj_add_style(screen, &style_base, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    LV_IMAGE_DECLARE(img_bg);
    lv_obj_t * main_bg = lv_image_create(screen);
    lv_image_set_src(main_bg, &img_bg);
    lv_obj_align(main_bg, LV_ALIGN_CENTER, 0, 0);
    
    LV_IMAGE_DECLARE(img_user);
    lv_obj_t * user_placeholder = lv_image_create(screen);
    lv_image_set_src(user_placeholder, &img_user);
    lv_obj_align(user_placeholder, LV_ALIGN_TOP_MID, 0, 70);

    lv_obj_t * user_label = lv_label_create(screen);
    lv_label_set_text(user_label, "Windows Inside");
    lv_obj_set_style_text_color(user_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(user_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(user_label, LV_ALIGN_CENTER, 0, -30);

    static lv_style_t style_textarea;
    lv_style_init(&style_base);
    lv_style_set_border_width(&style_base, 2);
    lv_style_set_border_color(&style_textarea, lv_color_hex(0xffffff));
    lv_style_set_radius(&style_textarea, 0);
    lv_style_set_bg_color(&style_textarea, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&style_textarea, LV_OPA_70);
    lv_style_set_text_color(&style_textarea, lv_color_hex(0xffffff));

    lv_obj_t * user_password = lv_textarea_create(screen);
    lv_textarea_set_placeholder_text(user_password, "PIN");
    lv_textarea_set_one_line(user_password, true);
    lv_obj_set_size(user_password, 200, 40);
    lv_obj_align(user_password, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_style(user_password, &style_textarea, LV_PART_MAIN);
    lv_obj_add_event_cb(user_password, ta_event_cb, LV_EVENT_ALL, NULL);

//    lv_keyboard_set_textarea(kb, user_password);

    static lv_style_t style_panel;
    lv_style_init(&style_panel);
    lv_style_set_border_width(&style_panel, 0);
    lv_style_set_bg_opa(&style_panel, LV_OPA_TRANSP);
    lv_style_set_pad_all(&style_panel, 0);

    lv_obj_t * menu_panel = lv_obj_create(screen);
    lv_obj_set_size(menu_panel, 140, 70);
    lv_obj_align_to(menu_panel, user_password, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_style(menu_panel, &style_panel, LV_PART_MAIN);

    lv_obj_t * sign_in_label = lv_label_create(menu_panel);
    lv_label_set_text(sign_in_label, "Sign-in options");
    lv_obj_set_style_text_color(sign_in_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(sign_in_label, LV_ALIGN_TOP_MID, 0, 0);

    LV_IMAGE_DECLARE(icon_key);
    lv_obj_t * img_btn_key = lv_imagebutton_create(menu_panel);
    lv_imagebutton_set_src(img_btn_key, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &icon_key, NULL);
    lv_obj_align(img_btn_key, LV_ALIGN_BOTTOM_LEFT, 10, 0);

    LV_IMAGE_DECLARE(icon_numpad);
    lv_obj_t * img_btn_numpad = lv_imagebutton_create(menu_panel);
    lv_imagebutton_set_src(img_btn_numpad, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &icon_numpad, NULL);
    lv_obj_align(img_btn_numpad, LV_ALIGN_BOTTOM_RIGHT, -10, 0);

    kb = lv_keyboard_create(lv_screen_active());
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

}

static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if(code == LV_EVENT_READY) {
        LV_LOG_USER("Ready, current text: %s", lv_textarea_get_text(ta));
    }
}

