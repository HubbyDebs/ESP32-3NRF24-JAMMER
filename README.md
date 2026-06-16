# 🔥 CYPHER BOX — ESP32 Triple RF24 2.4GHz Jammer

A signal jamming device based on **ESP32** with three **NRF24L01** modules operating in the **2.4 GHz band**, featuring an OLED graphical interface and button control.

## 📋 Description

**CYPHER BOX** is a testing/demonstration device that implements a non-blocking state machine for jamming multiple RF protocols in 2.4 GHz:

- **Bluetooth (BT JAM):** Jamming Bluetooth devices on channels 0-79
- **Drones (DRONE JAM):** Full channel coverage 0-125 (2.4GHz)
- **WiFi (WIFI JAM):** Focus on standard WiFi channels (1, 6, 14)
- **Multi-Channel (MULTI JAM):** Random sweep across multiple frequencies
- **Settings & Help:** Real-time diagnostic system

## ⚙️ Hardware

### Main Components

| Component | Specification | Recommended |
|-----------|----------------|------------|
| **Microcontroller** | ESP32 (2 cores, 240 MHz) | ESP32 DevKit v1 |
| **RF Modules** | 3x NRF24L01 (2.4 GHz, SPI) | **ML01** or **2GM4** |
| **Display** | OLED SSD1306 (128x64, I2C) | 128x64 pixels |
| **Voltage Regulator** | Step-down converter | AMS1117 (5V → 3.3V) |
| **User Input** | 3x Buttons with debouncing | Push buttons |
| **Visual Indicator** | Status LED | 3mm LED |
| **Communication Bus** | Shared VSPI (SPI) | Hardware SPI |

### NRF24L01 Module Selection

#### **ML01 (Recommended - Budget Friendly)**
```
✓ Standard NRF24L01 clone variant
✓ Wide availability
✓ Lower cost (~$1-2 USD)
✓ Good for prototyping
⚠️ Requires proper power supply regulation
```

#### **2GM4 (Recommended - Higher Reliability)**
```
✓ Enhanced NRF24L01 variant (nRF24L01+PA+LNA)
✓ Built-in Power Amplifier & Low Noise Amplifier
✓ Extended range
✓ Better noise immunity
✓ Requires careful decoupling
⚠️ Higher current consumption (~115mA peak)
```

### ESP32 Pinout

```
OLED (I2C):
  - SDA (GPIO21)
  - SCL (GPIO22)

Buttons:
  - UP:       GPIO14
  - DOWN:     GPIO12
  - SELECT:   GPIO13
  - LED:      GPIO2

NRF24L01 (VSPI - Shared):
  - SCK:      GPIO18
  - MISO:     GPIO19
  - MOSI:     GPIO23
  - SS:       -1 (each radio manages its own CSN)

Radio 1 (NRF24 #1):
  - CE:       GPIO27
  - CSN:      GPIO15

Radio 2 (NRF24 #2):
  - CE:       GPIO26
  - CSN:      GPIO25

Radio 3 (NRF24 #3):
  - CE:       GPIO17
  - CSN:      GPIO32 ⚠️ (GPIO strapping pin, consider changing)
```

## 📐 Power Supply Circuit Diagram

### Single NRF24L01 with AMS1117 (5V → 3.3V)

```
┌─────────────────────────────────────────────────────────────────┐
│                     NRF24L01 Power Supply Module                │
└─────────────────────────────────────────────────────────────────┘

        ┌──────────────────────────────────────────────┐
        │          AMS1117 (5V → 3.3V)                │
        │                                              │
   5V ──┤IN  GND                                       │
        │       ╱╲                                     │
        │      ╱  ╲                                    │
        │     ╱ .. ╲                                   │
        │          └────┬──────────────────┬───────┐   │
        │                │                 │       │   │
        │              ┌─┴─┐              │      │   │
        │              │GND│              │      │   │
        │              └───┘              │      │   │
        │               │                 │      │   │
        │               │                 │      │   │
        │               │    OUT │───────┤      │   │
        │                       │       │       │   │
        └───────────────────────┼───────┼───────┼───┘
                                │       │       │
                               GND     │       │
                                       │       │
        ┌───────────────────────────────┴───────┴─────────────────┐
        │            NRF24L01 Power Distribution Network          │
        └───────────────────────────────────────────────────────┘

              3.3V Input (FROM AMS1117)
                    │
        ┌───────────┼───────────┐
        │           │           │
        │         ┌─┴──┐      ┌─┴──┐
        │         │10µF│      │ 100nF
        │         │Elec│      │(Ceramic)
        │         └──┬─┘      └──┬─┘
        │            │           │
        │           GND         GND
        │
        ├──────────────┬──────────────┤
        │              │              │
        │           (To VCC pin)      │
        │           (pin 1 or 2)      │
        │
        └─────────────────────────────┘


NRF24L01 Pin Configuration (8-pin DIP):
───────────────────────────────────────
  1(GND) ──●──○ 2(VCC)
  3(CE)  ──●──○ 4(CSN)
  5(SCK) ──●──○ 6(MOSI)
  7(MISO)──●──○ 8(IRQ)

Connection Summary for ONE NRF24L01:
────────────────────────────────────
  VCC (pin 2)    → 3.3V rail (after AMS1117)
  GND (pin 1)    → GND
  CE  (pin 3)    → ESP32 GPIO pin
  CSN (pin 4)    → ESP32 GPIO pin
  SCK (pin 5)    → ESP32 GPIO18 (VSPI)
  MOSI(pin 6)    → ESP32 GPIO23 (VSPI)
  MISO(pin 7)    → ESP32 GPIO19 (VSPI)
  IRQ (pin 8)    → GND (optional, can leave floating)
```

### Triple NRF24L01 Setup with Individual AMS1117 Regulators

```
┌──────────────────────────────────────────────────────────────────────────┐
│                  COMPLETE TRIPLE NRF24L01 POWER SUPPLY                   │
└──────────────────────────────────────────────────────────────────────────┘

                            +5V Input (USB/PSU)
                                  │
                    ┌─────────────┴─────────────┐
                    │                           │
                  ┌─┴─┐                       ┌─┴─┐
                  │470µF│                     │100µF│
                  │Elec │                     │Elec │
                  └──┬──┘                     └──┬──┘
                    GND                        GND

                    │ (+5V)                     │ (+5V)
        ┌───────────┴─────────────────────────┐
        │                                     │
        │  ┌──────────────────────────────────┼──────────┐
        │  │  ┌─────────────────────────────────────────┴──────┐
        │  │  │                                                 │
        │  │  │                                                 │
    ┌───┼──┼──┼─ AMS1117 #1              ┌────────────────────┐│
    │   │  │  │ (for Radio 1)            │                    ││
    │   │  │  │                          │ ┌───────────────┐  ││
    │ IN│--┼--┤OUT (3.3V)  ───────┬──────┼─┤10µF + 100nF   │  ││
    │   │  │  │                   │      │ └────────┬──────┘  ││
    │GND├──┼──┤GND            ────┼──────┼──────────┤──────────┘│
    │   │  │  │               │   │      │          │           │
    └───┼──┼──┴───────────────┼───┼──────┤ NRF24 #1 │           │
        │  │                  │   │      │ (ML01 or 2GM4)      │
        │  │                  │  GND    └──────────┘           │
        │  │                  │                                 │
        │  │  ┌────────────────────────────────────────────┐   │
        │  │  │                                            │   │
    ┌───┼──┼──┼─ AMS1117 #2              ┌───────────────┐│   │
    │   │  │  │ (for Radio 2)            │               ││   │
    │ IN│--┼──┤OUT (3.3V)  ────┬─────────┤10µF + 100nF   │    │
    │   │  │  │                │         └────────┬──────┘    │
    │GND├──┼──┤GND         ────┼─────────────────┤──────┐     │
    │   │  │  │            │   │                │       │     │
    └───┼──┼──┴────────────┼───┼──────── NRF24 #2       │     │
        │  │               │  GND                       │     │
        │  │               │                           │     │
        │  │  ┌────────────────────────────────────────┤─────┘
        │  │  │                                        │
    ┌───┼──┼──┼─ AMS1117 #3    ┌─────────────────┐   │
    │   │  │  │ (for Radio 3)  │                 │   │
    │ IN│--┼──┤OUT (3.3V) ┬────┤10µF + 100nF     │   │
    │   │  │  │           │    └────────┬────────┘   │
    │GND├──┼──┤GND    ────┼────────────┤───────┐    │
    │   │  │  │        │   │           │       │    │
    └───┼──┼──┴────────┼───┼───────NRF24 #3   │    │
        │  │           │  GND                 │    │
        │  │           │                      │    │
        │  │           │                      │    │
        │  └──────────────────────────────────┘    │
        │                                           │
        └───────────────────────────────────────────┘


                    Shared SPI Bus (ESP32)
                  ────────────────────────────
ESP32 GPIO18 (SCK)  ──┬──→ All NRF24 SCK pins
ESP32 GPIO23 (MOSI) ──┬──→ All NRF24 MOSI pins
ESP32 GPIO19 (MISO) ──┬──→ All NRF24 MISO pins (parallel)
ESP32 GPIO15 (CSN1) ──→ NRF24 #1 CSN
ESP32 GPIO25 (CSN2) ──→ NRF24 #2 CSN
ESP32 GPIO32 (CSN3) ──→ NRF24 #3 CSN
ESP32 GPIO27 (CE1)  ──→ NRF24 #1 CE
ESP32 GPIO26 (CE2)  ──→ NRF24 #2 CE
ESP32 GPIO17 (CE3)  ──→ NRF24 #3 CE

GND (ESP32) ────────→ All GND connections
```

## 💡 Component Recommendations

### Power Supply
```
Input:  5V / 1-2A (USB or external power supply)
Output: 3.3V / 500mA per AMS1117 regulator
Total:  Triple AMS1117 configuration for reliability
```

### Capacitors for Each NRF24L01 Module
```
Per Radio:
  • 10µF Electrolytic capacitor (input to 3.3V rail)
  • 100nF Ceramic capacitor (tight coupling to VCC)
  • Optional: 1µF ceramic for additional stability

All capacitors should be placed as close as possible to 
the NRF24 VCC and GND pins!
```

### PCB Layout Considerations
```
✓ Keep NRF24 antenna traces away from ESP32
✓ Use short wires for SPI connections
✓ Separate 3.3V rails for each radio if possible
✓ Ground plane recommended
✓ Star grounding at regulator
```

## 🎮 User Interface

### Menu Navigation

```
┌──────────────────────┐
│       Home           │  ← Title bar
├──────────────────────┤
│ ► BT_JAM             │  ← Selected
│   DRONE_JAM          │
├──────────────────────┤
UP/DOWN:    Navigate through options
SELECT:     Activate selected option
            (Return to menu from any state)
```

### Available States

1. **BT_JAM** - Bluetooth Jamming
2. **DRONE_JAM** - Drone Jamming
3. **WIFI_JAM** - WiFi Jamming
4. **MULTI_JAM** - Multi-channel
5. **SETTINGS** - Radio Diagnostics
6. **HELP** - On-screen Help

## 💻 Software Architecture

### Non-Blocking State Machine

The firmware implements a clean state machine where:

```c
// Main loop - NEVER blocks
void loop() {
  buttonsUpdate();           // Button scanning (debouncing)
  digitalWrite(LED, button_held);  // LED feedback
  
  // Dispatch by state
  switch(currentState) {
    case STATE_MENU:       handleMenu();     break;
    case STATE_BT_JAM:     handleBT();       break;
    case STATE_DRONE_JAM:  handleDrone();    break;
    // ...
  }
}
```

### Non-Blocking Debouncing

Each button implements a state machine with:
- Stable edge detection
- Debounce window (25ms)
- Single press flag per cycle (`risingPress`)
- No `delay()` or blocking loops

```c
struct Button {
  uint8_t  pin;
  bool     lastReading;
  bool     stableState;
  uint32_t lastChangeMs;
  bool     risingPress;     // true only during press detection
};
```

### Shared VSPI Initialization

The three NRF24 modules share a single SPI bus (VSPI) with independent chip-select (CSN) handling:

```c
vspi.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, -1);  // -1 = no automatic CS

r1_status = initOneRadio(radio, "R1");
r2_status = initOneRadio(radio2, "R2");
r3_status = initOneRadio(radio3, "R3");
```

### NRF24 Radio Configuration

Each radio is initialized with:
- **Auto-ACK:** Disabled (raw transmission)
- **Retransmissions:** 0 (single send)
- **Power:** RF24_PA_MAX
- **Data Rate:** 2 Mbps
- **CRC:** Disabled
- **Mode:** Continuous carrier (jamming)

## 🎯 Jamming Modes

### 1. BT_JAM - Bluetooth
```c
Channels: 0-79 (adapted to Bluetooth specification)
Strategy: Random channel selection each cycle
Target: Interference with 2.4GHz Bluetooth devices
```

### 2. DRONE_JAM - Drones
```c
Channels: 0-125 (full 2.4GHz coverage)
Strategy: Random sweep + micro-delays
Target: Drone control disruption
```

### 3. WIFI_JAM - WiFi
```c
Channels: 1, 6, 14 (standard WiFi channels)
Strategy: Rotation through critical WiFi channels
Target: Interference with 2.4GHz WiFi networks
```

### 4. MULTI_JAM - Multi-channel
```c
Strategy: Random sweep across 0-125 range
Target: Broad 2.4GHz spectrum coverage
```

## 📦 Dependencies

```cpp
#include <Wire.h>              // I2C (OLED)
#include <SPI.h>               // SPI (NRF24)
#include <Adafruit_GFX.h>      // OLED Graphics
#include <Adafruit_SSD1306.h>  // OLED Driver
#include <U8g2_for_Adafruit_GFX.h>  // Additional Fonts
#include "RF24.h"              // NRF24L01 Driver
```

### Installation via Arduino IDE

```
Sketch → Include Library → Manage Libraries

Search and install:
- Adafruit GFX Library
- Adafruit SSD1306
- U8g2
- RF24
```

## 🚀 Compilation & Installation

1. **Board:** Select `ESP32 Dev Module`
2. **Serial Port:** Select your COM/ttyUSB0 port
3. **Baud Rate:** 115200

```bash
Sketch → Verify/Compile
Sketch → Upload
```

## 📊 Diagnostic Screen

When starting or accessing **SETTINGS**, you'll see:

```
┌──────────────────────┐
│ You need to jam once │
├──────────────────────┤
│ Radio 1: OK/FAIL     │
│ Radio 2: OK/FAIL     │
│ Radio 3: OK/FAIL     │
└──────────────────────┘
```

## ⚠️ Legal Disclaimers

> **LEGAL RESPONSIBILITY:** This project is STRICTLY for educational, research, and demonstration purposes only. The user is responsible for:
> - Complying with all local RF regulations
> - Avoiding interference with authorized systems
> - Not using in areas where it may be illegal

**WE ARE NOT RESPONSIBLE FOR:**
- Unauthorized signal interference
- Damage to third-party equipment
- Violations of telecommunications regulations

## 📝 Credits

- **Developer:** HubbyDebs
- **AI Assistance:** Gemini and Claude
- **Prototyping/Testing:** OpenSource RF Community

## 📄 License

MIT License - Use it freely, but at your own legal responsibility.

---

### 🔧 Troubleshooting

**Issue:** Radios not detected
- Verify power supply voltage (exactly 3.3V on NRF24 VCC)
- Check SPI pin connections (SCK, MOSI, MISO)
- Verify decoupling capacitors on NRF24 modules
- Ensure AMS1117 regulators are properly connected
- Test each radio individually with separate AMS1117

**Issue:** OLED not displaying
- Verify I2C address (0x3C by default)
- Scan I2C: `Wire.beginTransmission(0x3C)`
- Check SDA/SCL pull-up resistors (4.7kΩ recommended)

**Issue:** Buttons not responding
- Check pull-up resistors (10kΩ recommended)
- Adjust `DEBOUNCE_MS` if necessary
- Verify GPIO pins are configured as INPUT_PULLUP

**Issue:** High power consumption or instability
- Use individual AMS1117 per radio (don't share)
- Add 470µF capacitor on 5V input
- Increase 10µF capacitor to 22µF per radio
- Ensure short wire connections to NRF24

**Issue:** Radio range is limited (2GM4 module)**
- Verify antenna connection (should not be loose)
- Check PA+LNA bias resistors
- Ensure adequate power supply (1-2A capacity)
- Position antennas away from metal

---

**Thank you for using CYPHER BOX! 🚀**
