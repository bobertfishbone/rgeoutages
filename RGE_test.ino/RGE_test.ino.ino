#include "esp32_digital_led_lib.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid  = WIFI_SSID;
const char* password  = WIFI_PASS;

const char* host = "hamraffl.es";
const int port = 70;

const unsigned int rows = 13;
const unsigned int columns = 26;

const int ledPin = 32;

const int refreshRate(300000);

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

void setup() {

  delay(500);
  Serial.begin(19200);
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
  strand_t * strands [] = { &STRANDS[0], &STRANDS[1], &STRANDS[2], &STRANDS[3] };
  strands[0]->pixels[9] = pixelFromRGBW(255, 0, 0, 0);
  digitalLeds_updatePixels(strands[0]);

  WiFi.begin(ssid, password);

  int state = 0;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(25, (state) ? HIGH : LOW);
    state = !state;
    delay(500);
    Serial.println(WiFi.status());
  }

  strands[0]->pixels[9] = pixelFromRGBW(0, 255, 0, 0);
  digitalLeds_updatePixels(strands[0]);



  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Init complete");
}




void loop() {

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
    delay(refreshRate);
  }


  else {
    Serial.println("WiFi not connected!");
    delay(refreshRate);
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


