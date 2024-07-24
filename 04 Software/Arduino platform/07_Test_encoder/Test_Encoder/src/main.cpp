#include <Arduino.h>

#include "Button2.h" //  https://github.com/LennartHennigs/Button2
#include "ESPRotary.h"

#define ROTARY_PIN1	16
#define ROTARY_PIN2	17
#define BUTTON_PIN	25

#define CLICKS_PER_STEP   4   // this number depends on your rotary encoder 

#define SERIAL_SPEED    115200

ESPRotary encoder;
Button2 button;

// on change
void rotate(ESPRotary& encoder) {
   Serial.println(encoder.getPosition());
}

// on left or right rotation
void showDirection(ESPRotary& encoder) {
  Serial.println(encoder.directionToString(encoder.getDirection()));
}
 
// single click
void click(Button2& btn) {
  Serial.println("Click!");
}

// long click
void resetPosition(Button2& btn) {
  encoder.resetPosition();
  Serial.println("Reset!");
}

void setup() {
  Serial.begin(SERIAL_SPEED);
  delay(50);
  Serial.println("\n\nSimple Counter");
  
  encoder.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP);
  encoder.setChangedHandler(rotate);
  encoder.setLeftRotationHandler(showDirection);
  encoder.setRightRotationHandler(showDirection);

  button.begin(BUTTON_PIN);
  button.setTapHandler(click);
  button.setLongClickHandler(resetPosition);
}

void loop() {
  encoder.loop();
  button.loop();
  delay(100);
}

