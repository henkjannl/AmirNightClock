#pragma once

#include <vector>

#include <Arduino.h>
#include <TFT_eSPI.h>       
#include <time.h>
#include <string>

#include "constants.h"
#include "dial_wheel.h"
#include "text_input.h"
#include "settings.h"

#include "icon_sun.h"
#include "icon_moon.h"
#include "sync_error.h"

class Screen {
  protected:
    Settings* settings;
    uint16_t active_element = 0;
  public:
    Screen(Settings& settings) : settings(&settings) {}
    virtual void rotate(int steps) {};
    virtual command_t click() { return command_t(CMD_OK); };
    virtual void draw(TFT_eSPI &tft, bool enforce) {
      if(enforce) tft.fillScreen(CLR_BACKGROUND);  
    };
    virtual ~Screen() {}
};

class ScreenHome : public Screen {
  public:
    ScreenHome(Settings& settings) : Screen(settings) {}

    void rotate(int steps) override {
      // In the homescreen, the dial changes the backlight intensity directly
      if( (steps>0) & (settings->backlight<BACKLIGHT_PWM_VALUE.size() ) ) settings->backlight++;
      if( (steps<0) & (settings->backlight>0) )                           settings->backlight--;
    };

    command_t click() override { return command_t(CMD_OK); };

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);

      time_t world_time;
      struct tm * now;

      enum icon_t { ICN_SUN, ICN_MOON, ICN_SYNC, ICON_INIT };
      icon_t iconToShow;
      static icon_t prevIcon = ICON_INIT;

      time (&world_time);
      now = localtime (&world_time);

      // Assume that the clock is not synched
      iconToShow = ICN_SYNC;

      if(now->tm_year > (2016 - 1900)) {
        // Clock is synched, determine if the icon should be sun or moon
        uint16_t now_min   = 60*     now->tm_hour           +      now->tm_min;
        uint16_t wake_min  = 60*settings->wake_up_time.hour + settings->wake_up_time.minute;
        uint16_t sleep_min = 60*settings->sleep_time.hour   + settings->sleep_time.minute;

        // Clock is synched, assume night
        iconToShow = ICN_MOON;

        if(sleep_min>wake_min) {
          // e.g., wake at 7:00 and sleep at 19:00
          if ((wake_min<=now_min) and (now_min<sleep_min)) iconToShow = ICN_SUN;
        }
        else {
          // e.g., wake at 19:00 and sleep at 7:00
          if((now_min>=wake_min) or (now_min<sleep_min)) iconToShow = ICN_SUN;
        }
      }

      // Determine if there is a need to draw
      if( enforce or (prevIcon != iconToShow) ) {
        tft.fillScreen(CLR_BACKGROUND);

        switch(iconToShow) {
          case ICN_SUN:
            tft.pushImage(47, 47, ICON_SUN_WIDTH, ICON_SUN_HEIGHT, ICON_SUN_PIXELS);
            break;

          case ICN_MOON:
            tft.pushImage(57, 57, ICON_MOON_WIDTH, ICON_MOON_HEIGHT, ICON_MOON_PIXELS);
            break;

          default:
            tft.pushImage(12, 12, SYNC_ERROR_WIDTH, SYNC_ERROR_HEIGHT, SYNC_ERROR_PIXELS);
          }
        }
      prevIcon = iconToShow;
    };
};

class ScreenDevicePassword: public Screen {
  protected: 
    DialWheel dialWheel;  
    std::string password = "";
    const std::string reference = std::string("IRIS");

  public:
    ScreenDevicePassword(Settings& settings): Screen(settings) {
      // Initialize the dialwheel  
      dialWheel = DialWheel();
      dialWheel.add_standard_commands(LIMITED_DEFAULT_COMMANDS);
      dialWheel.add_characters(CMD_CHARACTER, "AIMRS");  
    };  

    void reset() {
      password = "";
    };

    void rotate(int steps) override {
      dialWheel.rotate(steps);
    };

    command_t click() override { 

      dial_pad_t dial_pad = dialWheel.click(); 

      switch(dial_pad.command) {
        case CMD_OK:
          return (password.compare(reference)==0) ? command_t(CMD_OK) : command_t(CMD_CANCEL);
        break;

        case CMD_CANCEL:
          return command_t(CMD_CANCEL);
        break;

        case CMD_BACKSPACE:
          if(password.size()>0) password.pop_back();
          return command_t(CMD_CHARACTER);
        break;

        case CMD_CHARACTER:
          if(password.size()<4) password+=dial_pad.character;
          return (password.compare(reference)==0) ? command_t(CMD_OK) : command_t(CMD_CHARACTER);
        break;
      }

      return command_t(CMD_CANCEL);
    };

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);
      dialWheel.draw(tft);
      uint16_t left_box = 10;

      tft.setFreeFont(&FreeSansBold9pt7b);
      tft.setTextColor(CLR_TITLE);
      tft.setTextDatum(MR_DATUM);
      tft.drawString("Ontgrendel", dialWheel.left-4, 78);

      tft.setFreeFont(&FreeSansBold18pt7b);
      tft.setTextColor(CLR_GROUP_TXT_SEL);
      tft.setTextDatum(MC_DATUM);

      for(int i=0; i<4; i++) {
        uint16_t rct_color = (i==password.size()) ? CLR_RECT_SELECT : CLR_ITEM_BCK_UNSEL;
        tft.fillRect(left_box, 100, 25, 40, CLR_BACKGROUND);
        tft.drawRect(left_box, 100, 25, 40, rct_color);
        if(i<password.size()) {
          char c[2] = {password[i], '\0'};  
          tft.drawString(c, left_box+12, 120);
        }
        left_box += 30;
      }
    };
};

class ScreenOptions: public Screen {
  protected:
    DialWheel dialWheel;  

  public:
    ScreenOptions(Settings& settings): Screen(settings) {
      dialWheel.add_standard_commands(  std::list<dial_pad_t> {
        { CMD_WAKE_UP_TIME,  ' ', "Tijd om op te staan",     true },
        { CMD_SLEEP_TIME,    ' ', "Tijd om te gaan slapen",  true },
        { CMD_SET_CLOCK,     ' ', "Klok instellen",          true },
        { CMD_SET_TIMEZONE,  ' ', "Tijdzone instellen",      true },
        { CMD_SET_WIFI,      ' ', "Wifi",                    true },
        { CMD_INFO,          ' ', "Informatie",              true },
        { CMD_FACTORY_RESET, ' ', "Fabrieksinstellingen",    true },
        { CMD_HOME,          ' ', "Beginscherm",             true } 
      } );

      // Center the dialwheel from left to right
      dialWheel.left = (TFT_WIDTH - dialWheel.width)/2;
      dialWheel.cycle_wheel = false;
    };  

    void rotate(int steps) override {
      dialWheel.rotate(steps);
    };

    command_t click() override { 
      return dialWheel.click().command;
    };

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);
      dialWheel.draw(tft);
    };
};

class ScreenTime: public Screen {
  protected: 

  public:
    command_group_t target;
    char digits[8];
    std::map<uint8_t, DialWheel> dialWheel;
    uint8_t active_digit = 0;

    ScreenTime(Settings& settings, command_group_t target): Screen(settings) {
      this->target = target;

      // Initialize the dialwheels for the various digits
      dialWheel[0] = DialWheel();
      dialWheel[0].add_characters(CMD_CHARACTER, "012");  
      dialWheel[0].width = 40;
      dialWheel[0].left = TFT_WIDTH - dialWheel[0].width;

      dialWheel[1] = DialWheel();
      dialWheel[1].add_characters(CMD_CHARACTER, "0123456789");  
      dialWheel[1].width = 40;
      dialWheel[1].left = TFT_WIDTH - dialWheel[1].width;

      dialWheel[2] = DialWheel();
      dialWheel[2].add_characters(CMD_CHARACTER, "012345");  
      dialWheel[2].width = 40;
      dialWheel[2].left = TFT_WIDTH - dialWheel[2].width;

      dialWheel[3] = DialWheel();
      dialWheel[3].add_characters(CMD_CHARACTER, "0123456789");  
      dialWheel[3].width = 40;
      dialWheel[3].left = TFT_WIDTH - dialWheel[3].width;
    };  

    void reset() {
      switch(target) {
        case CMD_SET_CLOCK:
          // The screen is used to set the clock
          time_t world_time;
          struct tm * now;
          time (&world_time);
          now = localtime (&world_time);

          snprintf ( digits, 5, "%02d%02d", now->tm_hour, now->tm_min );
        break;

        case CMD_WAKE_UP_TIME:
          // The screen is used to set the wake up time
          snprintf ( digits, 5, "%02d%02d", settings->wake_up_time.hour, settings->wake_up_time.minute );
        break;

        case CMD_SLEEP_TIME:
          // The screen is used to set the sleep time
          snprintf ( digits, 5, "%02d%02d", settings->sleep_time.hour, settings->sleep_time.minute );
        break;
      }
      for(int digit=0; digit<4; digit++) {
        dialWheel[digit].selectItemByName( std::string("") + digits[digit]);
      }
      active_digit = 0;      
    };

    void rotate(int steps) override {
      if(active_digit<dialWheel.size()) dialWheel[active_digit].rotate(steps);
    };

    command_t click() override { 
      struct tm timeinfo;
      struct tm *time_ptr;
      time_t utc_time;
      time_t t;
      struct timeval now;
      char buf[20];

      // Store the digit from the dialwheel
      if(active_digit<dialWheel.size()) {
        // Copy the value of the active digit from the dialWheel
        digits[active_digit] = dialWheel[active_digit].click().character;
        active_digit++;
      }

      // We are not yet done, move to the next digit
      if(active_digit<4) return command_t(CMD_CHARACTER);

      // Store result
      char c[3];

      c[0]=digits[0];
      c[1]=digits[1];
      c[2]='\0';
      int hour = std::stoi(c);

      c[0]=digits[2];
      c[1]=digits[3];
      c[2]='\0';
      int min = std::stoi(c);

      switch(target) {
        case CMD_SET_CLOCK:
            time_ptr = localtime( &utc_time );

            time_ptr->tm_year = 2024 - 1900; // To prevent the computer chooses 1970 which may lead to new time sync
            time_ptr->tm_hour = hour;
            time_ptr->tm_min  = min;
            time_ptr->tm_sec  = 0;

            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", time_ptr->tm_hour, time_ptr->tm_min, time_ptr->tm_sec );
            Serial.printf("Trying to set the time to %s", buf);

            // Convert tm structure to time_t (epoch time)
            t = mktime(time_ptr);

            // Create a timeval structure
            now.tv_sec = t;
            now.tv_usec = 0;

            // Set the system time
            settimeofday(&now, NULL);

            struct tm timeinfo;
            time_ptr = localtime( &utc_time );
            snprintf(buf, sizeof( buf ), "%02d:%02d:%02d", time_ptr->tm_hour, time_ptr->tm_min, time_ptr->tm_sec );
            Serial.printf("This is the result %s", buf);

            return command_t(CMD_SET_CLOCK);
        break;

        case CMD_WAKE_UP_TIME:
          settings->wake_up_time.hour = hour;
          settings->wake_up_time.minute = min;
          settings->save();
        break;

        case CMD_SLEEP_TIME:
          settings->sleep_time.hour = hour;
          settings->sleep_time.minute = min;
          settings->save();
        break;
      }

      return command_t(CMD_OK);
    };

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);

      if(active_digit<dialWheel.size() ) dialWheel[active_digit].draw(tft);

      // Print a title above the screen
      std::string title;

      switch(target) {
        case CMD_SET_CLOCK:
          title = "Huidige tijd";
        break;

        case CMD_WAKE_UP_TIME:
          title = "Opstaan";
        break;

        case CMD_SLEEP_TIME:
          title = "Gaan slapen";
        break;
      }

      // Write the title
      tft.setFreeFont(&FreeSansBold9pt7b);
      tft.setTextColor(CLR_TITLE);
      tft.setTextDatum(MR_DATUM);
      tft.drawString(title.c_str(), 176, 78);

      // Write each digit
      tft.setFreeFont(&FreeSans18pt7b);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(CLR_GROUP_TXT_SEL);

      uint16_t left_box = 12;
      for(int i=0; i<4; i++) {
        uint16_t rct_color = (i==active_digit) ? CLR_RECT_SELECT : CLR_ITEM_BCK_UNSEL;
        tft.fillRect(left_box, 100, 25, 40, CLR_BACKGROUND);
        tft.drawRect(left_box, 100, 25, 40, rct_color);
        char c[2] = {digits[i], '\0'};  
        tft.drawString(c, left_box+12, 120);
        left_box += 40;

        if(i==1) {
          tft.drawString(":", left_box+8, 120);
          left_box += 16;
        };
      } // for i=0
    }; // void draw

}; // class ScreenTime


class ScreenTimeZone: public Screen {
  protected:
    DialWheel dialWheel;  

  public:
    ScreenTimeZone(Settings& settings): Screen(settings) {
      for (uint8_t i=0; i<TIMEZONES.size(); i++) {
          dial_pad_t dialpad = dial_pad_t(CMD_SET_TIMEZONE, (char) i, TIMEZONES[i].label, true);
          dialWheel.add_command(dialpad);
      }; 
      dialWheel.left = (TFT_WIDTH-dialWheel.width)/2;
      dialWheel.selectItemByIndex(settings.timezone_index);
    };

    void rotate(int steps) override {
      dialWheel.rotate(steps);
    };

    command_t click() override { 
      dial_pad_t dialpad = dialWheel.click();
      settings->timezone_index = (uint8_t) dialpad.character;
      settings->save();
      return command_t(CMD_OK);
    };

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);
      dialWheel.draw(tft);
    };
};

class ScreenWiFiStation: public Screen {
  protected:
    DialWheel dialWheel;  

  public:
    ScreenWiFiStation(Settings& settings): Screen(settings) {
      dialWheel.clear();
      dialWheel.cycle_wheel = false;
    }

    void reset(std::vector<wifi_station_t> &wifi_stations) {
      dialWheel.clear();

      for(int index = 0; index<wifi_stations.size(); index++) {
        dial_pad_t dialpad = dial_pad_t(CMD_SET_WIFI, (char) index, wifi_stations[index].station_name, true);
        dialWheel.add_command(dialpad);

        // Limit the list to the 20 strongest stations
        if(index>20) break;
      }

      // Put the dialwheel in the middle of the screen
      dialWheel.left = (TFT_WIDTH-dialWheel.width)/2;
    };

    void rotate(int steps) override {
      dialWheel.rotate(steps);
    };

    command_t click() override { 
      dial_pad_t dialpad = dialWheel.click();

      // Return the index of the selected WiFi station back to the main program
      return command_t(CMD_OK, dialpad.character);
    };  

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);
      dialWheel.draw(tft);
    };
};


class ScreenWiFiPassword: public Screen {
  protected:
    TextInput textInput;
    DialWheel dialWheelUpper;
    DialWheel dialWheelLower;
    DialWheel dialWheelNumbers;
    DialWheel dialWheelSymbols;
    DialWheel *currentDialWheel = &dialWheelUpper;
    std::string title;

  public:
    ScreenWiFiPassword(Settings& settings): Screen(settings) {
      dialWheelUpper.add_standard_commands(DEFAULT_COMMANDS);
      dialWheelUpper.add_characters(CMD_CHARACTER, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

      dialWheelLower.add_standard_commands(DEFAULT_COMMANDS);
      dialWheelLower.add_characters(CMD_CHARACTER, "abcdefghijklmnopqrstuvwxyz");

      dialWheelNumbers.add_standard_commands(DEFAULT_COMMANDS);
      dialWheelNumbers.add_characters(CMD_CHARACTER, "0123456789");

      dialWheelSymbols.add_standard_commands(DEFAULT_COMMANDS);
      dialWheelSymbols.add_characters(CMD_CHARACTER, "#_%$&<=>\"'()*+,-./:;?!@[\\]^~");
    };

    void setTitle(std::string station_name) {
      title = station_name;
      textInput.clear();
    };

    void rotate(int steps) override {
      currentDialWheel->rotate(steps);
    };

    command_t click() override { 
      dial_pad_t dialpad = currentDialWheel->click();

      switch(dialpad.command) {
       case CMD_OK:
         // Save the entered password in the settings
         // main program will take care of connecting to the WiFi router
         settings->wifi_passwords[title] = textInput.result();
         settings->save();
         return command_t(CMD_OK);
       break;

       case CMD_CANCEL:
          return command_t(CMD_CANCEL);
        break;

        case CMD_DIAL_LOWER:
          dialWheelLower.selectItemByName(currentDialWheel->face());
          currentDialWheel = &dialWheelLower;
        break;

        case CMD_DIAL_UPPER:
          dialWheelUpper.selectItemByName(currentDialWheel->face());
          currentDialWheel = &dialWheelUpper;
        break;

        case CMD_DIAL_NUMBERS:
          dialWheelNumbers.selectItemByName(currentDialWheel->face());
          currentDialWheel = &dialWheelNumbers;
        break;

        case CMD_DIAL_SYMBOLS:
          dialWheelSymbols.selectItemByName(currentDialWheel->face());
          currentDialWheel = &dialWheelSymbols;
        break;

        case CMD_BACKSPACE:
          textInput.back_space();
        break;

        case CMD_CHARACTER:
          textInput.add_char(dialpad.character);
        break;
      };

      return command_t(CMD_CHARACTER);    
    };  

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);

      // Draw dialwheel
      currentDialWheel->draw(tft);

      // Write the title
      tft.setFreeFont(&FreeSansBold9pt7b);
      tft.setTextColor(CLR_TITLE);
      tft.setTextDatum(BR_DATUM);
      tft.drawString(title.c_str(), currentDialWheel->left-4, textInput.y_offset-4);

      textInput.margin_right = currentDialWheel->left-4;
      textInput.draw(tft);
    };
};


class ScreenFactoryReset: public Screen {
  protected:
    DialWheel dialWheel;  

  public:
    ScreenFactoryReset(Settings& settings): Screen(settings) {
      dialWheel.add_standard_commands(OK_CANCEL_COMMANDS);
      dialWheel.cycle_wheel = false;
      dialWheel.left = (TFT_WIDTH-dialWheel.width)/2;
      dialWheel.center_y = 150;
    }

    void rotate(int steps) override {
      dialWheel.rotate(steps);
    };

    command_t click() override { 
      dial_pad_t dialpad = dialWheel.click();
      if(dialpad.command==CMD_OK) {
        Serial.println("Factory reset");
        // Reset to factory settings
        settings->reset();  
        settings->save();
      }
      return command_t(CMD_OK);    
    };  

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);

      dialWheel.draw(tft);

      // Write the title
      tft.setFreeFont(&FreeSansBold9pt7b);
      tft.setTextColor(CLR_TITLE);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("ALLES WISSEN?", 120, 85);
    };
};

class ScreenInfo: public Screen {
  protected:
    bool wifi_connected;
    bool time_synched;
    std::string station;


  public:
    ScreenInfo(Settings& settings): Screen(settings) {
    }; 

    void setInfo(bool conn, bool sync, std::string SSID) {
      wifi_connected = conn;
      time_synched = sync;
      station = SSID;
    }

    command_t click() override { 
      return command_t(CMD_OK);
    };

    void drawLabelAndText(TFT_eSPI &tft, uint16_t &y, const char* label, const char* text) {
      tft.setTextDatum(MC_DATUM);

      tft.setFreeFont(&FreeSans9pt7b);
      tft.setTextColor(CLR_TITLE);
      tft.drawString(label, 120, y);
      y+=tft.fontHeight();

      tft.setFreeFont(&FreeSansBold12pt7b);
      tft.setTextColor(CLR_GROUP_TXT_SEL);
      tft.drawString(text, 120, y);
      y+=tft.fontHeight();
    };

    void draw(TFT_eSPI &tft, bool enforce) override {
      Screen::draw(tft,enforce);

      std::string consyn = wifi_connected ? "Yes" : "No";
      consyn+= time_synched ? " / Yes" : " / No";

      time_t utc_time;
      struct tm * timeinfo;
      time( &utc_time );

      char loc[20];
      timeinfo = localtime( &utc_time );
      snprintf(loc, sizeof( loc ), "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );

      uint16_t y = 20;
      drawLabelAndText(tft, y, "Wifi station", station.c_str());
      drawLabelAndText(tft, y, "Connected/Synched", consyn.c_str() );
      drawLabelAndText(tft, y, TIMEZONES[settings->timezone_index].label.c_str(), loc);
      drawLabelAndText(tft, y, "Click to continue", "");
    };
};

