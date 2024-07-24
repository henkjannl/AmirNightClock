#include <Arduino.h>
#include <map>
#include <vector>
#include <memory>
#include "time.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <NTPClient.h>

#include <FS.h>
#include <SPIFFS.h>

#include <TFT_eSPI.h>       
#include "Button2.h" 
#include "ESPRotary.h"

#include "constants.h"
#include "settings.h"
// #include "icon_sun.h"
// #include "icon_moon.h"
#include "dial_wheel.h"
#include "text_input.h"
#include "screen.h"

#define ROTARY_PIN1	     17
#define ROTARY_PIN2	     16
#define BUTTON_PIN	     25
#define LED_PIN          32
#define CLICKS_PER_STEP   4 
#define VERSION "1.0"

#define FORMAT_SPIFFS_IF_FAILED true

/*
Version history

1.0 Initial release

To do:
  test if clock is reliably synched
  test if sun icon is working reliably
  ensure time is valid -> only valid numbers in each next dialwheel
  icons for backspace, OK and cancel
  fonts in constants
*/

// GLOBAL CONSTANTS
// Hardware
WiFiMulti wifiMulti;
TFT_eSPI tft = TFT_eSPI();  
ESPRotary encoder;
Button2 button;

// Settings
Settings settings = Settings(SETTINGS_FILE, SETTINGS_TEMP);

// Screens
ScreenHome           screenHome           = ScreenHome(settings);
ScreenDevicePassword screenDevicePassword = ScreenDevicePassword(settings);
ScreenOptions        screenOptions        = ScreenOptions(settings);
ScreenTime           screenCurrentTime    = ScreenTime(settings, CMD_SET_CLOCK);
ScreenTimeZone       screenTimeZone       = ScreenTimeZone(settings);
ScreenTime           screenWakeupTime     = ScreenTime(settings, CMD_WAKE_UP_TIME);
ScreenTime           screenSleepTime      = ScreenTime(settings, CMD_SLEEP_TIME);
ScreenWiFiStation    screenWiFiStation    = ScreenWiFiStation(settings);
ScreenWiFiPassword   screenWiFiPassword   = ScreenWiFiPassword(settings);
ScreenFactoryReset   screenFactoryReset   = ScreenFactoryReset(settings);
ScreenInfo           screenInfo           = ScreenInfo(settings);

// Screen currently displayed
Screen *currentScreen = &screenHome;

// List of WiFi stations
std::vector<wifi_station_t> wifi_stations;

// Flag to redraw the screen
bool redraw_screen = true;

// Flag to indicate that the clock is synchronized with the timeserver
bool clock_synched = false;

// Index of WiFi station for which a password is requested
uint8_t requestedWiFiIndex = 0;

// Some timers for different purposes
milliSecTimer clockCheckTimer(60*1000, true);
milliSecTimer updateIconTimer(1000, true);
milliSecTimer screentimeoutTimer(3*60*1000, false);

void syncTimeAndDisconnect() {
  if(clock_synched) return;

  Serial.println(TIMEZONES[settings.timezone_index].label.c_str());
  Serial.println(TIMEZONES[settings.timezone_index].code.c_str());
  Serial.printf("Wifi connected: %s\n", WiFi.status() == WL_CONNECTED ? "Yes" : "No");

  configTzTime(TIMEZONES[settings.timezone_index].code.c_str(), "time.google.com", "time.windows.com", "pool.ntp.org");
  // configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "time.google.com", "time.windows.com", "pool.ntp.org");

  time_t utc_time;
  struct tm * timeinfo;
  time( &utc_time );
  timeinfo = localtime( &utc_time );
  Serial.println(asctime(timeinfo));  

  if(timeinfo->tm_year > (2016 - 1900)) {
    clock_synched=true;
  }
  else {
    Serial.println("Failed to obtain time");
  }
};

// CALLBACK FUNCTIONS
// On change
void rotate(ESPRotary& encoder) {
  static int prev_enc;
  int enc = encoder.getPosition();
  currentScreen->rotate(enc-prev_enc);
  screentimeoutTimer.reset();
  prev_enc = enc;
  redraw_screen = true;
}
 
// Single click
void click(Button2& btn) {
  command_t cmd = currentScreen->click();
  screentimeoutTimer.reset();

  // screenHome
  if(currentScreen == &screenHome) {
    if (cmd.command == CMD_OK) {
      currentScreen = &screenDevicePassword;
      screenDevicePassword.reset();
      redraw_screen = true;
    }
  }

  // screenDevicePassword
  else if(currentScreen == &screenDevicePassword) {
    switch(cmd.command) {
      case CMD_OK:
        currentScreen = &screenOptions;
      break;
      
      case CMD_CANCEL:
        currentScreen = &screenHome;
      break;
    }
  } 

  // screenOptions
  else if(currentScreen == &screenOptions) {
    switch(cmd.command) {
      case CMD_SET_CLOCK:
        currentScreen = &screenCurrentTime;
        screenCurrentTime.reset();
        Serial.println(screenCurrentTime.digits);
      break;
      
      case CMD_SET_TIMEZONE:
        currentScreen = &screenTimeZone;
      break;
      
      case CMD_WAKE_UP_TIME:
        currentScreen = &screenWakeupTime;
        screenWakeupTime.reset();
      break;
      
      case CMD_SLEEP_TIME:
        currentScreen = &screenSleepTime;
        screenSleepTime.reset();
      break;
      
      case CMD_SET_WIFI:
        currentScreen = &screenWiFiStation;

        // Display wait message
        tft.fillRect(35,65,170,110,CLR_BACKGROUND);
        tft.setFreeFont(&FreeSans12pt7b);
        tft.setTextColor(CLR_GROUP_TXT_SEL);

        tft.setTextDatum(MC_DATUM);
        tft.drawString("Eventjens",    120,  85);
        tft.drawString("snuffelen",    120, 108);
        tft.drawString("naar lekkere", 120, 132);
        tft.drawString("verse wifi",   120, 155);

        scanNetworks(wifi_stations);
        for(auto& wifi_station : wifi_stations) {
          Serial.println(wifi_station.station_name.c_str() );
        }

        screenWiFiStation.reset(wifi_stations);
      break;

      case CMD_REQ_WIFI_PWD:
        currentScreen = &screenWiFiPassword;
        requestedWiFiIndex = (uint8_t) cmd.character;
        screenWiFiPassword.setTitle(wifi_stations[requestedWiFiIndex].station_name);
        Serial.printf("Requesting password for %s", wifi_stations[requestedWiFiIndex].station_name);
      break;
      
      case CMD_INFO:
        screenInfo.setInfo(WiFi.status() == WL_CONNECTED, clock_synched, WiFi.SSID().c_str() );
        currentScreen = &screenInfo;
      break;

      case CMD_FACTORY_RESET:
        currentScreen = &screenFactoryReset;
      break;
      
      default:
        currentScreen = &screenHome;
    } // switch(cmd.command)
  } // currentScreen == &screenOptions)

  // screenCurrentTime
  else if(currentScreen == &screenCurrentTime) {
    switch(cmd.command) {
      case CMD_OK:
        currentScreen = &screenOptions;
      break;

      case CMD_SET_CLOCK:
        // Time was synched manually
        clock_synched = true;
        currentScreen = &screenOptions;
      break;
    }
  }

  // screenSleepTime
  else if(currentScreen == &screenSleepTime) {
    if(cmd.command==CMD_OK) currentScreen = &screenOptions;
  }

  // screenWakeupTime
  else if(currentScreen == &screenWakeupTime) {
    if(cmd.command==CMD_OK) currentScreen = &screenOptions;
  }

  // screenTimeZone
  else if(currentScreen== &screenTimeZone) {
    if(cmd.command==CMD_OK) {
      clock_synched=false;
      syncTimeAndDisconnect();
      currentScreen = &screenOptions;
    }
  }

  // screenWiFiStation
  else if(currentScreen== &screenWiFiStation) {
    requestedWiFiIndex = (uint8_t) cmd.character;
    Serial.printf("Requesting password for %s\n", wifi_stations[requestedWiFiIndex].station_name.c_str() );
    screenWiFiPassword.setTitle(wifi_stations[requestedWiFiIndex].station_name);
    currentScreen = &screenWiFiPassword;
  }

  // screenWiFiPassword
  else if(currentScreen== &screenWiFiPassword) {
    
    wifi_station_t wifi_station;

    switch(cmd.command) {
      case CMD_OK:
        // Potentially, this SSID already exists in wifiMulti.APlist, 
        // but we do not have access to this list
        // upon next restart, every SSID will only occur once in the list
        wifi_station = wifi_stations[requestedWiFiIndex];
        Serial.printf("We received password %s\n", settings.wifi_passwords[wifi_station.station_name].c_str());
        wifiMulti.addAP(wifi_station.station_name.c_str(), settings.wifi_passwords[wifi_station.station_name].c_str() );
        wifiMulti.run();
        Serial.println( (wifiMulti.run() == WL_CONNECTED) ? "Connected" : "Not connected");
        syncTimeAndDisconnect();

        currentScreen = &screenOptions;
      break;

      case CMD_CANCEL:
        currentScreen = &screenOptions;
      break;
    }
  }

  // screenInfo
  else if(currentScreen == &screenInfo) {
    currentScreen = &screenOptions;
  }

  // screenFactoryReset
  else if(currentScreen== &screenFactoryReset) {
    if(cmd.command==CMD_OK) currentScreen = &screenOptions;
  }

  redraw_screen = true;
} // void click(Button2& btn) {


void setup() {
  Serial.begin(115200);

  delay(50);
  Serial.println("\n\nNight clock Amir");

  SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);

  // Prepare backlight control
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);

  // Init screen
  tft.begin();
  tft.setRotation(2);	
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  // Show splash screen  
  tft.fillRect(35,65,170,110,CLR_BACKGROUND);
  tft.setFreeFont(&FreeSans12pt7b);
  tft.setTextColor(CLR_GROUP_TXT_SEL);

  tft.setTextDatum(MC_DATUM);
  tft.drawString( "Welkom",                                 120,  85);
  tft.drawString( "bij de nachtlamp",                       120, 108);
  tft.drawString( "van Amir",                               120, 132);
  tft.drawString( (std::string("versie ")+VERSION).c_str(), 120, 155);

  // Smoothly switch on backlight
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){   
      // changing the LED brightness with PWM
      analogWrite(LED_PIN, dutyCycle);
      delay(25);
    }

  // Configure encoder and button
  encoder.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP);
  encoder.setChangedHandler(rotate);

  button.begin(BUTTON_PIN);
  button.setTapHandler(click);

  // Load settings from file
  settings.load();

  // Set backlight on full power by default
  settings.backlight=BACKLIGHT_PWM_VALUE.size()-1;

  // Load wifi stations and try to connect
  for (const auto &wifi : settings.wifi_passwords) wifiMulti.addAP(wifi.first.c_str(), wifi.second.c_str());

  // Choose a short timeout. We can try again later
  Serial.println( (wifiMulti.run(1000) == WL_CONNECTED) ? "Connected" : "Not connected");

  syncTimeAndDisconnect();
};

void loop() {  

  // Periodically try to sync the clock if that was not done yet
  if (clockCheckTimer.lapsed()) {
    syncTimeAndDisconnect();    

    // Switch off wifi if clock is synched
    if(clock_synched) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);  
    }
  }

  // Periodically check if the moon or sun needs to change
  if(updateIconTimer.lapsed() and currentScreen == &screenHome) redraw_screen=true;

  // Jump back to the home screen at some point
  if(screentimeoutTimer.lapsed()) currentScreen = &screenHome;

  // Probe the button and the encoder
  encoder.loop();
  button.loop();

  // Modify the backlight
  static uint8_t prev_backlight = 255;
  if (settings.backlight != prev_backlight) {
    if(settings.backlight>BACKLIGHT_PWM_VALUE.size()-1) settings.backlight=BACKLIGHT_PWM_VALUE.size()-1;
    analogWrite(LED_PIN, BACKLIGHT_PWM_VALUE[settings.backlight]);
    prev_backlight = settings.backlight;
  }

  // Redraw the screen
  if (redraw_screen) {
    static Screen * prevScreen = nullptr;
    currentScreen->draw(tft, prevScreen != currentScreen);
    prevScreen = currentScreen;
    redraw_screen = false;
  }

};

