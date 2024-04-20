#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "other_tasks.h"

extern QueueHandle_t uiInputEventQueue;
extern QueueHandle_t uiOutputEventQueue;

void display_task(void *arg)
{
    event_message_t eventMsg;

    printf("Start display task...\n");

    while (1) {
        if (xQueueReceive(uiOutputEventQueue, &eventMsg, 0) == pdPASS) {
            printf("Display - Event type: %d, sub-type: %d, data: %f\n", eventMsg.type, eventMsg.sub_type, eventMsg.data);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else {
            printf("Display - UI Output event queue is empty!\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

}
