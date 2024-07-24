#include "SPI.h"
#include "TFT_eSPI.h"
#include "icon_sun.h"

TFT_eSPI tft = TFT_eSPI();

void setup(void) {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
}

void loop() {
  Serial.print(".");

  tft.fillScreen(TFT_RED);
  delay(200);

  tft.fillScreen(TFT_WHITE);
  delay(200);

  tft.fillScreen(TFT_BLACK);
  delay(200);
  // void TFT_eSPI::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor)

  tft.drawBitmap(0, 0, ICON_SUN_PIXELS, ICON_SUN_WIDTH, ICON_SUN_HEIGHT, TFT_WHITE, TFT_BLACK);
  delay(1000);
}
