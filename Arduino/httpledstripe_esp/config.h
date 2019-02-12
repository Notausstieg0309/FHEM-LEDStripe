// the TCP port, were the HTTP server should accept requests
#define SERVER_PORT  80

// WiFi settings
#define WIFI_SSID  "<Insert your WLAN SSID here>"
#define WIFI_KEY   "<Insert your WLAN passphrase here>"

// the number of WS2812B Pixels attached 
#define NUM_PIXELS     120

// the Pin to use as status LED's (D0 = NodeMCU LED / D4 = ESP-12 LED)
#define STATUS_LED_PIN  D4 

// disable WLAN sleep modes
#define USE_WLAN_SLEEP 0

// enable OTA flash via Arduino IDE (0/1)
#define OTA_FLASH_ENABLE    1

// set a custom esp hostname in Arduino IDE, if commented out, the board id will be used (esp8266-XXXXX)
#define OTA_FLASH_HOSTNAME  "esp-wohnzimmer"  

// Select the used NeoPixel library, only one can be active at the same time
#define LIB_NEOPIXELBUS
//#define LIB_ADAFRUIT
//#define LIB_ADAFRUIT_PIN   14
