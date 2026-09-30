/// @file    serial-json.ino
/// @brief   Minimal Serial JSON protocol for ESP32
///          Receives pixel data and commands from p5.js via Serial
/// @example serial-json.ino
///
/// This firmware listens for JSON commands on the Serial port
/// and controls an attached LED strip accordingly.
///
/// Use with the Serial-to-WebSocket bridge for p5.js communication:
///   p5js-simple/server/serial-bridge/
///
/// Dependencies:
///   - FastLED
///   - ArduinoJson

#include <FastLED.h>
#include <ArduinoJson.h>

// ============================================
// LED Configuration
// ============================================

#define DATA_PIN    5           // GPIO pin for LED data
#define LED_TYPE    WS2812B    // WS2811, WS2812, WS2812B
#define COLOR_ORDER GRB         // GRB, RGB, BRG, etc.
#define NUM_LEDS    240        // Number of LEDs in strip

CRGB leds[NUM_LEDS];

#define BRIGHTNESS  96         // Default brightness (0-255)
#define FRAME_RATE   60         // Target frame rate

// ============================================
// Serial Configuration
// ============================================

#define SERIAL_BAUD   115200   // Serial baud rate
#define MESSAGE_DELIMITER '\n' // JSON message delimiter

// ============================================
// State Variables
// ============================================

String currentMode = "off";
CRGB currentColor = CRGB::Black;
int currentBrightness = BRIGHTNESS;

// Fire simulation variables
float cooling = 55;
float sparking = 120;
int heat[NUM_LEDS];

// ============================================
// Setup
// ============================================

void setup() {
  // Start serial
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // Wait for serial to initialize
  
  Serial.println("\n=== ESP32 Serial JSON Client ===");
  Serial.println("Compiled: " __DATE__ " " __TIME__);
  Serial.println("Protocol: serial-json v1.0");
  Serial.println("Baud: " + String(SERIAL_BAUD));
  Serial.println("LEDs: " + String(NUM_LEDS));
  
  // Initialize LED strip
  Serial.println("\nInitializing LED strip...");
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  Serial.println("LED strip initialized");
  
  // Initialize heat array for fire effect
  memset(heat, 0, sizeof(heat));
  
  // Send ready message
  sendMessage({
    {"type", "ready"},
    {"firmware", "serial-json"},
    {"version", "1.0"},
    {"leds", NUM_LEDS},
    {"protocol", "serial-json"}
  });
  
  Serial.println("\nWaiting for commands...");
}

// ============================================
// Main Loop
// ============================================

void loop() {
  // Check for incoming serial data
  if (Serial.available()) {
    String message = Serial.readStringUntil(MESSAGE_DELIMITER);
    message.trim();
    
    if (message.length() > 0) {
      // Parse and handle message
      handleMessage(message);
    }
  }
  
  // Run LED effects based on mode
  switch(currentMode.c_str()[0]) {
    case 'f':  // fire
      fireEffect();
      break;
    case 'r':  // rainbow
      rainbowEffect();
      break;
    case 's':  // solid
      solidEffect();
      break;
    case 'o':  // off
      offEffect();
      break;
    default:
      // Unknown mode, show current color
      fill_solid(leds, NUM_LEDS, currentColor);
      FastLED.show();
      break;
  }
  
  // Small delay to maintain frame rate
  delay(1000 / FRAME_RATE);
}

// ============================================
// Message Handling
// ============================================

void handleMessage(String message) {
  // Debug: echo received message
  // Serial.print("RX: ");
  // Serial.println(message);
  
  // Parse JSON
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    
    // Send error response
    sendError("Invalid JSON: " + String(error.c_str()));
    return;
  }
  
  // Check for type field
  if (!doc.containsKey("type")) {
    sendError("Message has no 'type' field");
    return;
  }
  
  String type = doc["type"];
  
  // Handle message based on type
  if (type == "ping") {
    sendMessage({{"type", "pong"}});
  }
  else if (type == "identify") {
    // Respond to identification request
    sendMessage({
      {"type", "identify"},
      {"client", "esp32"},
      {"firmware", "serial-json"},
      {"version", "1.0"},
      {"leds", NUM_LEDS},
      {"mode", currentMode}
    });
  }
  else if (type == "status") {
    sendStatus();
  }
  else if (type == "mode") {
    if (doc.containsKey("mode")) {
      currentMode = doc["mode"];
      Serial.print("Mode set to: ");
      Serial.println(currentMode);
      sendAck("mode", currentMode);
    } else {
      sendError("mode: missing 'mode' field");
    }
  }
  else if (type == "color") {
    if (doc.containsKey("color") && doc["color"].is<JsonArray>()) {
      JsonArray colorArray = doc["color"];
      if (colorArray.size() >= 3) {
        currentColor = CRGB(
          colorArray[0] | 0,
          colorArray[1] | 0,
          colorArray[2] | 0
        );
        Serial.print("Color set to: ");
        Serial.print(colorArray[0]);
        Serial.print(",");
        Serial.print(colorArray[1]);
        Serial.print(",");
        Serial.println(colorArray[2]);
        sendAck("color", currentColor);
      } else {
        sendError("color: array must have at least 3 elements");
      }
    } else {
      sendError("color: missing or invalid 'color' field");
    }
  }
  else if (type == "brightness") {
    if (doc.containsKey("value")) {
      currentBrightness = doc["value"] | 0;
      FastLED.setBrightness(currentBrightness);
      Serial.print("Brightness set to: ");
      Serial.println(currentBrightness);
      sendAck("brightness", currentBrightness);
    } else {
      sendError("brightness: missing 'value' field");
    }
  }
  else if (type == "pixels") {
    if (doc.containsKey("data") && doc.containsKey("count")) {
      JsonArray data = doc["data"];
      int count = doc["count"] | 0;
      int offset = doc.containsKey("offset") ? doc["offset"] | 0 : 0;
      
      int pixelsToUpdate = min(count, NUM_LEDS - offset);
      for (int i = 0; i < pixelsToUpdate; i++) {
        if (data[i * 3].is<int>() && data[i * 3 + 1].is<int>() && data[i * 3 + 2].is<int>()) {
          leds[offset + i] = CRGB(
            data[i * 3] | 0,
            data[i * 3 + 1] | 0,
            data[i * 3 + 2] | 0
          );
        }
      }
      FastLED.show();
      Serial.print("Updated ");
      Serial.print(pixelsToUpdate);
      Serial.print(" pixels starting at ");
      Serial.println(offset);
      sendAck("pixels", pixelsToUpdate);
    } else {
      sendError("pixels: missing 'data' or 'count' field");
    }
  }
  else if (type == "pixel") {
    if (doc.containsKey("index") && doc.containsKey("color")) {
      int index = doc["index"] | 0;
      JsonArray colorArray = doc["color"];
      
      if (index >= 0 && index < NUM_LEDS && colorArray.size() >= 3) {
        leds[index] = CRGB(
          colorArray[0] | 0,
          colorArray[1] | 0,
          colorArray[2] | 0
        );
        FastLED.show();
        Serial.print("Set pixel ");
        Serial.print(index);
        Serial.println(" to new color");
        sendAck("pixel", index);
      } else {
        sendError("pixel: invalid index or color");
      }
    } else {
      sendError("pixel: missing 'index' or 'color' field");
    }
  }
  else if (type == "fireParams") {
    if (doc.containsKey("cooling")) {
      cooling = doc["cooling"];
    }
    if (doc.containsKey("sparking")) {
      sparking = doc["sparking"];
    }
    Serial.print("Fire params: cooling=");
    Serial.print(cooling);
    Serial.print(", sparking=");
    Serial.println(sparking);
    sendAck("fireParams", {"cooling", cooling, "sparking", sparking});
  }
  else if (type == "rainbowParams") {
    // Handle rainbow parameters if needed
    sendAck("rainbowParams");
  }
  else {
    // Unknown message type
    Serial.print("Unknown message type: ");
    Serial.println(type);
    sendError("Unknown message type: " + type);
  }
}

// ============================================
// Send Functions
// ============================================

// Send a message (JSON object as key-value pairs)
void sendMessage(const std::vector<std::pair<const char*, JsonVariant>>& pairs) {
  StaticJsonDocument<256> doc;
  for (const auto& pair : pairs) {
    doc[pair.first] = pair.second;
  }
  
  String json;
  serializeJson(doc, json);
  Serial.println(json);
}

// Send acknowledgment
void sendAck(String command, String value) {
  sendMessage({
    {"type", "ack"},
    {"command", command},
    {"value", value}
  });
}

void sendAck(String command, int value) {
  sendAck(command, String(value));
}

void sendAck(String command, const char* value) {
  sendAck(command, String(value));
}

// Send error message
void sendError(String message) {
  sendMessage({
    {"type", "error"},
    {"message", message},
    {"timestamp", millis()}
  });
}

// Send status
void sendStatus() {
  sendMessage({
    {"type", "status"},
    {"leds", NUM_LEDS},
    {"mode", currentMode},
    {"brightness", currentBrightness},
    {"freeMemory", ESP.getFreeHeap()},
    {"uptime", millis() / 1000}
  });
}

// ============================================
// LED Effects
// ============================================

// Fire effect
void fireEffect() {
  // Step 1: Cool down every cell
  for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }
  
  // Step 2: Heat drifts up and diffuses
  for (int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  
  // Step 3: Randomly ignite new sparks
  if (random8() < sparking) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }
  
  // Step 4: Map heat to LED colors
  for (int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor(heat[j]);
    leds[j] = color;
  }
  
  FastLED.show();
}

// Rainbow effect
void rainbowEffect() {
  static uint8_t hue = 0;
  fill_rainbow(leds, NUM_LEDS, hue++);
  FastLED.show();
}

// Solid color effect
void solidEffect() {
  fill_solid(leds, NUM_LEDS, currentColor);
  FastLED.show();
}

// Off effect
void offEffect() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}
