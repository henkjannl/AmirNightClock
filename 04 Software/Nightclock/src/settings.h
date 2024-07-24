#pragma once

#include <list>
#include <vector>
#include <map>
//#include <ctime>
#include <string>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

#include "constants.h"

/* To do:
Store and retrieve settings from non volatile memory
*/

// Type for storing wakeup time and sleep time
struct time_setting_t {
  uint8_t hour;
  uint8_t minute;
};

// Wifi stations
struct wifi_station_t {
  std::string station_name;
  int32_t signal_strength;
  wifi_station_t() : station_name(""), signal_strength(0) {} 
  wifi_station_t(std::string SSID, int8_t RSSI) : station_name(SSID), signal_strength(RSSI) {} 
};

// Scan for WiFi networks
// WiFi.scanNetworks will return the number of networks found
bool compareBySignal(const wifi_station_t& a, const wifi_station_t& b) {
  // Strongest signal first
  return a.signal_strength > b.signal_strength;
};

void scanNetworks(std::vector<wifi_station_t> &wifi_stations) {

  wifi_stations.clear();

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    wifi_station_t station = wifi_station_t( WiFi.SSID(i).c_str(), WiFi.RSSI(i) );
    wifi_stations.push_back(station);
  }

  std::sort(wifi_stations.begin(), wifi_stations.end(), compareBySignal);

  // Only retain the 20 strongest stations
  while(wifi_stations.size()>20) wifi_stations.pop_back();
};


class Settings {
  public:
    // Not stored in permanent memory
    std::string filename;
    std::string tempfile;

    // Stored in permanent memory
    time_setting_t wake_up_time;
    time_setting_t sleep_time;
    uint8_t timezone_index;
    uint8_t backlight = 5;
    std::map<std::string, std::string> wifi_passwords;

    Settings(std::string filename, std::string tempfile) {
      this->filename = filename;
      this->tempfile = tempfile;

      if ( !load() ) reset();
    };

    void reset() {    
      wake_up_time = {7,0};  
      sleep_time = {19,0};
      timezone_index = DEFAULT_TIMEZONE;
      backlight = 5;
      wifi_passwords.clear();
    };

    void save() {
      // Saves the settings to a file
      fs::File output = SPIFFS.open(tempfile.c_str(), FILE_WRITE);

      if( output.isDirectory() ){
          Serial.printf("- temporary file %s is directory\n", tempfile.c_str());
          return;
      }
      if( !output ){
          Serial.printf("- failed to open temporary file %s for writing\n", tempfile.c_str());
          return;
      }

    // serializeJson(doc, output);
    JsonDocument doc;

    doc[ "wake_up_hour"   ] = (int) wake_up_time.hour;    
    doc[ "wake_up_min"    ] = (int) wake_up_time.minute;    
    doc[ "sleep_hour"     ] = (int) sleep_time.hour;    
    doc[ "sleep_min"      ] = (int) sleep_time.minute;    
    doc[ "timezone_index" ] = (int) timezone_index;

    JsonObject wifi = doc["wifi_passwords"].to<JsonObject>();
    // JsonObject nested = doc.createNestedObject("wifi_passwords");
    for (const auto& pair : wifi_passwords) {
      wifi[pair.first.c_str()] = pair.second.c_str();
    }

    if( serializeJson(doc, output) > 0) {
      // Assume success if at least one byte was written
  
      if( SPIFFS.remove(filename.c_str()) ) 
        Serial.printf("Previous settings file %s removed\n", filename.c_str());
      else 
        Serial.printf("Error removing previous settings file %s\n", filename.c_str());
        
      if( SPIFFS.rename(tempfile.c_str(), filename.c_str() ) )
        Serial.printf("Temporary file %s renamed to %s\n", tempfile.c_str(), filename.c_str() );
      else
        Serial.println("Renaming failed");
    }
    else
      Serial.println("Error during serialization");
    };

    bool load() { 
      fs::File input = SPIFFS.open(filename.c_str(), FILE_READ);

      if(!input || input.isDirectory()){
          Serial.printf("- failed to open settings file %s for reading\n", filename.c_str());
          return false;
      }
      
      JsonDocument doc;

      DeserializationError error = deserializeJson(doc, input);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
      }

      wake_up_time.hour   = doc["wake_up_hour"]; // 7
      wake_up_time.minute = doc["wake_up_min"]; // 30
      sleep_time.hour     = doc["sleep_hour"]; // 22
      sleep_time.minute   = doc["sleep_min"]; // 45
      timezone_index      = doc["timezone_index"]; // 1

      JsonObject wifiPasswords = doc["wifi_passwords"];
      
      for (JsonPair kv : wifiPasswords) {
        wifi_passwords[kv.key().c_str()] = kv.value().as<const char*>();
      }
  
      return true; 
    };

};


