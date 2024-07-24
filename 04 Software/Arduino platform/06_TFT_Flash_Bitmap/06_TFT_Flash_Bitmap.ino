#include "arduino.h"
#include <TFT_eSPI.h>       // Hardware-specific library
#include "icon_sun.h"
#include "icon_moon.h"

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

void setup()
{
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(0);	// portrait

  tft.fillScreen(TFT_BLACK);

  tft.setSwapBytes(true);
}

void loop()
{
  for(uint16_t y=240; y>0; y--) {
    tft.pushImage(0, y, ICON_SUN_WIDTH, ICON_SUN_HEIGHT, ICON_SUN_PIXELS);
    tft.fillRect(0,y+ICON_SUN_HEIGHT+1,TFT_WIDTH,1,TFT_BLACK); 
  }

  for(uint16_t y=240; y>0; y--) {
    tft.pushImage(0, y, ICON_MOON_WIDTH, ICON_MOON_HEIGHT, ICON_MOON_PIXELS);
    tft.fillRect(0,y+ICON_MOON_HEIGHT+1,TFT_WIDTH,1,TFT_BLACK); 
  }

}
