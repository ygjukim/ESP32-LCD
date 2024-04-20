#ifndef _OTHER_TASKS_H_
#define _OTHER_TASKS_H_

typedef struct {
    uint8_t type;
    uint8_t sub_type;
    double data;
} event_message_t;

void display_task(void *arg);

#endif  // _OTHER_TASKS_H_