#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>

// Define the pin for the relay
const int relayPin = D1;

// NTP Configuration
const char* ntpServerName = "pool.ntp.org";
const int gmtOffsetSec = 25200; // GMT offset for Thailand (7 hours)
const int daylightOffsetSec = 0;

// WiFiManager configuration
char customSSID[32] = "";
char customPassword[32] = "";

// JSON response URL
const char* jsonURL = "http://209.97.174.12/loadData";

// Declare the NTPClient object outside setup()
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerName, gmtOffsetSec, daylightOffsetSec);

void setup() {
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  //digitalWrite(relayPin, LOW);
  
  // Initialize WiFi using WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect(customSSID, customPassword);

  // Connect to NTP server
  timeClient.begin();
  timeClient.update();
}

void loop() {
  // Send HTTP GET request to JSON URL
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client; // Create a WiFiClient object
    HTTPClient http;
    
    // Use the WiFiClient object with HTTPClient
    http.begin(client, jsonURL);

    int httpResponseCode = http.GET();

    if (httpResponseCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);

      // Parse JSON
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String namecode = doc["response"][0][3].as<String>();
        String timeStr = doc["response"][0][4].as<String>();

        // Check if the namecode matches
        if (namecode == "LOCKER-01") {
          // Get current time from NTP server
          timeClient.update();
          String currentTime = timeClient.getFormattedTime();

          // Calculate the time difference in seconds
          unsigned long jsonTime = parseTime(timeStr);
          unsigned long currentTimeEpoch = timeClient.getEpochTime();
          long timeDifference = currentTimeEpoch - jsonTime;

          if (timeDifference >= 0 && timeDifference <= 60) {
            // Activate the relay (LOW)
            digitalWrite(relayPin, HIGH);
             Serial.println("Relay Status: Activated (H)");
            delay(5000); // Wait 5 seconds before checking again
          } else {
            // Deactivate the relay (HIGH)
            digitalWrite(relayPin, LOW);
             Serial.println("Relay Status: Activated (LOW)");
          }
        }
      }
    }

    http.end();
  }
}

unsigned long parseTime(String timeStr) {
  int hours = timeStr.substring(0, 2).toInt();
  int minutes = timeStr.substring(3, 5).toInt();
  int seconds = timeStr.substring(6, 8).toInt();

  return hours * 3600 + minutes * 60 + seconds;
}
