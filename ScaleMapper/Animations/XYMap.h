#pragma once

#include <FastLEDHub.h>

#ifndef NUM_LEDS
#define NUM_LEDS 300
#endif

// Zigzag panel geometry: one physical strip driven as an X/Y grid.
// (0,0) is the top-left corner, X grows right, Y grows down.
// Row y starts at the start pad plus the accumulated length (and one U-turn
// gap) of every earlier row. Even rows run left->right, odd rows
// right->left (serpentine). The start/end pads and the U-turn gaps are
// physically part of the strip but stay unlit.

struct Panel
{
  uint16_t C, R, G, S, E;

  Panel()
  {
    C = FastLEDHub.getSlider("Columns")->value;
    R = FastLEDHub.getSlider("Rows")->value;
    G = FastLEDHub.getSlider("U-Turn gap")->value;
    S = FastLEDHub.getSlider("Start pad")->value;
    E = FastLEDHub.getSlider("End pad")->value;
  }

  uint16_t maxLen() const { return C; }

  uint8_t distFromCenter(uint16_t x, uint16_t y) const
  {
    uint16_t cx = C / 2, cy = R / 2;
    uint16_t dx = (x > cx) ? x - cx : cx - x;
    uint16_t dy = (y > cy) ? y - cy : cy - y;
    return sqrt16(dx * dx + dy * dy);
  }

  int16_t index(uint16_t x, uint16_t y) const
  {
    if (x >= C || y >= R) return -1;
    uint32_t idx = S + (uint32_t)y * (C + G) + ((y & 1) ? (C - 1 - x) : x);
    if (idx >= NUM_LEDS || idx >= (uint32_t)(NUM_LEDS - E)) return -1;
    return (int16_t)idx;
  }
};

inline CRGB primaryColor() { return FastLEDHub.getColorPicker("Primary Color")->value; }
inline CRGB secondaryColor() { return FastLEDHub.getColorPicker("Secondary Color")->value; }
