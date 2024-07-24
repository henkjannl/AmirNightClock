#pragma once

#include <list>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#include <Arduino.h>
#include <TFT_eSPI.h>       

#include "constants.h"

/*
ToDo: use icons for left, right, delete, etc.
*/

int16_t pos_mod(int16_t a, int16_t b) {
    // Returns a positive modulo, even if a is negative
    if(b==0) return 0;
    return ((a % b) + b) % b;
};

struct dial_pad_t {
  command_group_t command;
  char character;
  std::string face;
  bool group = false;

  dial_pad_t(command_group_t cmd, char ch, std::string fc, const bool gr)
    : command(cmd), character(ch), face(fc), group(gr) {}

  dial_pad_t(command_group_t cmd, char ch, const char* fc, const bool gr)
    : command(cmd), character(ch), group(gr) {
    face = std::string(fc);
  }

  dial_pad_t(command_group_t cmd, char ch, char fc)
    : command(cmd), character(ch) {
    face = std::string(1, fc);
  }  

};

class DialWheel {
  protected:
    uint16_t selected_pad = 0;

    void calculate_dialpads() {

      // Create temporary screen object to determine text width
      TFT_eSPI tft = TFT_eSPI();  
      tft.setFreeFont(font);

      // Determine where middle of text of buttons should be written
      // By default, place the dial on the right hand side
      width = 0;
      for (auto & dial_pad : dial_pads) {
        width = std::max( (int) width,(int) tft.textWidth(dial_pad.face.c_str() ));
      }
      width+=2*button_padding;
      left = TFT_WIDTH - width;
    };

  public:
    std::vector<dial_pad_t> dial_pads;

    int32_t center_y = 120;
    int32_t button_padding = 2;

    uint16_t num_rotated_pads = 0;
    uint16_t left = 0;
    uint16_t width = 0;
    bool cycle_wheel = true;

    // Which font to use
    const GFXfont* font = &FreeSans12pt7b;

    void clear() {
      dial_pads.clear();
      calculate_dialpads();
    };

    void add_command(dial_pad_t new_dial_pad) {
      dial_pads.push_back(new_dial_pad);
      calculate_dialpads();
    };

    void add_standard_commands(std::list<dial_pad_t> new_dial_pads) {

      for (auto& new_dial_pad : new_dial_pads) 
        dial_pads.push_back(new_dial_pad);

      calculate_dialpads();
    };

    void add_characters(command_group_t command_group, std::string characters) {
      for (size_t i = 0; i < characters.length(); ++i) {
        dial_pad_t new_dial_pad = dial_pad_t(command_group, characters[i], characters[i]);
        dial_pads.push_back(new_dial_pad);
        }
      calculate_dialpads();
     };

    void selectItemByName(std::string name) {
      for(uint16_t i=0; i<dial_pads.size(); i++) {
        if(dial_pads[i].face.compare(name)==0) {
          selected_pad = i;
        }
      }
    } 

    void selectItemByIndex(uint8_t index) {
      selected_pad = index;
    } 

    std::string face() {
      return dial_pads[selected_pad].face;
    };

    uint16_t selected_index() {
      return selected_pad;
    };

    void rotate(int steps) {
      selected_pad = pos_mod( selected_pad+steps, dial_pads.size() );
    };

    dial_pad_t click() {
      int pad_id=0;

      if(dial_pads.empty()) return dial_pad_t(CMD_CANCEL, ' ', "", false);

      dial_pad_t result = *dial_pads.begin();
      for (auto & dial_pad : dial_pads) {
        if(pad_id==selected_pad) result=dial_pad;
        pad_id++;
      }
      return result;
    }

    void draw(TFT_eSPI tft) {

      // ToDo: animate movement

      if(dial_pads.empty()) return;

      tft.setFreeFont(font);
      tft.setTextColor(TFT_LIGHTGREY);

      tft.setTextDatum(MC_DATUM);

      // Middle of selected pad should end up in the middle of the screen
      int16_t btn_pitch = tft.fontHeight() + 2*button_padding + 1;
      int16_t offset_y = (int16_t) (1.0*center_y-(selected_pad-0.5)*btn_pitch);
      int16_t text_x = left + width/2;

      for (int id=-5; id<5; id++) {

        int rounded_pad = pos_mod(id+selected_pad, dial_pads.size() );  

        if(cycle_wheel or (rounded_pad == id+selected_pad) ) {
          dial_pad_t dial_pad = dial_pads.at(rounded_pad);
          uint16_t btn_color;
          uint16_t txt_color;

          if(id==0) {
            if (dial_pad.group) {
              // Group, selected
              btn_color = CLR_GROUP_BCK_SEL;
              txt_color = CLR_GROUP_TXT_SEL;
            }
            else {
              // Item, selected
              btn_color = CLR_ITEM_BCK_SEL;
              txt_color = CLR_ITEM_TXT_SEL;
            }
          }
          else {
            if (dial_pad.group) {
              // Group, not selected
              btn_color = CLR_GROUP_BCK_UNSEL;
              txt_color = CLR_GROUP_TXT_UNSEL;
            }
            else {
              // Item, not selected
              btn_color = CLR_ITEM_BCK_UNSEL;
              txt_color = CLR_ITEM_TXT_UNSEL;
            }
          }
  
          tft.fillSmoothRoundRect(left, center_y + (id-0.5)*btn_pitch, width, btn_pitch-1, 4, btn_color, CLR_BACKGROUND);
          tft.setTextColor(txt_color, btn_color, false);
          tft.drawString(dial_pad.face.c_str(), text_x, center_y + id*btn_pitch - 1);  
       }
      else {
        tft.fillRect(left, center_y + (id-0.5)*btn_pitch, width, btn_pitch-1, CLR_BACKGROUND);
      }; // if(cycle_wheel or (rounded_pad == id+selected_pad) ) 
    }; // for (int id=-5; id<5; id++) {
  }; // void draw(TFT_eSPI tft)
}; // DialWheel  


// Initialize the default command sets
std::list<dial_pad_t> DEFAULT_COMMANDS = {
  { CMD_OK,           ' ', "OK"      , false},
  { CMD_CANCEL,       ' ', "Cancel"  , false}, 
  { CMD_DIAL_LOWER,   ' ', "abc"     , true },
  { CMD_DIAL_UPPER,   ' ', "ABC"     , true },
  { CMD_DIAL_NUMBERS, ' ', "123"     , true },
  { CMD_DIAL_SYMBOLS, ' ', "!@#"     , true },
  { CMD_BACKSPACE,    ' ', "BCKSP"   , false}
};

std::list<dial_pad_t> LIMITED_DEFAULT_COMMANDS = {
  { CMD_OK,           ' ', "OK"      , false},
  { CMD_CANCEL,       ' ', "Cancel"  , false}, 
  { CMD_BACKSPACE,    ' ', "BCKSP"   , false} 
};

std::list<dial_pad_t> OK_CANCEL_COMMANDS = {
{ CMD_OK,           ' ', "OK"      , false},
{ CMD_CANCEL,       ' ', "Cancel"  , false}, 
};
