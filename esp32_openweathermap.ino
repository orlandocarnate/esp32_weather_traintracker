/*
  Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp32-http-get-open-weather-map-thingspeak-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
#include "Arduino.h"
#include "heltec.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <TimeLib.h>
#include "Creds.h"

const char* ssid = my_ssid;
const char* password = my_pass;

#define DEMO_DURATION 3000
typedef void (*Demo)(void);
int demoMode = 0;
int counter = 1;

String city = "Chicago";
String countryCode = "US";
String units = "imperial";

String openWeatherMapApiKey = my_openweathermap_key;
String openWeatherServerPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&units=" + units + "&APPID=" + openWeatherMapApiKey;

// THE DEFAULT TIMER IS SET TO 10 SECONDS FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 10 seconds (10000)
unsigned long timerDelay = 10000;

String jsonBuffer;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  // Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 30 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");

  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  // Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
}

JSONVar jsonParser(String apiPath) {
  Serial.println(apiPath);
  jsonBuffer = httpGETRequest(apiPath.c_str());
  Serial.println(jsonBuffer);

  return JSON.parse(jsonBuffer);
}

void weather() {

  // jsonBuffer = httpGETRequest(serverPath.c_str());
  // // Serial.println(jsonBuffer);
  // JSONVar myObject = JSON.parse(jsonBuffer);

  JSONVar myObject = jsonParser(openWeatherServerPath);

  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }

  Serial.print("JSON object = ");
  Serial.println(myObject);
  Serial.print("Temperature: ");
  Serial.println(myObject["main"]["temp"]);
  Serial.print("Pressure: ");
  Serial.println(myObject["main"]["pressure"]);
  Serial.print("Humidity: ");
  Serial.println(myObject["main"]["humidity"]);
  Serial.print("Wind Speed: ");
  Serial.println(myObject["wind"]["speed"]);

  // Display to OLED
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);

  String temperature =  "Temperature: ";
  temperature.concat(JSON.stringify(myObject["main"]["temp"]));

  String pressure = "Pressure: ";
  pressure.concat(JSON.stringify(myObject["main"]["pressure"]));

  String humidity = "Humidity: ";
  humidity.concat(JSON.stringify(myObject["main"]["humidity"]));

  String windspeed = "Wind Speed: ";
  windspeed.concat(JSON.stringify(myObject["wind"]["speed"]));

  String location = JSON.stringify(myObject["name"]);
  location.concat(", ");
  location.concat(JSON.stringify(myObject["sys"]["country"]));
  location.replace("\"", "");

  Heltec.display->drawString(0, 0, temperature);
  Heltec.display->drawString(0, 10, pressure);
  Heltec.display->drawString(0, 20, humidity);
  Heltec.display->drawString(0, 30, windspeed);
  Heltec.display->drawString(0, 40, location);
  Heltec.display->display();
}

time_t tmConvert_t(String timeString) {
  tmElements_t tmSet;
  int year, month, day, hour, minute, second;

  char newTime[timeString.length() + 1];

  timeString.toCharArray(newTime, timeString.length() + 1);

  sscanf(newTime, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
  Serial.println(newTime);
  Serial.println(year);
  Serial.println(month);
  Serial.println(day);
  Serial.println(hour);
  Serial.println(minute);
  Serial.println(second);

  tmSet.Year = year - 1970;
  tmSet.Month = month;
  tmSet.Day = day;
  tmSet.Hour = hour;
  tmSet.Minute = minute;
  tmSet.Second = second;

  return makeTime(tmSet); 
}

String estimatedTime(float arrT, float tmst) {
  int result = int(((arrT - tmst) / 60) + 0.5);
  if (result <= 1) {
    return "Due";
  }
  String eta = String(result);
  eta.concat("min");
  return eta;
}

// void displayRoute(routeObject) {}St

void trainTracker() {
  String stop1 = "30092"; // East bound
  String stop2 = "30093"; // West bound
  // CTA TRACKER API

  // String ctaBusKey = cta_bus_key;
  String ctaTrainKey = cta_train_key;
  String eastBound = "http://lapi.transitchicago.com/api/1.0/ttarrivals.aspx?key=" + ctaTrainKey + "&stpid=" + stop1 + "&outputType=JSON";
  String westBound = "http://lapi.transitchicago.com/api/1.0/ttarrivals.aspx?key=" + ctaTrainKey + "&stpid=" + stop2 + "&outputType=JSON";


  JSONVar eastBoundObject = jsonParser(eastBound);
  JSONVar westBoundObject = jsonParser(westBound);

  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(eastBound) == "undefined") {
    Serial.println("Parsing Eastbound Input failed!");
    return;
  }
  if (JSON.typeof(westBound) == "undefined") {
    Serial.println("Parsing Westbound Input failed!");
    return;
  }

  /*
  Ex:
  Next 'L' Services at Racine
  1. Blue Line #000 to
     O'Hare               2min

  2. Blue Line #001 to
     Lake Forest          3min
  */


  // Display to OLED

  // EastBound ---------------------------------------------------------------------
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  
  String timeStamp = JSON.stringify(eastBoundObject["ctatt"]["tmst"]);
  timeStamp.replace("\"", "");
  timeStamp.replace("T", " ");

  String arrivalTime = JSON.stringify(eastBoundObject["ctatt"]["eta"][0]["arrT"]);
  arrivalTime.replace("\"", "");
  arrivalTime.replace("T", " ");

  char newTime[timeStamp.length() + 1];
  timeStamp.toCharArray(newTime, timeStamp.length() + 1);

  time_t unixTime = tmConvert_t(timeStamp);
  Serial.println(unixTime);
  time_t arrivalUnixTime = tmConvert_t(arrivalTime);
  Serial.println(arrivalUnixTime);

  String eta = estimatedTime(arrivalUnixTime, unixTime);
  
  Serial.println(eta);

  // Print to Monitor
  Serial.println(eastBoundObject["ctatt"]["eta"][0]["rt"]);
  Serial.println(eastBoundObject["ctatt"]["eta"][0]["destNm"]);

  String route = JSON.stringify(eastBoundObject["ctatt"]["eta"][0]["rt"]);
  route.replace("\"", "");

  String routeNum = JSON.stringify(eastBoundObject["ctatt"]["eta"][0]["rn"]);
  routeNum.replace("\"", "");

  String destinationName = JSON.stringify(eastBoundObject["ctatt"]["eta"][0]["destNm"]);
  destinationName.replace("\"", "");
  
  String destination = route + " Line #" + routeNum + " to";
  
  // String arrT = JSON.stringify(eastBoundObject["ctatt"]["eta"][0]["arrT"]);
  // arrT.replace("\"", "");

  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, destination);

  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 10, destinationName);

  // The coordinates define the right end of the text
  Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
  Heltec.display->drawString(128, 10, eta);

  // West bound -----------------------------------------------------------------
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);

  timeStamp = JSON.stringify(westBoundObject["ctatt"]["tmst"]);
  timeStamp.replace("\"", "");
  timeStamp.replace("T", " ");

  arrivalTime = JSON.stringify(westBoundObject["ctatt"]["eta"][0]["arrT"]);
  arrivalTime.replace("\"", "");
  arrivalTime.replace("T", " ");

  char newWestBoundTime[timeStamp.length() + 1];
  timeStamp.toCharArray(newWestBoundTime, timeStamp.length() + 1);

  time_t unixWestBoundTime = tmConvert_t(timeStamp);
  Serial.println(unixWestBoundTime);
  time_t arrivalWestBoundUnixTime = tmConvert_t(arrivalTime);
  Serial.println(arrivalWestBoundUnixTime);

  eta = estimatedTime(arrivalWestBoundUnixTime, unixWestBoundTime);
  Serial.println(eta);

  Serial.println(westBoundObject["ctatt"]["eta"][0]["rt"]);
  Serial.println(westBoundObject["ctatt"]["eta"][0]["destNm"]);

  route = JSON.stringify(westBoundObject["ctatt"]["eta"][0]["rt"]);
  route.replace("\"", "");

  routeNum = JSON.stringify(westBoundObject["ctatt"]["eta"][0]["rn"]);
  routeNum.replace("\"", "");

  destinationName = JSON.stringify(westBoundObject["ctatt"]["eta"][0]["destNm"]);
  destinationName.replace("\"", "");
  
  destination = route + " Line #" + routeNum + " to";

  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 30, destination);

  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 40, destinationName);

  // The coordinates define the right end of the text
  Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
  Heltec.display->drawString(128, 40, eta);

  Heltec.display->display();

}

void drawTextAlignmentDemo() {
    // Text alignment demo
  Heltec.display->setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawString(0, 10, "Left aligned (0,10)");

  // The coordinates define the center of the text
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(64, 22, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
  Heltec.display->drawString(128, 33, "Right aligned (128,33)");
}

Demo demos[] = {trainTracker, weather};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;


String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void loop() {
  // Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
    
    // clear the display
    Heltec.display->clear();

    demos[demoMode]();

    Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
    Heltec.display->drawString(10, 128, String(millis()));
    // write the buffer to the display
    Heltec.display->display();

    if (millis() - timeSinceLastModeSwitch > DEMO_DURATION) {
    demoMode = (demoMode + 1)  % demoLength;
    timeSinceLastModeSwitch = millis();
    }
    counter++;
    delay(timerDelay);
  }
  else {
    Serial.println("WiFi Disconnected");
    // Heltec.display->drawString(0, 30, "Wifi disconnected!");
  }

}
