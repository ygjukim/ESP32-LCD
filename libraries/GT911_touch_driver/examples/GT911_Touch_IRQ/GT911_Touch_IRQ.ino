/*
  Touch_IRQ
*/

#include "GT911_touch_driver.h"

GT911TouchDriver touchDetector;

void gigaTouchHandler(uint8_t contacts, GDTpoint_t* points) {
  Serial.print("Contacts: ");
  Serial.println(contacts);

  if (contacts > 0) {
    /* First touch point */
    Serial.print(points[0].x);
    Serial.print(" ");
    Serial.println(points[0].y);
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial) {}

  if (touchDetector.begin()) {
    Serial.println("Touch controller init - OK");
  } else {
    Serial.println("Touch controller init - FAILED");
    while(1) ;
  }

  touchDetector.onDetect(gigaTouchHandler);
}

void loop() { }