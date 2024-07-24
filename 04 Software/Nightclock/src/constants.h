#pragma once

#include <map>
#include <vector>

#define CLR_BACKGROUND       0x0000   // 00, 00, 00 = black
#define CLR_ITEM_BCK_UNSEL   0x6B4D   // 6A, 6A, 6A = dark grey
#define CLR_ITEM_TXT_UNSEL   0xD6BA   // D5, D5, D5 = light grey
#define CLR_GROUP_BCK_UNSEL  0x4290   // 44, 51, 87 = dark blue
#define CLR_GROUP_TXT_UNSEL  0xD6BA   // D5, D5, D5 = light grey
#define CLR_ITEM_BCK_SEL     0xB5B6   // B7, B7, B7 = light grey
#define CLR_ITEM_TXT_SEL     0x0000   // 00, 00, 00 = black
#define CLR_GROUP_BCK_SEL    0x6BF9   // 68, 7C, CB = light blue
#define CLR_GROUP_TXT_SEL    0xFFFF   // FF, FF, FF = white
#define CLR_RECT_SELECT      0xF800   // FF, 00, 00 = red
#define CLR_TITLE            0xB5A9   // B7, B7, 4F = yellow

#define SETTINGS_FILE "/settings.jsn"
#define SETTINGS_TEMP "/settings.tmp"

enum command_group_t { CMD_OK, CMD_CANCEL, CMD_DIAL_LOWER, CMD_DIAL_UPPER, CMD_DIAL_NUMBERS, CMD_DIAL_SYMBOLS, 
                       CMD_BACKSPACE, CMD_DEL, CMD_LEFT, CMD_RIGHT, CMD_SPACE, CMD_CHARACTER,
                       CMD_SET_CLOCK, CMD_SET_TIMEZONE, CMD_WAKE_UP_TIME, CMD_SLEEP_TIME,
                       CMD_SET_WIFI, CMD_REQ_WIFI_PWD, CMD_INFO, CMD_FACTORY_RESET, CMD_HOME };

struct command_t {
  command_group_t command;
  char character = ' ';
  command_t(const command_group_t cmd) : command(cmd) {};
  command_t(const command_group_t cmd, const char c) : command(cmd), character(c) {};
};

std::vector<uint8_t> BACKLIGHT_PWM_VALUE = { 1, 2, 3, 6, 12, 22, 40, 74, 138, 255 };

struct timezone_t {
  std::string label;
  std::string code;  
};

std::vector<timezone_t> TIMEZONES = {
  { "Fiji -12",                      "<+12>-12",                      },   // 0
  { "Sakhalin -11",                  "<+11>-11",                      },   // 1
  { "Brisbane -10",                  "AEST-10",                       },   // 2
  { "Tokyo -9",                      "JST-9",                         },   // 3
  { "Manila -8",                     "PST-8",                         },   // 4
  { "Bangkok -7",                    "<+07>-7",                       },   // 5
  { "Dhaka -6",                      "<+06>-6",                       },   // 6
  { "Almaty -5",                     "<+05>-5",                       },   // 7
  { "Dubai -4",                      "<+04>-4",                       },   // 8
  { "Damascus -3",                   "<+03>-3",                       },   // 9
  { "Istanbul -3",                   "<+03>-3",                       },   // 10
  { "Athene -2",                     "EET-2EEST,M3.5.0/3,M10.5.0/4",  },   // 11
  { "Moskou -2",                     "MSK-3",                         },   // 12
  { "Amsterdam -1",                  "CET-1CEST,M3.5.0,M10.5.0/3",    },   // 13   "CET-1CEST,M3.5.0,M10.5.0/3"
  { "London +0",                     "GMT0BST,M3.5.0/1,M10.5.0",      },   // 14
  { "Wie woont hier +1",             "<-01>1",                        },   // 15
  { "En hier dan +2",                "<-02>2",                        },   // 16
  { "Buenos Aires +3",               "<-03>3",                        },   // 17
  { "Aruba +4",                      "AST4",                          },   // 18
  { "Havana +5",                     "CST5CDT,M3.2.0/0,M11.1.0/1",    },   // 19
  { "New York +5",                   "EST5EDT,M3.2.0,M11.1.0",        },   // 20
  { "Chicago +6",                    "CST6CDT,M3.2.0,M11.1.0",        },   // 21
  { "Phoenix +7",                    "MST7",                          },   // 22
  { "Los Angeles +8",                "PST8PDT,M3.2.0,M11.1.0",        },   // 23
  { "Gambier +9",                    "<-09>9",                        },   // 24
  { "Honolulu +10",                  "HST10",                         },   // 25
};

const uint8_t DEFAULT_TIMEZONE = 13;

class milliSecTimer {
  public:
    unsigned long previous;
    unsigned long interval;
    bool autoReset; // Beware, reset happens only when calling lapsed(), and interval is not accurate
    
    // Constructor
    milliSecTimer(unsigned long interval, bool autoReset = true) {
      this->previous = millis();
      this->interval = interval;
      this->autoReset = autoReset;
    }
    
    void reset() { previous = millis(); }
      
    bool lapsed() {
      bool result = (millis() - previous >= interval );
      if(result and autoReset) reset(); // interval could be made more accurate with % operator 
      return result;
    }
};
