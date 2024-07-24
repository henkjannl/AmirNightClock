#pragma once

#include <list>
#include <string>
#include <algorithm>

#include <Arduino.h>
#include <TFT_eSPI.h>       

#include "constants.h"

class TextInput {
  private:
    std::string text = "";

  public:
    // Margins need to ensure the current character is visible
    int16_t margin_right = 190;
    int16_t y_offset = 112;
    int16_t padding = 2;

    // Which font to use
    const GFXfont* font = &FreeSans12pt7b;

    void clear() {
      text = "";
    };
 
    void add_char(char c) {
      text+=c;
    };

    void back_space() {
      if(text.size()<1) return;
      text.pop_back();
    };

    std::string result() {
      return text;  
    };

    void draw(TFT_eSPI tft) {

      tft.setFreeFont(font);
      tft.setTextColor(CLR_ITEM_TXT_UNSEL);
      tft.setTextDatum(TR_DATUM);

      int16_t h = tft.fontHeight();
      int16_t w = tft.textWidth("X");

      // Rectangle for the new character
      tft.drawRect(margin_right-padding-w-padding, y_offset-padding, w+2*padding, h+2*padding, CLR_RECT_SELECT);

      // Erase previous characters
      tft.fillRect(0, y_offset-padding, margin_right-padding-w-padding, h+2*padding, CLR_BACKGROUND);

      // Draw text recorded so far
      tft.drawString(text.c_str(), margin_right-w-3*padding, y_offset);
    };
};
