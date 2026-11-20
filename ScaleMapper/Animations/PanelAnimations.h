#pragma once

#include <FastLEDHub.h>

#include "XYMap.h"

extern CRGB leds[];

class PanelSolid : public Animation
{
public:
  using Animation::Animation;

  void reset() {}

  void loop()
  {
    fill_solid(leds, NUM_LEDS, primaryColor());
    FastLEDHub.delay(100);
    FastLEDHub.show();
  }
};

class PanelGlow : public Animation
{
public:
  using Animation::Animation;

  uint8_t step = 0;

  void reset() { step = 0; }

  void loop()
  {
    Panel p;
    CRGB c1 = primaryColor();
    CRGB c2 = secondaryColor();
    uint8_t breath = beatsin8(30, 40, 255);
    uint16_t maxDim = (p.C > p.R) ? p.C : p.R;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (uint16_t y = 0; y < p.R; y++)
    {
      for (uint16_t x = 0; x < p.C; x++)
      {
        int16_t i = p.index(x, y);
        if (i < 0) continue;
        uint16_t fall = 255 - ((uint32_t)p.distFromCenter(x, y) * 480 / maxDim);
        CRGB c = blend(c1, c2, 255 - (uint8_t)max(5, (int)fall));
        c.nscale8_video(breath);
        c.nscale8_video((uint8_t)max(5, (int)fall));
        leds[i] = c;
      }
    }

    FastLEDHub.delay(16);
    FastLEDHub.show();
  }
};

class PanelRipple : public Animation
{
public:
  using Animation::Animation;

  void reset() {}

  void loop()
  {
    Panel p;
    CRGB c1 = primaryColor();
    CRGB c2 = secondaryColor();
    uint16_t maxDim = (p.C > p.R) ? p.C : p.R;
    uint16_t spacing = 512 / (maxDim ? maxDim : 1);
    uint8_t t = millis() / 4;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (uint16_t y = 0; y < p.R; y++)
    {
      for (uint16_t x = 0; x < p.C; x++)
      {
        int16_t i = p.index(x, y);
        if (i < 0) continue;
        uint8_t v = sin8(p.distFromCenter(x, y) * spacing - t);
        leds[i] = blend(c2, c1, v);
      }
    }

    FastLEDHub.delay(16);
    FastLEDHub.show();
  }
};

class WaveX : public Animation
{
public:
  using Animation::Animation;

  void reset() {}

  void loop()
  {
    Panel p;
    CRGB c1 = primaryColor();
    CRGB c2 = secondaryColor();
    uint16_t spacing = 512 / (p.C ? p.C : 1);
    uint8_t t = millis() / 4;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (uint16_t y = 0; y < p.R; y++)
    {
      for (uint16_t x = 0; x < p.C; x++)
      {
        int16_t i = p.index(x, y);
        if (i < 0) continue;
        uint8_t v = sin8(x * spacing - t);
        leds[i] = blend(c2, c1, v);
      }
    }

    FastLEDHub.delay(16);
    FastLEDHub.show();
  }
};

class WaveY : public Animation
{
public:
  using Animation::Animation;

  void reset() {}

  void loop()
  {
    Panel p;
    CRGB c1 = primaryColor();
    CRGB c2 = secondaryColor();
    uint16_t maxDim = (p.C > p.R) ? p.C : p.R;
    uint16_t spacing = 512 / (maxDim ? maxDim : 1);
    uint8_t t = millis() / 4;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (uint16_t y = 0; y < p.R; y++)
    {
      for (uint16_t x = 0; x < p.C; x++)
      {
        int16_t i = p.index(x, y);
        if (i < 0) continue;
        uint8_t v = sin8(y * spacing - t);
        leds[i] = blend(c2, c1, v);
      }
    }

    FastLEDHub.delay(16);
    FastLEDHub.show();
  }
};

class PanelFire : public Animation
{
public:
  using Animation::Animation;

  void reset() {}

  void loop()
  {
    Panel p;
    uint8_t t = millis() / 8;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (uint16_t y = 0; y < p.R; y++)
    {
      for (uint16_t x = 0; x < p.C; x++)
      {
        int16_t i = p.index(x, y);
        if (i < 0) continue;
        uint8_t n = inoise8(x * 90, y * 90 - t * 6, t);
        leds[i] = ColorFromPalette(HeatColors_p, qadd8(n, 40));
      }
    }

    FastLEDHub.delay(16);
    FastLEDHub.show();
  }
};

class PanelRainbow : public Animation
{
public:
  using Animation::Animation;

  void reset() {}

  void loop()
  {
    Panel p;
    uint8_t t = millis() / 8;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (uint16_t y = 0; y < p.R; y++)
    {
      for (uint16_t x = 0; x < p.C; x++)
      {
        int16_t i = p.index(x, y);
        if (i < 0) continue;
        leds[i] = CHSV(x * 255 / (p.C ? p.C : 1) + y * 20 + t, 255, 255);
      }
    }

    FastLEDHub.delay(16);
    FastLEDHub.show();
  }
};

// Scans every grid cell white in reading order and marks the unlit regions:
// strip start/end pads blue, U-turn gap pixels dim red. Use it to verify the
// physical wiring against the slider geometry.
class TestPattern : public Animation
{
public:
  using Animation::Animation;

  void reset() {}

  void loop()
  {
    Panel p;
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    for (uint16_t i = 0; i < p.S && i < NUM_LEDS; i++)
      leds[i] = CRGB(0, 0, 60);
    for (uint16_t i = 0; i < p.E; i++)
    {
      uint32_t tail = NUM_LEDS - 1 - i;
      if (tail < NUM_LEDS) leds[tail] = CRGB(0, 0, 60);
    }
    for (uint16_t y = 0; y < p.R; y++)
    {
      for (uint16_t k = 0; k < p.G; k++)
      {
        uint32_t g = p.S + (uint32_t)y * (p.C + p.G) + p.C + k;
        if (g < NUM_LEDS) leds[g] = CRGB(40, 0, 0);
      }
    }

    uint16_t cells = 0;
    for (uint16_t y = 0; y < p.R; y++)
      for (uint16_t x = 0; x < p.C; x++)
      {
        int16_t i = p.index(x, y);
        if (i >= 0) cells++;
      }

    for (uint16_t y = 0; y < p.R; y++)
    {
      for (uint16_t x = 0; x < p.C; x++)
      {
        int16_t i = p.index(x, y);
        if (i < 0) continue;
        uint16_t n = y * p.C + x;
        leds[i] = (n == (millis() / 120) % cells) ? CRGB::White : CRGB(2, 2, 2);
      }
    }

    FastLEDHub.delay(16);
    FastLEDHub.show();
  }
};
