#include <Arduino.h>
#include <GT911.h>

GT911 ts = GT911();

void setup() {
  Serial.begin(115200);
  // ESP32 Devkit : IRQ = 26, RST = -1, ADDR = 0x14, SDA = 21, SCL = 22
  if (ts.begin(26, -1, 0x14)) {
    Serial.println("GT911 ts initialized successfully.");
    ts.setRotation(GT911::Rotate::_270);
  }
  else {
    Serial.println("GT911 ts doesn't be initialized.");
  }
}

void loop() {
  uint8_t touches = ts.touched(GT911_MODE_INTERRUPT);

  if (touches) {
    GTPoint* tp = ts.getPoints();
    for (uint8_t  i = 0; i < touches; i++) {
      Serial.printf("#%d  %d,%d s:%d\n", tp[i].trackId, tp[i].x, tp[i].y, tp[i].area);
    }
  }
}
