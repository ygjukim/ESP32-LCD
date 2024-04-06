#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"

// I2C
#define I2C_SDA          GPIO_NUM_8
#define I2C_SCL          GPIO_NUM_9
#define I2C_RST          GPIO_NUM_7
#define I2C_INT          GPIO_NUM_NC
#define I2C_CLK_SPEED_HZ 100000
#define I2C_NUM          I2C_NUM_0

#define LCD_H_RES         320
#define LCD_V_RES         480

typedef enum {
    ROTATION_0=0, ROTATION_90, ROTATION_180, ROTATION_270, 
} TS_ROTATION;

static const char *TAG = "TOUCH";

esp_lcd_touch_handle_t tp;

static void i2c_scan()
{
  uint8_t foundDevices = 0;
  printf("Start i2c bus scanning...\r\n");
  for(int address = 1; address < 127; address++) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    if(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS) == ESP_OK) {
      printf("-> found device with address 0x%02x\r\n", address);
      foundDevices++;
    }
    i2c_cmd_link_delete(cmd);
  }
  if (foundDevices == 0)
  {
    printf("-> found NO devices\r\n");
  }
}

/**
 * @brief Initialize Touch Driver
 *
 * This function installs the touch driver, configures the I2C interface for touch communication,
 * initializes the touch controller, and creates a touch handle for touch input.
 *
 * @param[out] touch_handle Pointer to the handle for the initialized touch controller.
 */
static void init_touch(esp_lcd_touch_handle_t *touch_handle, TS_ROTATION ts_rotation) {

    ESP_LOGI(TAG, "Install Touch driver");
    
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_CLK_SPEED_HZ
    };
    ESP_LOGI(TAG, "i2c_param_config");
    i2c_param_config(I2C_NUM, &i2c_conf);
    ESP_LOGI(TAG, "i2c_driver_install");
    i2c_driver_install(I2C_NUM, i2c_conf.mode, 0, 0, 0);

    i2c_scan();

    /* Initialize touch */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = I2C_RST, 
        .int_gpio_num = I2C_INT,
        .levels = {
            .reset = 0,
            .interrupt = 1,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    switch (ts_rotation) {
        case ROTATION_90:
            tp_cfg.flags.mirror_x = 1;
            tp_cfg.flags.swap_xy = 1;
            break;
        case ROTATION_180:
            tp_cfg.flags.mirror_x = 1;
            tp_cfg.flags.mirror_y = 1;
            break;
        case ROTATION_270:
            tp_cfg.flags.mirror_y = 1;
            tp_cfg.flags.swap_xy = 1;
        default:
            break;
    }

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    ESP_LOGI(TAG, "Create LCD panel IO handle");
    esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM, &tp_io_config, &tp_io_handle);
    ESP_LOGI(TAG, "Create a new GT911 touch driver");
    esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, touch_handle);
}

/**
 * @brief Touchpad Read Function
 *
 * This function reads touchpad input and updates the LVGL input device data accordingly.
 *
 * @param[in] indev_driver Pointer to the LVGL input device driver structure.
 * @param[out] data Pointer to the LVGL input device data structure to be updated.
 */
static void touchpad_read(esp_lcd_touch_handle_t touch_handle)
{
    uint16_t touchpad_x;
    uint16_t touchpad_y;
    uint16_t touch_strength;
    uint8_t touch_cnt = 0;

    esp_lcd_touch_read_data(touch_handle);
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_handle, &touchpad_x, &touchpad_y, &touch_strength, &touch_cnt, 1);
    if (touchpad_pressed) {
        ESP_LOGI(TAG, "c: %d  x: %d  y: %d  s: %d", touch_cnt, touchpad_x, touchpad_y, touch_strength);
    }
}

void setup() {
    init_touch(&tp, ROTATION_270);
}

void app_main(void)
{
    setup();

    while (1) {
        touchpad_read(tp);
        vTaskDelay(25/portTICK_PERIOD_MS);  // pdMS_TO_TICKS(25)
    }
}