#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO

#include <FastLED.h> // only included for some math functions. Must be included before (Pololu's) APA102.h or else there will be namespace conflicts

#include <APA102.h>

// Define which pins to use.
uint8_t const dataPin = 11;
uint8_t const clockPin = 12;

Pololu::APA102<dataPin, clockPin> ledStrip;

uint8_t const rows = 21;
uint8_t const cols = 9;
rgb_color fullMatrix[cols][rows];

uint16_t const ledCount = 149;
rgb_color colors[ledCount];

uint8_t const colHeight[cols] = { 9, 16, 18, 20, 21, 20, 18, 16, 11 }; // number of LEDs in each column
uint8_t const colOffset[cols] = { 8,  2,  0,  0,  0,  0,  0,  1,  6 }; // vertical offset of each column (= # of LEDs missing from bottom of each column)

uint8_t const brightness = 4; // 0 to 31

void setup()
{
}

/* Converts a color from HSV to RGB.
 * h is hue, as a number between 0 and 360.
 * s is the saturation, as a number between 0 and 255.
 * v is the value, as a number between 0 and 255. */
rgb_color hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch((h / 60) % 6){
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return (rgb_color){r, g, b};
}

void setColorsFromFullMatrix()
{  
  uint8_t x = 0;
  int8_t y = colOffset[0]; // must be signed for comparison < 0
  
  for (uint8_t i = 0; i < ledCount; i++)
  {
    colors[i] = fullMatrix[x][y];
    if ((x % 2) == 0)
    {
      // even column - LEDs go up
      y++;
      if (y >= (colOffset[x] + colHeight[x]))
      {
        // next column
        x++;
        y = colOffset[x] + colHeight[x] - 1;
      }
    }
    else
    {
      // odd column - LEDs go down
      y--;
      if (y < colOffset[x])
      {
        // next column
        x++;
        y = colOffset[x];
      }
    }
  }
}

void redGoldWave()
{
  uint8_t time = millis() >> 6;

  for(uint8_t y = 0; y < rows; y++)
  {
    for (uint8_t x = 0; x < cols; x++)
    {
      fullMatrix[x][y] = hsvToRgb(sin8(time - x*16 - y*8) / 6, 255, 255);
    }
  }
}

void rainbowNoise()
{
  static uint32_t xOffset = random16();
  static uint32_t yOffset = random16();
  static uint32_t zOffset = random16();
  
  uint32_t time = millis() >> 4;

  for(uint32_t y = 0; y < rows; y++)
  {
    for (uint32_t x = 0; x < cols; x++)
    {
      uint8_t data = inoise8(x * 50 + xOffset, y * 29 + yOffset, time + zOffset);

      // "The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data."
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));

      // this seems to work better than trying to map the data value to a hue myself with hsvToRgb()...
      CRGB crgbColor = ColorFromPalette(RainbowColors_p, data, 255);

      // instead of just assigning the new color, do some smoothing to get rid of flickering (artifacts),
      // kind of like the NoisePlusPalette example from FastLED does
      fullMatrix[x][y].red = ((uint16_t)2*fullMatrix[x][y].red + crgbColor.red) / 3;
      fullMatrix[x][y].green = ((uint16_t)2*fullMatrix[x][y].green + crgbColor.green) / 3;
      fullMatrix[x][y].blue = ((uint16_t)2*fullMatrix[x][y].blue + crgbColor.blue) / 3;
    }
  }
}

void loop()
{
  static uint8_t lastMs = 0;
  uint8_t ms = millis();
  
  if ((uint8_t)(ms - lastMs) > 10)
  {
    redGoldWave();
  
    setColorsFromFullMatrix();
    ledStrip.write(colors, ledCount, brightness);

    lastMs = ms;
  }
}
