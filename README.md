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

| Component | Specification |
|-----------|----------------|
| **Microcontroller** | ESP32 (2 cores, 240 MHz) |
| **RF Modules** | 3x NRF24L01 (2.4 GHz, SPI) |
| **Display** | OLED SSD1306 (128x64, I2C) |
| **User Input** | 3x Buttons with debouncing |
| **Visual Indicator** | Status LED |
| **Communication Bus** | Shared VSPI (SPI) |

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
> - Damage to third-party equipment
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
- Verify power supply voltage (exactly 3.3V)
- Check SPI pin connections
- Verify decoupling capacitors on NRF24 modules

**Issue:** OLED not displaying
- Verify I2C address (0x3C by default)
- Scan I2C: `Wire.beginTransmission(0x3C)`

**Issue:** Buttons not responding
- Check pull-up resistors
- Adjust `DEBOUNCE_MS` if necessary

---

**Thank you for using CYPHER BOX! 🚀**
