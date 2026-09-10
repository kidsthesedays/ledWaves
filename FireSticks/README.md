# Fire Sticks

ILLUTRON Kunst Pa Kajan - LED Control System

## Overview

Fire Sticks is a simplified, focused evolution of the Serpent Fragment project. This version concentrates on creating beautiful fire-like effects on one-meter lengths of low-density NeoPixel LED strips (WS2811/WS2812) using ESP32 microcontrollers.

## Hardware Stack

- **Microcontroller**: ESP32 (primary platform)
- **LED Strips**: 1-meter WS2811/WS2812 NeoPixel strips (low density)
- **Power**: Daisy-chained power distribution
- **Signal**: Daisy-chained data signal
- **Cabling**: Custom cabling solutions for clean power and signal distribution

## Software Stack

### Embedded (ESP32)
- **FastLED Library**: For LED strip control
- **WiFi/Bluetooth**: For wireless communication
- **WebSocket/REST**: For receiving commands from web interface

### Web Interface
- **JavaScript/p5.js**: For pixel mapping and control
- **p5.gui**: For user interface controls
- **WebSocket**: Real-time communication with ESP32

## Project Structure

```
FireSticks/
├── hardware/           # Hardware documentation, schematics
├── software/
│   ├── esp32/         # ESP32 firmware
│   │   └── FireSticks.ino
│   └── web/           # Web interface
│       ├── js/        # JavaScript files
│       ├── css/       # CSS files
│       └── lib/       # Libraries
└── docs/             # Additional documentation
```

## Features

- Real-time LED color and pattern control via web interface
- Fire simulation effects
- Video/plane mapping capabilities
- Daisy-chain support for multiple sticks
- Clean cabling solutions

## Getting Started

See [docs/](docs/) for detailed setup instructions.
