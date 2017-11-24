#include "esp32_digital_led_lib.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid  = WIFI_SSID;
const char* password  = WIFI_PASS;

const char* host = "hamraffl.es";
const int port = 70;

const unsigned int rows = 13;
const unsigned int columns = 26;

const int refreshRate(600000);

#if defined(ARDUINO) && ARDUINO >= 100
// No extras
#elif defined(ARDUINO) // pre-1.0
// No extras
#elif defined(ESP_PLATFORM)
#include "arduinoish.hpp"
#endif

strand_t STRANDS[] = {
  { .rmtChannel = 0, .gpioNum = 17, .ledType = LED_WS2812B_V3, .brightLimit = 32, .numPixels = 338,
    .pixels = nullptr, ._stateVars = nullptr
  }
};
int STRANDCNT = 1;

class Scannerer {
  private:
    strand_t * pStrand;
    pixelColor_t minColor;
    pixelColor_t maxColor;
    int prevIdx;
    int currIdx;
  public:
    Scannerer(strand_t *, pixelColor_t);
    void drawNext();
};

Scannerer::Scannerer(strand_t * pStrandIn, pixelColor_t maxColorIn)
{
  pStrand = pStrandIn;
  minColor = pixelFromRGBW(0, 0, 0, 0);
  maxColor = maxColorIn;
  prevIdx = 0;
  currIdx = 0;
}

/*void Scannerer::drawNext()
{
  pStrand->pixels[prevIdx] = minColor;
  pStrand->pixels[currIdx] = maxColor;
  digitalLeds_updatePixels(pStrand);
  prevIdx = currIdx;
  currIdx++;
  if (currIdx >= pStrand->numPixels) {
    currIdx = 0;
  }
}

void scanners(strand_t * strands[], int numStrands, unsigned long delay_ms, unsigned long timeout_ms)
{
  //Scannerer scan(pStrand); Scannerer * pScanner = &scan;
  Scannerer * pScanner[numStrands];
  int i;
  uint8_t c = strands[0]->brightLimit; // TODO: improve
  pixelColor_t scanColors [] = {
    pixelFromRGBW(c, 0, 0, 0),
    pixelFromRGBW(0, c, 0, 0),
    pixelFromRGBW(c, c, 0, 0),
    pixelFromRGBW(0, 0, c, 0),
    pixelFromRGBW(c, 0, c, 0),
    pixelFromRGBW(0, c, c, 0),
    pixelFromRGBW(c, c, c, 0),
    pixelFromRGBW(0, 0, 0, c),
  };
  Serial.print("DEMO: scanners(");
  for (i = 0; i < numStrands; i++) {
    pScanner[i] = new Scannerer(strands[i], scanColors[i]);
    if (i > 0) {
      Serial.print(", ");
    }
    Serial.print("ch");
    Serial.print(strands[i]->rmtChannel);
    Serial.print(" (0x");
    Serial.print((uint32_t)pScanner[i], HEX);
    Serial.print(")");
    Serial.print(" #");
    Serial.print((uint32_t)scanColors[i].num, HEX);
  }
  Serial.print(")");
  Serial.println();
  unsigned long start_ms = millis();
  while (timeout_ms == 0 || (millis() - start_ms < timeout_ms)) {
    for (i = 0; i < numStrands; i++) {
      pScanner[i]->drawNext();
    }
    delay(delay_ms);
  }
  for (i = 0; i < numStrands; i++) {
    delete pScanner[i];
    digitalLeds_resetPixels(strands[i]);
  }
}

void scanner(strand_t * pStrand, unsigned long delay_ms, unsigned long timeout_ms)
{
  strand_t * strands [] = { pStrand };
  scanners(strands, 1, delay_ms, timeout_ms);
}
*/

void setup() {
  delay(500);
  Serial.begin(115200);
  Serial.println("Initializing...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  gpioSetup(17, OUTPUT, LOW);
  Serial.println("GPIO Setup Complete");
  if (digitalLeds_initStrands(STRANDS, STRANDCNT)) {
    Serial.println("Init FAILURE: halting");
    while (true) {};
  }
  for (int i = 0; i < STRANDCNT; i++) {
    strand_t * pStrand = &STRANDS[i];
    Serial.print("Strand ");
    Serial.print(i);
    Serial.print(" = ");
    Serial.print((uint32_t)(pStrand->pixels), HEX);
    Serial.println();
#if DEBUG_ESP32_DIGITAL_LED_LIB
    dumpDebugBuffer(-2, digitalLeds_debugBuffer);
#endif
    digitalLeds_resetPixels(pStrand);
#if DEBUG_ESP32_DIGITAL_LED_LIB
    dumpDebugBuffer(-1, digitalLeds_debugBuffer);
#endif
  }
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
    if (httpCode > 0) {
      //Serial.println("[HTTP] GET... code: "+httpCode);

      if (httpCode == HTTP_CODE_OK) {

        String dataString = http.getString();
        Serial.println( "dataString = ");
        Serial.println(dataString);
        char charArray[dataString.length()];
        dataString.toCharArray(charArray, dataString.length());
        int numbers[rows * columns];
        int curNum = 0;
        char *tok = strtok(charArray, " ");
        while (tok) {
          numbers[curNum] = atoi(tok);
          Serial.print("Token ");
          Serial.print(curNum);
          Serial.print(": ");
          Serial.println(tok);
          curNum++;
          tok = strtok(NULL, " ");
        }

        int fixedNumbers[rows * columns] = { 0 };


        // Remap the order of every other row, cause I'm dumb and soldered backwards
        for (int i = 0; i < rows; i++) {
          for (int j = 0; j < columns; j++) {
            if (i == 0 || i % 2 == 0) {
              fixedNumbers[(i * columns) + j] = numbers[(i * columns) + (columns - j)];
              /*Serial.print(i * (columns) + j);
                Serial.print(" Value: ");
                Serial.print(numbers[i * (columns - j)]);
                Serial.print(" maps to ");
                Serial.println((i * columns) + (columns - j));
              */
            }
            else {
              fixedNumbers[(i * columns) + j] = numbers[(i * columns) + j];
              /*Serial.print(i * (columns) + j);
                Serial.print(" Value: ");
                Serial.print(numbers[i * (columns - j)]);
                Serial.print(" maps to ");
                Serial.println((i * columns) + j);
              */
            }
          }
        }
        digitalLeds_resetPixels(strands[0]);
        for (int i = 0; i <= (rows * columns); i++) {
          strands[0]->pixels[i-1] = pixelFromRGBW(0, 0, 0, 0);
          strands[0]->pixels[i] = pixelFromRGBW(32, 0, 0, 0);
          digitalLeds_updatePixels(strands[0]);
          

          }


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
