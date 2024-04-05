#include <Arduino.h>
#include <GT911.h>

#define TS_ADDR   0x5D
#define TS_INT    -1
#define TS_RST    7
#define TS_SDA    8   // 21
#define TS_SCL    9   // 22

GT911 ts = GT911();

void setup() {
  Serial.begin(115200);
  
  // ESP32 Devkit : SDA = 21, SCL = 22
  if (ts.begin(TS_INT, TS_RST, TS_ADDR, TS_SDA, TS_SCL)) {
    Serial.println("GT911 ts initialized successfully.");
  }
  else {
    Serial.println("GT911 ts doesn't be initialized.");
  }
}

void loop() {
  uint8_t touches = ts.touched(GT911_MODE_POLLING);

  if (touches) {
    GTPoint* tp = ts.getPoints();
    for (uint8_t  i = 0; i < touches; i++) {
      Serial.printf("#%d  %d,%d s:%d\n", tp[i].trackId, tp[i].x, tp[i].y, tp[i].area);
    }
  }
}
