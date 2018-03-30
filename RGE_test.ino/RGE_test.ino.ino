#include "esp32_digital_led_lib.h"
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>


hw_timer_t * timer = NULL;

volatile byte blinked = 0;
volatile byte delayed = 0;
volatile int blinks = 300;

void IRAM_ATTR onTimer() {
  delayed = !delayed;
  blinked = 0;
}

const char* ssid  = WIFI_SSID;
const char* password  = WIFI_PASS;

const char* host = "hamraffl.es/rge";
const int port = 80;

const unsigned int rows = 13;
const unsigned int columns = 26;

const int ledPin = 32;
const int statusPin = 337;

const int refreshRate(300000);

unsigned int prevTime;
unsigned long sincePrev = 0;

#if defined(ARDUINO) && ARDUINO >= 100
// No extras
#elif defined(ARDUINO) // pre-1.0
// No extras
#elif defined(ESP_PLATFORM)
#include "arduinoish.hpp"
#endif

strand_t STRANDS[] = {
  { .rmtChannel = 0, .gpioNum = ledPin, .ledType = LED_WS2812B_V3, .brightLimit = 32, .numPixels = 338,
    .pixels = nullptr, ._stateVars = nullptr
  }
};
int STRANDCNT = 1;
strand_t * strands [] = { &STRANDS[0], &STRANDS[1], &STRANDS[2], &STRANDS[3] };

void setup() {

  delay(500);
  Serial.begin(115200);
  Serial.println("Initializing...");



  gpioSetup(ledPin, OUTPUT, LOW);
  Serial.println("GPIO Setup Complete");
  if (digitalLeds_initStrands(STRANDS, STRANDCNT)) {
    Serial.println("Init FAILURE: halting");
    while (true) {};
  }
  for (int i = 0; i < STRANDCNT; i++) {
    strand_t * pStrand = &STRANDS[i];
  }
  
  strands[0]->pixels[9] = pixelFromRGBW(255, 0, 0, 0);
  digitalLeds_updatePixels(strands[0]);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (attempts < 20) {
      delay(500);
      Serial.println(WiFi.status());
    }
    else {
      ESP.restart();
    }
    attempts++;
  }

  strands[0]->pixels[9] = pixelFromRGBW(0, 255, 0, 0);
  digitalLeds_updatePixels(strands[0]);

  arduinoOTAstuff();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  timer = timerBegin(0, 80, true);

  timerAttachInterrupt(timer, &onTimer, true);

  timerAlarmWrite(timer, 1000000, true);

  timerAlarmEnable(timer);

  Serial.println("Init complete");
}




void loop() {
  
  if (!delayed) {
    strands[0]->pixels[337] = pixelFromRGBW(0, 0, 0, 0);
    digitalLeds_updatePixels(strands[0]);
    if (blinks >= 300){
    if (WiFi.status() == WL_CONNECTED) {

      HTTPClient http;
      strand_t * strands [] = { &STRANDS[0], &STRANDS[1], &STRANDS[2], &STRANDS[3] };
      //Serial.println("Beginning!");
      http.begin(host, port);

      int httpCode = http.GET();
      Serial.println("Got a page!");
      Serial.println(httpCode);
      if (httpCode > 0) {
        Serial.println("[HTTP] GET... code: " + httpCode);

        if (httpCode == HTTP_CODE_OK) {

          String dataString = http.getString();
          char charArray[dataString.length()];

          dataString.toCharArray(charArray, dataString.length());

          int numbers[rows * columns] = { 0 };
          int curNum = 0;
          char *tok = strtok(charArray, " ");

          // Tokenize data from hamraffl.es
          while (tok) {
            numbers[curNum] = atoi(tok);
            curNum++;
            tok = strtok(NULL, " ");
          }
          digitalLeds_resetPixels(strands[0]);

          for (int i = 0; i < columns; i++) {
            if (i > 0) {
              strands[0]->pixels[i - 1] = pixelFromRGBW(0, 0, 0, 0);
            }
            strands[0]->pixels[i] = pixelFromRGBW(0, 0, 100, 0);
            digitalLeds_updatePixels(strands[0]);
            //Serial.println(i);
            delay(50);
          }

          digitalLeds_resetPixels(strands[0]);

          for (int i = 0; i < (sizeof(numbers) / sizeof(int)); i++) {

            if (numbers[i] > 0) {
              strands[0]->pixels[i] = colorPicker(numbers[i]);
            }
            else {
              strands[0]->pixels[i] = pixelFromRGBW(0, 0, 0, 0);
            }
          }
          digitalLeds_updatePixels(strands[0]);

        }
      }
      else {
        Serial.print("GET FAILED!");
        Serial.println(httpCode);
      }
      http.end();
      //delay(refreshRate);
      blinks = 0;
    }


    else {
      Serial.println("WiFi not connected!");
      delay(refreshRate);
    }
  }
  else {
    
    ArduinoOTA.handle();
    
  }
  }
  else {
    if (blinked == 0) {
    strands[0]->pixels[337] = pixelFromRGBW(32, 32, 0, 0);
    digitalLeds_updatePixels(strands[0]);
    Serial.println(blinks);
    blinks++;
      
      blinked = 1;
    }

  }
  
}



void gpioSetup(int gpioNum, int gpioMode, int gpioVal) {
#if defined(ARDUINO) && ARDUINO >= 100
  pinMode (gpioNum, gpioMode);
  digitalWrite (gpioNum, gpioVal);
#elif defined(ESP_PLATFORM)
  gpio_num_t gpioNumNative = static_cast<gpio_num_t>(gpioNum);
  gpio_mode_t gpioModeNative = static_cast<gpio_mode_t>(gpioMode);
  gpio_pad_select_gpio(gpioNumNative);
  gpio_set_direction(gpioNumNative, gpioModeNative);
  gpio_set_level(gpioNumNative, gpioVal);
#endif
}

pixelColor_t colorPicker(int percent) {
  pixelColor_t v;
  if (percent > 0 && percent < 10) {
    v.r = 0;
    v.g = 0;
    v.b = 64;
    v.w = 0;
  }
  else if (percent >= 10 && percent < 20) {
    v.r = 0;
    v.g = 64;
    v.b = 64;
    v.w = 0;
  }
  else if (percent >= 20 && percent < 30) {
    v.r = 0;
    v.g = 128;
    v.b = 64;
    v.w = 0;
  }
  else if (percent >= 30 && percent < 40) {
    v.r = 32;
    v.g = 64;
    v.b = 64;
    v.w = 0;
  }
  else if (percent >= 40 && percent < 50) {
    v.r = 64;
    v.g = 64;
    v.b = 64;
    v.w = 0;
  }
  else if (percent >= 50 && percent < 60) {
    v.r = 64;
    v.g = 32;
    v.b = 64;
    v.w = 0;
  }
  else if (percent >= 60 && percent < 70) {
    v.r = 64;
    v.g = 32;
    v.b = 32;
    v.w = 0;
  }
  else if (percent >= 70 && percent < 80) {
    v.r = 64;
    v.g = 32;
    v.b = 0;
    v.w = 0;
  }
  else if (percent > 80) {
    v.r = 64;
    v.g = 0;
    v.b = 0;
    v.w = 0;
  }

  return v;
}

void arduinoOTAstuff(){
    ArduinoOTA.onStart([]() {
    
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

