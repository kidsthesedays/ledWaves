# Fire Sticks - Hardware Documentation

## Overview

The Fire Sticks hardware system is designed for creating beautiful fire-like LED effects using one-meter lengths of low-density NeoPixel LED strips. This documentation covers the hardware components, wiring, and cabling solutions for the Fire Sticks project.

## Components

### Core Components

1. **ESP32 Microcontroller**
   - Primary controller for each Fire Stick
   - Handles LED strip control and communication
   - Runs FastLED library for LED effects
   - Provides WiFi and Bluetooth connectivity

2. **NeoPixel LED Strips**
   - WS2811 or WS2812 LED strips
   - Low density (typically 30-60 LEDs per meter)
   - 1-meter lengths for each Fire Stick
   - 5V operation

3. **Power Supply**
   - 5V power supply with sufficient current for all connected strips
   - Recommended: 5V, 10A power supply for up to 10 meters of LED strips
   - Power distribution board for daisy-chaining

### Optional Components

- **Potentiometer**: For analog input control (brightness, speed, etc.)
- **Buttons/Switches**: For mode selection and control
- **Enclosure**: For protecting electronics and providing mounting points
- **Heat Sinks**: For power supply and ESP32 (if needed)

## Wiring Diagram

### Single Stick Configuration

```
ESP32 Pinout:
├── GPIO 5 ─────────────► LED Strip Data In
├── 5V ─────────────────► LED Strip 5V
├── GND ────────────────► LED Strip GND
├── A0 (Optional) ───────► Potentiometer
└── GND ────────────────► Potentiometer GND
```

### Daisy-Chain Configuration

For multiple sticks, connect them in series:

```
ESP32 ─────► Stick 1 ─────► Stick 2 ─────► Stick 3
   │            │            │            │
   ├── 5V ─────┼────────────┼────────────┤
   ├── GND ─────┼────────────┼────────────┤
   └── Data ───┴────────────┴────────────┘
```

**Important Notes:**
- Data line must be connected in series (daisy-chained)
- Power (5V and GND) should be connected in parallel to all strips
- For longer runs, consider adding power injection at multiple points

## Power Distribution

### Power Requirements

Each WS2812 LED can draw up to 60mA at full brightness (white). For a 1-meter strip with 60 LEDs:
- Maximum current: 60 LEDs × 60mA = 3.6A per meter
- Recommended: 4A per meter for safety margin

### Daisy-Chain Power

For multiple sticks in a daisy-chain:

1. **Short runs (1-3 meters)**: Can be powered from ESP32 5V pin
2. **Medium runs (3-6 meters)**: Use external 5V power supply
3. **Long runs (6+ meters)**: Use power injection at multiple points

### Power Injection

For long LED strip runs, inject power at both ends and every 2-3 meters:

```
Power Supply 5V ────┬─────────────► Stick 1 Start
                   ├─────────────► Stick 1 End
                   ├─────────────► Stick 2 Start
                   └─────────────► Stick 2 End

Power Supply GND ──┬─────────────► Stick 1 Start
                   ├─────────────► Stick 1 End
                   ├─────────────► Stick 2 Start
                   └─────────────► Stick 2 End
```

## Cabling Solutions

### Recommended Cabling

1. **LED Strip to ESP32**
   - Use 22-24 AWG wire for data
   - Use 18-20 AWG wire for power
   - Keep data wire as short as possible (< 30cm)

2. **Between Strips (Daisy-Chain)**
   - Use 3-conductor cable (Data, 5V, GND)
   - Recommended: 22 AWG for data, 18 AWG for power
   - Use connectors for easy assembly/disassembly

3. **Power Distribution**
   - Use 16-18 AWG wire for main power distribution
   - Use terminal blocks for power connections

### Connector Options

1. **JST SM Connectors** (2.5mm pitch)
   - Standard for WS2812 strips
   - 3-pin: Data, 5V, GND
   - Easy to connect/disconnect

2. **Screw Terminals**
   - For permanent installations
   - Secure connections
   - Easy to repair

3. **XH2.54 Connectors**
   - Common for LED strips
   - 3-pin configuration
   - Polarized to prevent reverse connection

### Cable Length Recommendations

- **Data Signal**: Keep under 1 meter for reliable operation
- **Power**: Can be longer with appropriate wire gauge
- **Total Stick Length**: Up to 10 meters with proper power distribution

## Signal Integrity

### Data Signal Considerations

1. **Maximum Length**: 
   - Single run: ~5 meters maximum
   - With signal boosters: Can be extended

2. **Signal Boosters**
   - Use WS2812 buffer ICs for long runs
   - Or use separate ESP32 for each 5-meter segment

3. **Data Rate**
   - WS2812: 800kHz (default)
   - WS2812B: 800kHz or 400kHz
   - Lower data rates may improve reliability for long runs

### Troubleshooting Signal Issues

1. **Flickering LEDs**: Check power supply, add capacitors
2. **Incorrect Colors**: Check data line connections
3. **LEDs Not Responding**: Check data line continuity, verify connections
4. **Random Behavior**: Check for loose connections, electrical noise

## Mounting Options

### Stick Mounting

1. **Aluminum Channels**
   - Provides heat dissipation
   - Diffuses light for smoother appearance
   - Available in various sizes

2. **3D Printed Mounts**
   - Custom designs for specific installations
   - Can include cable management
   - Lightweight and flexible

3. **Adhesive Backing**
   - Direct stick-on mounting
   - Quick and easy installation
   - Less secure for permanent installations

### Enclosure Options

1. **Plastic Enclosures**
   - For protecting ESP32 and connections
   - Various sizes available
   - Can be waterproof

2. **Metal Enclosures**
   - For EMI shielding
   - Better heat dissipation
   - More durable

3. **Custom Enclosures**
   - 3D printed enclosures
   - Designed specifically for Fire Sticks
   - Can include mounting features

## Safety Considerations

### Electrical Safety

1. **Power Supply**
   - Use appropriate power supply for your configuration
   - Ensure proper voltage (5V)
   - Check current rating (should exceed maximum expected draw)

2. **Wiring**
   - Use appropriate wire gauge for current
   - Insulate all connections
   - Avoid sharp edges that could damage wires

3. **Heat**
   - LED strips can get warm at high brightness
   - Provide adequate ventilation
   - Consider heat sinks for high-power installations

### Fire Safety

1. **Keep away from flammable materials**
2. **Don't cover LED strips with insulating materials**
3. **Monitor temperature during operation**
4. **Use appropriate fusing for power circuits**

## Assembly Instructions

### Single Stick Assembly

1. **Prepare LED Strip**
   - Cut strip to desired length (1 meter)
   - Solder wires to data, 5V, and GND pads
   - Or use connectors if available

2. **Connect to ESP32**
   - Connect Data to GPIO 5
   - Connect 5V to ESP32 5V pin
   - Connect GND to ESP32 GND

3. **Add Optional Controls**
   - Connect potentiometer to A0 and GND
   - Add buttons to desired GPIO pins

4. **Test**
   - Upload firmware
   - Test LED strip operation
   - Verify all LEDs are working

### Daisy-Chain Assembly

1. **Prepare All Strips**
   - Cut strips to desired lengths
   - Solder/connect data out of one strip to data in of next

2. **Connect Power**
   - Connect 5V and GND in parallel to all strips
   - Use power injection for long runs

3. **Connect to ESP32**
   - Connect first strip data in to ESP32 GPIO 5
   - Connect power to all strips

4. **Test**
   - Upload firmware
   - Test all strips in sequence
   - Verify data flow through all strips

## Maintenance

### Regular Maintenance

1. **Visual Inspection**
   - Check for loose connections
   - Look for damaged wires or connectors
   - Verify all LEDs are functioning

2. **Cleaning**
   - Clean LED strips with soft cloth
   - Avoid harsh chemicals
   - Keep connections dry

3. **Firmware Updates**
   - Regularly update firmware
   - Backup configurations before updates
   - Test after updates

### Troubleshooting

1. **LEDs Not Working**
   - Check power connections
   - Verify data connections
   - Test with known-good strip

2. **Flickering or Random Behavior**
   - Check power supply voltage
   - Add capacitors near power connections
   - Verify data line integrity

3. **Color Issues**
   - Check data line connections
   - Verify LED strip type (WS2811 vs WS2812)
   - Check color order settings in firmware

## Bill of Materials

### Basic Single Stick

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32 | 1 | Any ESP32 variant |
| WS2812 LED Strip | 1m | Low density |
| 5V Power Supply | 1 | 2A minimum |
| JST SM Connectors | 2 | For LED strip |
| Wires | As needed | Various gauges |

### Daisy-Chain System (3 sticks)

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32 | 1 | Any ESP32 variant |
| WS2812 LED Strip | 3m | Low density |
| 5V Power Supply | 1 | 10A recommended |
| JST SM Connectors | 6 | For LED strips |
| Power Distribution Board | 1 | For parallel power |
| Wires | As needed | Various gauges |

### Full Installation (10 sticks)

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32 | 1-2 | Multiple for long runs |
| WS2812 LED Strip | 10m | Low density |
| 5V Power Supply | 1-2 | 20A total |
| JST SM Connectors | 20 | For LED strips |
| Power Distribution Boards | 2-3 | For power injection |
| Aluminum Channels | 10 | For mounting |
| Enclosures | 1-2 | For ESP32 protection |
| Wires | As needed | Various gauges |

## Additional Resources

- [FastLED Library Documentation](https://github.com/FastLED/FastLED)
- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [WS2812 Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812.pdf)
- [NeoPixel Uberguide](https://learn.adafruit.com/adafruit-neopixel-uberguide)

## Version History

- **v1.0** (Current): Initial Fire Sticks hardware documentation
- **v0.9**: Draft based on Serpent Fragment experience
