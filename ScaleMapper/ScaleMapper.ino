#include <FastLEDHub.h>

#include "Animations/XYMap.h"
#include "Animations/PanelAnimations.h"

// Physical strip (compile-time)
#define NUM_LEDS 300
#define LED_TYPE WS2811
#define LIGHTSTRIP_PIN 5

CRGB leds[NUM_LEDS];

void setup()
{
  FastLEDHub.initialize("Scale Mapper");
  FastLEDHub.addLeds<LED_TYPE, LIGHTSTRIP_PIN, GRB>(leds, NUM_LEDS);

  // Zigzag panel geometry — adjustable live in the web interface and
  // persisted automatically by FastLEDHub
  FastLEDHub.registerSlider(new Slider("Columns", 1, 64, 16, 1, "grid-3x3"));
  FastLEDHub.registerSlider(new Slider("Rows", 1, 64, 16, 1, "grid-3x3"));
  FastLEDHub.registerSlider(new Slider("U-Turn gap", 0, 40, 1, 1, "arrow-return-left"));
  FastLEDHub.registerSlider(new Slider("Start pad", 0, 200, 0, 1, "caret-left"));
  FastLEDHub.registerSlider(new Slider("End pad", 0, 200, 0, 1, "caret-right"));

  FastLEDHub.registerColorPicker(new ColorPicker("Primary Color", CRGB(255, 96, 0)));
  FastLEDHub.registerColorPicker(new ColorPicker("Secondary Color", CRGB(0, 60, 255)));

  FastLEDHub.registerAnimation(new PanelSolid("Solid"));
  FastLEDHub.registerAnimation(new PanelGlow("Glow"));
  FastLEDHub.registerAnimation(new PanelRipple("Ripple"));
  FastLEDHub.registerAnimation(new WaveX("Wave X"));
  FastLEDHub.registerAnimation(new WaveY("Wave Y"));
  FastLEDHub.registerAnimation(new PanelFire("Fire"));
  FastLEDHub.registerAnimation(new PanelRainbow("Rainbow"));
  FastLEDHub.registerAnimation(new TestPattern("Test Pattern"));
}

void loop()
{
  FastLEDHub.handle();
}
