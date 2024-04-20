#include <uxr/client/transport.h>

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "esp_log.h"
#include "sdkconfig.h"

// --- micro-ROS Transports ---
static char TAG[] = "tusb";
static uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];

#define TUSB_RX_RING_BUFSIZE    2048
RingbufHandle_t tusb_rx_ring_buf = NULL;

static void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read(itf, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret == ESP_OK) {
        if (xRingbufferSend(tusb_rx_ring_buf, (const void *)buf, rx_size, pdMS_TO_TICKS(10)) == pdFALSE) {
            ESP_LOGE(TAG, "ring buffer full or send timeout");
        };
    } else {
        ESP_LOGE(TAG, "received data read error");
    }
}

static void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    int dtr = event->line_state_changed_data.dtr;
    int rts = event->line_state_changed_data.rts;
    ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}

bool esp32_usb_cdcacm_open(struct uxrCustomTransport * transport) {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    /* the second way to register a callback */
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
                        TINYUSB_CDC_ACM_0,
                        CDC_EVENT_LINE_STATE_CHANGED,
                        &tinyusb_cdc_line_state_changed_callback));

    tusb_rx_ring_buf = xRingbufferCreateWithCaps(
                        TUSB_RX_RING_BUFSIZE, 
                        RINGBUF_TYPE_NOSPLIT,
                        MALLOC_CAP_INTERNAL);

    return true;
}

bool esp32_usb_cdcacm_close(struct uxrCustomTransport * transport){
    vRingbufferDeleteWithCaps(tusb_rx_ring_buf);
    return true;
}

size_t esp32_usb_cdcacm_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * err){
    const int txBytes = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, buf, len);
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    return txBytes;
}

size_t esp32_usb_cdcacm_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err){
    size_t item_size = 0;
//    char *item = xRingbufferReceiveUpTo(tusb_rx_ring_buf, &item_size, pdMS_TO_TICKS(timeout), len);
    char *item = xRingbufferReceive(tusb_rx_ring_buf, &item_size, pdMS_TO_TICKS(timeout));
    if (item != NULL) {
//      if (item_size > len)  item_size = len;    // => occure illegal message reception 
        memcpy(buf, (const void *)item, item_size);
        vRingbufferReturnItem(tusb_rx_ring_buf, (void *)item);
    }
    return item_size;
}