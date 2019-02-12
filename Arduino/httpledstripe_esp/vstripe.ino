
uint32_t current_colors[NUM_PIXELS];
uint8_t  current_brightness = 255;

#ifdef LIB_NEOPIXELBUS

#include <NeoPixelBrightnessBus.h>

NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUM_PIXELS);

// Initialize all pixels to 'off'
void stripe_setup() {
  strip.Begin();
 // strip2.Begin();
  stripe_show(); 
}

void stripe_show() {
  strip.Show(); 
 // strip2.Show(); 
}

void stripe_setDirectPixelColor(uint16_t pixel, uint32_t color) {
    strip.SetPixelColor(pixel, RgbColor(Red(color), Green(color), Blue(color)));
}

void stripe_setPixelColor(uint16_t pixel, uint32_t color) {

    current_colors[pixel] = color;
    stripe_applyPixelBrightness(pixel);
}


void stripe_rotateRight() {
  strip.RotateRight(1);
}

#endif


#ifdef LIB_ADAFRUIT

#include <Adafruit_NeoPixel.h>

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, LIB_ADAFRUIT_PIN, NEO_GRB + NEO_KHZ800);


// Initialize all pixels to 'off'
void stripe_setup() {
  strip.begin();
  stripe_show(); 
}

void stripe_show() {
  strip.show(); 
}

void stripe_setDirectPixelColor(uint16_t pixel, uint32_t color)
{
  strip.setPixelColor(NUM_PIXELS, color);
}

uint32_t stripe_getDirectPixelColor(uint16_t pixel)
{
  return strip.getPixelColor(pixel);
}

#endif





//////////////////////////////////////////////
// generic color helpers

void stripe_applyPixelBrightness(uint16_t pixel)
{

  if(current_brightness == 255)
  {
    stripe_setDirectPixelColor(pixel, current_colors[pixel]);
  }
  else if(current_brightness == 0)
  {
    stripe_setDirectPixelColor(pixel, 0);
  }
  else if(current_brightness < 255 && current_brightness > 0)
  {
    uint8_t r = Red(current_colors[pixel]);
    uint8_t g = Green(current_colors[pixel]);
    uint8_t b = Blue(current_colors[pixel]);
    
    float factor = ((float)current_brightness / 255.0f);
    r = r * (float)factor; 
    g = g * (float)factor; 
    b = b * (float)factor; 
    
    stripe_setDirectPixelColor(pixel, stripe_color(r, g, b));
  }
}

uint32_t stripe_getPixelColor(uint16_t pixel) {
  return current_colors[pixel];
}

void stripe_dimPixel(uint16_t pixel)
{
   stripe_setPixelColor(pixel, DimColor(stripe_getPixelColor(pixel)));
}

void stripe_setBrightness(uint8_t b) {
  current_brightness = b;
  for (int i = 0; i < NUM_PIXELS; i++) {
    stripe_applyPixelBrightness(i);
  }
}

uint8_t stripe_getBrightness() {
  return current_brightness;
}

// returns the number of configured pixels
uint16_t stripe_numPixels()
{
  return NUM_PIXELS;
}

// returns 32-bit color value from separate red, green and blue values
uint32_t stripe_color(uint8_t r, uint8_t g, uint8_t b)
{
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

uint8_t Red(uint32_t color)
{
  return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color)
{
  return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color)
{
  return color & 0xFF;
}

uint32_t DimColor(uint32_t color)
{
  uint32_t dimColor = stripe_color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
  return dimColor;
}
