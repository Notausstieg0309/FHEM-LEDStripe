
#include <ESP8266WiFi.h>
#include "config.h"

#if OTA_FLASH_ENABLE == 1
// neccessary for OTA flashing via Arduino IDE
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif

// the server object listening on the defined port
WiFiServer server(SERVER_PORT);

// control special effects
boolean fire = false;
boolean rainbow = false;
boolean blinker = false;
boolean sparks = false;
boolean white_sparks = false;
boolean knightrider = false;
boolean dim = false;
boolean segments = false;
boolean fadeIn = false;
boolean fadeOut = false;

// blinker specific variables
int xfrom;
int yto;
int myOn;
int myOff;
int myredLevel;
int mygreenLevel;
int myblueLevel;

// rainbow specific variables
boolean rainbow_drawn = false;
uint16_t rainbowColor = 0;

// dim specific values
int dimSteps;
int dimCurrentStep;
uint32_t dimTargetColor;
uint32_t dimInitColor[NUM_PIXELS];

// knightrider specific values
int cur_step = 0;

// reset the MCU at the end of the main loop
boolean do_reset = false;

// wait interval between each effect step
uint16_t delay_interval = 50;

// setup network and output pins
void setup() {

  // setup onboard LED pin for output
  pinMode(STATUS_LED_PIN, OUTPUT); 

  // light up onboard LED
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  Serial.println(F("Booting"));

  // Initialize all pixels to 'off'
  stripe_setup();
  WiFi.mode(WIFI_STA);

  #if USE_WLAN_SLEEP == 0
  // disable WiFi sleep
  wifi_set_sleep_type(NONE_SLEEP_T);
  #endif
  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_KEY);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(500);
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // OTA Stuff if enabled
  #if OTA_FLASH_ENABLE == 1
  
  #ifdef OTA_FLASH_HOSTNAME
    ArduinoOTA.setHostname(OTA_FLASH_HOSTNAME);
  #endif
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

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
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  #endif

  // start the webserver
  server.begin();
}

// request receive loop
void loop() {
  // handle OTA stuff if enabled
  #if OTA_FLASH_ENABLE == 1
  ArduinoOTA.handle();
  #endif
  
  // listen for incoming clients
  WiFiClient client = server.available();  // Check if a client has connected
  if (client) {
    Serial.println(F("new client"));
    Serial.print(F("remote ip: "));
    Serial.println(client.remoteIP());

    String inputLine = "";
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    boolean isGet = false;
    boolean isPost = false;
    boolean isPostData = false;
    int postDataLength;
    int ledix = 0;
    int tupel = 0;
    int redLevel = 0;
    int greenLevel = 0;
    int blueLevel = 0;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (isPostData && postDataLength > 0) {
          switch (tupel++) {
            case 0:
              redLevel = colorVal(c);
              break;
            case 1:
              greenLevel = colorVal(c);
              break;
            case 2:
              blueLevel = colorVal(c);
              tupel = 0;
              stripe_setPixelColor(ledix++, stripe_color(redLevel, greenLevel, blueLevel));
              break;
          }
          if (--postDataLength == 0) {
            stripe_show();
            sendOkResponse(client);
            break;
          }
        }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          if (isPost) {
            isPostData = true;
            continue;
          } else {
            // send http response
            if (isGet) {
              sendOkResponse(client);
            } else {
              client.println(F("HTTP/1.1 500 Invalid request"));
              client.println(F("Connection: close"));  // the connection will be closed after completion of the response
              client.println();
            }
            break;
          }
        }
        if (c == '\n') {
          // http starting a new line, evaluate current line
          currentLineIsBlank = true;
          Serial.println(inputLine);

          // SET SINGLE PIXEL url should be GET /rgb/n/rrr,ggg,bbb
          if (inputLine.length() > 3 && inputLine.substring(0, 9) == F("GET /rgb/")) {
            int slash = inputLine.indexOf('/', 9);
            ledix = inputLine.substring(9, slash).toInt();
            int urlend = inputLine.indexOf(' ', 9);
            String getParam = inputLine.substring(slash + 1, urlend + 1);
            int komma1 = getParam.indexOf(',');
            int komma2 = getParam.indexOf(',', komma1 + 1);
            redLevel = getParam.substring(0, komma1).toInt();
            greenLevel = getParam.substring(komma1 + 1, komma2).toInt();
            blueLevel = getParam.substring(komma2 + 1).toInt();
            stripe_setPixelColor(ledix, stripe_color(redLevel, greenLevel, blueLevel));
            stripe_show();
            isGet = true;
          }
          // SET DELAY url should be GET /delay/n
          if (inputLine.length() > 3 && inputLine.substring(0, 11) == F("GET /delay/")) {
            delay_interval = inputLine.substring(11).toInt();
            isGet = true;
          }
          // SET BRIGHTNESS url should be GET /brightness/n
          if (inputLine.length() > 3 && inputLine.substring(0, 16) == F("GET /brightness/")) {
            stripe_setBrightness(inputLine.substring(16).toInt());
            stripe_show();
            isGet = true;
          }
          // SET PIXEL RANGE url should be GET /range/x,y/rrr,ggg,bbb
          if (inputLine.length() > 3 && inputLine.substring(0, 11) == F("GET /range/")) {
            int slash = inputLine.indexOf('/', 11);
            int komma1 = inputLine.indexOf(',');
            int x = inputLine.substring(11, komma1).toInt();
            int y = inputLine.substring(komma1 + 1, slash).toInt();
            int urlend = inputLine.indexOf(' ', 11);
            String getParam = inputLine.substring(slash + 1, urlend + 1);
            komma1 = getParam.indexOf(',');
            int komma2 = getParam.indexOf(',', komma1 + 1);
            redLevel = getParam.substring(0, komma1).toInt();
            greenLevel = getParam.substring(komma1 + 1, komma2).toInt();
            blueLevel = getParam.substring(komma2 + 1).toInt();
            for (int i = x; i <= y; i++) {
              stripe_setPixelColor(i, stripe_color(redLevel, greenLevel, blueLevel));
            }
            stripe_show();
            isGet = true;
          }

          // SET DIM url should be GET /dim/<rrr>,<ggg>,<bbb>
          //                    or GET /dim/<rrr>,<ggg>,<bbb>/<steps>
          if (inputLine.length() > 3 && inputLine.substring(0, 9) == F("GET /dim/"))
          {
            int urlend = inputLine.indexOf(' ', 9);

            String getParam = inputLine.substring(9, urlend + 1);

            int komma1 = getParam.indexOf(',');
            int komma2 = getParam.indexOf(',', komma1 + 1);

            int slash1 = getParam.indexOf('/');

            redLevel = getParam.substring(0, komma1).toInt();
            greenLevel = getParam.substring(komma1 + 1, komma2).toInt();

            if (slash1 != -1) {
              blueLevel = getParam.substring(komma2 + 1, slash1).toInt();
              dimSteps = getParam.substring(slash1 + 1, urlend).toInt();
            }
            else {
              blueLevel = getParam.substring(komma2 + 1).toInt();
              dimSteps = 50;
            }

            dimTargetColor = stripe_color(redLevel, greenLevel, blueLevel);

            for (int i = 0; i < NUM_PIXELS; i++) {
              dimInitColor[i] = stripe_getPixelColor(i);
            }

            dimCurrentStep = 1;

            rainbow = false;
            fire = false;
            sparks = false;
            white_sparks = false;
            blinker = false;
            knightrider = false;
            dim = true;

            isGet = true;
          }
          
          // SET BLINK EFFECT url should be GET /blink/<x>,<y>/<rrr>,<ggg>,<bbb>,<on>,<off>
          if (inputLine.length() > 3 && inputLine.substring(0, 11) == F("GET /blink/")) {
            int slash = inputLine.indexOf('/', 11);
            int komma1 = inputLine.indexOf(',');
            xfrom = inputLine.substring(11, komma1).toInt();
            yto = inputLine.substring(komma1 + 1, slash).toInt();
            int urlend = inputLine.indexOf(' ', 11);
            String getParam = inputLine.substring(slash + 1, urlend + 1);
            komma1 = getParam.indexOf(',');
            int komma2 = getParam.indexOf(',', komma1 + 1);
            int komma3 = getParam.indexOf(',', komma2 + 1);
            int komma4 = getParam.indexOf(',', komma3 + 1);
            myredLevel = getParam.substring(0, komma1).toInt();
            mygreenLevel = getParam.substring(komma1 + 1, komma2).toInt();
            myblueLevel = getParam.substring(komma2 + 1, komma3).toInt();

            myOn = getParam.substring(komma3 + 1, komma4).toInt();
            myOff = getParam.substring(komma4 + 1).toInt();

            blinker = true;
            rainbow = false;
            fire = false;
            sparks = false;
            white_sparks = false;
            knightrider = false;
            dim = false;

            isGet = true;
          }
          // POST PIXEL DATA
          if (inputLine.length() > 3 && inputLine.substring(0, 10) == F("POST /leds")) {
            isPost = true;
          }
          if (inputLine.length() > 3 && inputLine.substring(0, 16) == F("Content-Length: ")) {
            postDataLength = inputLine.substring(16).toInt();
          }
          
          // SET ALL PIXELS OFF url should be GET /off
          if (inputLine.length() > 3 && inputLine.substring(0, 8) == F("GET /off")) {
            fadeIn = false;
            fadeOut = true;
            
            isGet = true;
          }

          // SET ALL PIXELS to last color url should be GET /on
          if (inputLine.length() > 3 && inputLine.substring(0, 7) == F("GET /on")) {
            fadeIn = true;
            fadeOut = false;
            
            isGet = true;
          }
          
          // RESET MCU
          if (inputLine.length() > 3 && inputLine.substring(0, 10) == F("GET /reset")) {
            do_reset = true;
            isGet = true;
          }

          // GET STATUS url should be GET /status
          if (inputLine.length() > 3 && inputLine.substring(0, 11) == F("GET /status")) {
            isGet = true;
          }

          // SET FIRE EFFECT
          if (inputLine.length() > 3 && inputLine.substring(0, 9) == F("GET /fire")) {
            rainbow = false;
            fire = true;
            sparks = false;
            white_sparks = false;
            blinker = false;
            knightrider = false;
            dim = false;
            fadeIn = true;
            fadeOut = false;
            
            stripe_setBrightness(128);
            isGet = true;
          }

          // SET RAINBOW EFFECT
          if (inputLine.length() > 3 && inputLine.substring(0, 12) == F("GET /rainbow")) {
            rainbow = true;
            fire = false;
            sparks = false;
            white_sparks = false;
            blinker = false;
            knightrider = false;
            dim = false;
            fadeIn = true;
            fadeOut = false;
            
            rainbow_drawn = false;
            rainbowColor = 0;
            
            isGet = true;
          }

          // SET WHITE_SPARKS EFFECT
          if (inputLine.length() > 3 && inputLine.substring(0, 17) == F("GET /white_sparks")) {
            rainbow = false;
            fire = false;
            sparks = false;
            white_sparks = true;
            blinker = false;
            knightrider = false;
            dim = false;
            fadeIn = true;
            fadeOut = false;
            
            isGet = true;
          }

          // SET SPARKS EFFECT
          if (inputLine.length() > 3 && inputLine.substring(0, 11) == F("GET /sparks")) {
            rainbow = false;
            fire = false;
            sparks = true;
            white_sparks = false;
            blinker = false;
            knightrider = false;
            dim = false;
            fadeIn = true;
            fadeOut = false;
            
            isGet = true;
          }

          // SET KNIGHTRIDER EFFECT
          if (inputLine.length() > 3 && inputLine.substring(0, 16) == F("GET /knightrider")) {
            rainbow = false;
            fire = false;
            sparks = false;
            white_sparks = false;
            blinker = false;
            knightrider = true;
            dim = false;
            fadeIn = true;
            fadeOut = false;
            cur_step = 0;

            isGet = true;
          }

          // STOP RUNNING EFFECTS
          if (inputLine.length() > 3 && inputLine.substring(0, 9) == F("GET /nofx")) {
            rainbow = false;
            fire = false;
            sparks = false;
            white_sparks = false;
            blinker = false;
            knightrider = false;
            dim = false;
            fadeIn = false;
            fadeOut = false;
            
            isGet = true;
          }
          
          inputLine = "";
        }
        else if (c != '\r') {
          // add character to the current line
          currentLineIsBlank = false;
          inputLine += c;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println(F("client disconnected"));
  }


  if (do_reset) { // if reset command was executed, wait 1s and then restart
    yield();
    delay(500);
    ESP.restart();
  }

  if (fadeIn) fadeInEffect();
  if (fadeOut) fadeOutEffect();
  
  if (dim) dimEffect();

  if (fire) fireEffect();
  if (rainbow) rainbowCycle();
  if (blinker) blinkerEffect();
  if (sparks) sparksEffect();
  if (white_sparks) white_sparksEffect();
  if (knightrider) knightriderEffect();
}

// Reset stripe, all LED off and no effects
void reset() {
  for (int i = 0; i < stripe_numPixels(); i++) {
    stripe_setPixelColor(i, 0);
  }
  stripe_setBrightness(255);
  stripe_show();
  fire = false;
  rainbow = false;
  blinker = false;
  sparks = false;
  white_sparks = false;
  knightrider = false;
}

// LED flicker fire effect
void fireEffect() {
  for (int x = 0; x < stripe_numPixels(); x++) {
    int flicker = random(0, 55);
    int r1 = 226 - flicker;
    int g1 = 121 - flicker;
    int b1 = 35 - flicker;
    if (g1 < 0) g1 = 0;
    if (r1 < 0) r1 = 0;
    if (b1 < 0) b1 = 0;
    stripe_setPixelColor(x, stripe_color(r1, g1, b1));
  }
  stripe_show();
  delay(random(10, 113));
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle() {
  uint16_t i;

  if (!rainbow_drawn) {
    for (i = 0; i < stripe_numPixels(); i++) {
      stripe_setPixelColor(i, Wheel(((i * 256 / stripe_numPixels()) + rainbowColor) & 255));
    }
    rainbow_drawn = true;
  }
  else
  {
    stripe_rotateRight();
  }

  stripe_show();
  delay(delay_interval);
}

void blinkerEffect() {
  for (int i = xfrom; i <= yto; i++) {
    stripe_setPixelColor(i, stripe_color(myredLevel, mygreenLevel, myblueLevel));
  }
  stripe_show();
  delay(myOn);
  yield();
  for (int i = xfrom; i <= yto; i++) {
    stripe_setPixelColor(i, stripe_color(0, 0, 0));
  }
  stripe_show();
  delay(myOff);
}

void sparksEffect() {
  uint16_t i = random(NUM_PIXELS);

  if (stripe_getPixelColor(i) == 0) {
    stripe_setPixelColor(i, random(256 * 256 * 256));
  }

  for (i = 0; i < NUM_PIXELS; i++) {
    stripe_dimPixel(i);
  }

  stripe_show();
  delay(delay_interval);
}

void white_sparksEffect() {
  uint16_t i = random(NUM_PIXELS);
  uint16_t rand = random(256);

  if (stripe_getPixelColor(i) == 0) {
    stripe_setPixelColor(i, rand * 256 * 256 + rand * 256 + rand);
  }

  for (i = 0; i < NUM_PIXELS; i++) {
    stripe_dimPixel(i);
  }

  stripe_show();
  delay(delay_interval);
}

void knightriderEffect() {
  uint16_t i;

  cur_step += 1;

  if (cur_step >= ((NUM_PIXELS) * 2)) {
    cur_step = 0;
  }


  if (cur_step < (NUM_PIXELS)) {
    stripe_setPixelColor(cur_step, stripe_color(255, 0, 0));
    for (i = 1; i <= 32; i++) {
      if ((cur_step - i > -1)) {
        stripe_dimPixel(cur_step - i);
      }
      if ((cur_step + i - 1) < NUM_PIXELS) {
        stripe_dimPixel(cur_step + i - 1);
      }

    }
  } else {
    stripe_setPixelColor((NUM_PIXELS) * 2 - cur_step - 1, stripe_color(255, 0, 0));
    for (i = 1; i <= 32; i++) {
      if (((NUM_PIXELS) * 2 - cur_step - 1 + i < NUM_PIXELS)) {
        stripe_dimPixel((NUM_PIXELS) * 2 - cur_step - 1 + i);
      }
      if (((NUM_PIXELS) * 2 - cur_step - 1 - i) > -1) {
        stripe_dimPixel((NUM_PIXELS) * 2 - cur_step - 1 - i);
      }
    }
  }

  stripe_show();
  delay(delay_interval);
}

void fadeInEffect() {
  uint8_t curr = stripe_getBrightness();
  if(curr < 255) {
    stripe_setBrightness(curr + 1);

    if(curr > 245)
    {
        stripe_setBrightness(curr + 1);
    }
    else
    {
       stripe_setBrightness(curr + 10);
    }
  }
  else
  {
    fadeIn = false;
  }
  stripe_show();
  delay(5);
}

void fadeOutEffect() {
  uint8_t curr = stripe_getBrightness();

  Serial.println(curr);
  
  if(curr > 0) {

    if(curr < 10)
    {
        stripe_setBrightness(curr - 1);
    }
    else
    {
       stripe_setBrightness(curr - 10);
    }
  }
  else
  {
    rainbow = false;
    fire = false;
    sparks = false;
    white_sparks = false;
    blinker = false;
    knightrider = false;
    dim = false;
    fadeOut = false;
    fadeIn = false;
  }
  stripe_show();
  delay(5);
}

void dimEffect() {

  uint8_t targetR = Red(dimTargetColor);
  uint8_t targetG = Green(dimTargetColor);
  uint8_t targetB = Blue(dimTargetColor);

  if (dimCurrentStep < dimSteps) { // calculate current dim step

    for (int i = 0; i < NUM_PIXELS; i++) {

      uint8_t startR = Red(dimInitColor[i]);
      uint8_t startG = Green(dimInitColor[i]);
      uint8_t startB = Blue(dimInitColor[i]);

      uint8_t newR = startR + (dimCurrentStep * (float)((float)(targetR - startR) / (float)dimSteps));
      uint8_t newG = startG + (dimCurrentStep * (float)((float)(targetG - startG) / (float)dimSteps));
      uint8_t newB = startB + (dimCurrentStep * (float)((float)(targetB - startB) / (float)dimSteps));

      stripe_setPixelColor(i, stripe_color(newR, newG, newB));
    }

    dimCurrentStep++;
  }
  else { // set final color
    for (int i = 0; i < NUM_PIXELS; i++) {
      stripe_setPixelColor(i, stripe_color(targetR, targetG, targetB));
    }

    dim = false;
  }

  stripe_show();
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return stripe_color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return stripe_color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return stripe_color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

int colorVal(char c) {
  int i = (c >= '0' && c <= '9') ? (c - '0') : (c - 'A' + 10);
  return i * i + i * 2;
}

void sendOkResponse(WiFiClient client) {
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));  // the connection will be closed after completion of the response
  client.println();
  // standard response
  client.print(F("OK,"));
  client.print(stripe_numPixels());
  client.print(F(","));
  int oncount = 0;
  for (int i = 0; i < stripe_numPixels(); i++) {
    if (stripe_getPixelColor(i) != 0) oncount++;
  }
  client.println(oncount);
}
