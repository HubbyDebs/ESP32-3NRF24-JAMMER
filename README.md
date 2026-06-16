# 🔥 CYPHER BOX — ESP32 Dual RF24 2.4GHz Jammer

Un inhibidor de señal RF basado en **ESP32** con tres módulos **NRF24L01** operando en la banda de **2.4 GHz**, con interfaz gráfica OLED y control mediante botones.

## 📋 Descripción

**CYPHER BOX** es un dispositivo de prueba/demostración que implementa una máquina de estados no bloqueante para jamming de múltiples protocolos RF en 2.4 GHz:

- **Bluetooth (BT JAM):** Jamming de dispositivos Bluetooth en canales 0-79
- **Drones (DRONE JAM):** Cobertura completa de canales 0-125 (2.4GHz)
- **WiFi (WIFI JAM):** Enfoque en canales WiFi estándar (1, 6, 14)
- **Multi-Canal (MULTI JAM):** Barrido aleatorio de múltiples frecuencias
- **Configuración & Ayuda:** Sistema de diagnóstico en tiempo real

## ⚙️ Hardware

### Componentes Principales

| Componente | Especificación |
|-----------|----------------|
| **Microcontrolador** | ESP32 (2 núcleos, 240 MHz) |
| **Módulos RF** | 3x NRF24L01 (2.4 GHz, SPI) |
| **Pantalla** | OLED SSD1306 (128x64, I2C) |
| **Entrada de Usuario** | 3x Botones con antirrebote |
| **Indicador Visual** | LED de estado |
| **Bus de Comunicación** | VSPI compartido (SPI) |

### Pines ESP32

```
OLED (I2C):
  - SDA (GPIO21)
  - SCL (GPIO22)

Botones:
  - UP:       GPIO14
  - DOWN:     GPIO12
  - SELECT:   GPIO13
  - LED:      GPIO2

NRF24L01 (VSPI - Compartido):
  - SCK:      GPIO18
  - MISO:     GPIO19
  - MOSI:     GPIO23
  - SS:       -1 (cada radio gestiona su CSN)

Radio 1 (NRF24 #1):
  - CE:       GPIO27
  - CSN:      GPIO15

Radio 2 (NRF24 #2):
  - CE:       GPIO26
  - CSN:      GPIO25

Radio 3 (NRF24 #3):
  - CE:       GPIO17
  - CSN:      GPIO32 ⚠️ (GPIO strapping pin, considerar cambiar)
```

## 🎮 Interfaz de Usuario

### Navegación del Menú

```
┌─────────────────────┐
│      Home           │  ← Barra de título
├─────────────────────┤
│ ► BT_JAM            │  ← Seleccionado
│   DRONE_JAM         │
├─────────────────────┤
UP/DOWN:    Navegar por opciones
SELECT:     Activar opción seleccionada
            (Retorno a menú desde cualquier estado)
```

### Estados Disponibles

1. **BT_JAM** - Jamming Bluetooth
2. **DRONE_JAM** - Jamming de Drones
3. **WIFI_JAM** - Jamming WiFi
4. **MULTI_JAM** - Multi-canal
5. **SETTINGS** - Diagnostico de radios
6. **HELP** - Ayuda en pantalla

## 💻 Arquitectura de Software

### Máquina de Estados No Bloqueante

El firmware implementa una máquina de estados limpia donde:

```c
// Loop principal - NUNCA bloquea
void loop() {
  buttonsUpdate();           // Escaneo de botones (antirrebote)
  digitalWrite(LED, button_held);  // Feedback LED
  
  // Despacho por estado
  switch(currentState) {
    case STATE_MENU:       handleMenu();     break;
    case STATE_BT_JAM:     handleBT();       break;
    case STATE_DRONE_JAM:  handleDrone();    break;
    // ...
  }
}
```

### Antirrebote No Bloqueante

Cada botón implementa una máquina de estados con:
- Detección de flancos estables
- Ventana de debounce (25ms)
- Flag de pulsación única por ciclo (`risingPress`)
- Sin `delay()` ni bucles bloqueantes

```c
struct Button {
  uint8_t  pin;
  bool     lastReading;
  bool     stableState;
  uint32_t lastChangeMs;
  bool     risingPress;     // true solo durante la detección de pulsación
};
```

### Inicialización Compartida VSPI

Los tres módulos NRF24 comparten un único bus SPI (VSPI) con manejo independiente del chip-select (CSN):

```c
vspi.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, -1);  // -1 = sin CS automático

r1_status = initOneRadio(radio, "R1");
r2_status = initOneRadio(radio2, "R2");
r3_status = initOneRadio(radio3, "R3");
```

### Configuración de Radios NRF24

Cada radio se inicializa con:
- **Auto-ACK:** Deshabilitado (transmisión cruda)
- **Retransmisiones:** 0 (envío único)
- **Potencia:** RF24_PA_MAX
- **Velocidad de datos:** 2 Mbps
- **CRC:** Deshabilitado
- **Modo:** Portadora continua (jamming)

## 🎯 Modos de Jamming

### 1. BT_JAM - Bluetooth
```c
Canales: 0-79 (adaptado a especificación Bluetooth)
Estrategia: Selección aleatoria de canal en cada ciclo
Objetivo: Interferencia en dispositivos Bluetooth 2.4GHz
```

### 2. DRONE_JAM - Drones
```c
Canales: 0-125 (cobertura completa 2.4GHz)
Estrategia: Barrido aleatorio + micro-delays
Objetivo: Disrupción de control de drones
```

### 3. WIFI_JAM - WiFi
```c
Canales: 1, 6, 14 (canales WiFi estándar)
Estrategia: Rotación entre canales WiFi críticos
Objetivo: Interferencia en redes WiFi 2.4GHz
```

### 4. MULTI_JAM - Multi-canal
```c
Estrategia: Barrido aleatorio en rango 0-125
Objetivo: Cobertura amplia del espectro 2.4GHz
```

## 📦 Dependencias

```cpp
#include <Wire.h>              // I2C (OLED)
#include <SPI.h>               // SPI (NRF24)
#include <Adafruit_GFX.h>      // Gráficos OLED
#include <Adafruit_SSD1306.h>  // Driver OLED
#include <U8g2_for_Adafruit_GFX.h>  // Fuentes adicionales
#include "RF24.h"              // Driver NRF24L01
```

### Instalación via Arduino IDE

```
Sketch → Include Library → Manage Libraries

Buscar e instalar:
- Adafruit GFX Library
- Adafruit SSD1306
- U8g2
- RF24
```

## 🚀 Compilación e Instalación

1. **Placa:** Seleccionar `ESP32 Dev Module`
2. **Puerto Serial:** Seleccionar el puerto COM/ttyUSB0
3. **Velocidad Baud:** 115200

```bash
Sketch → Verificar/Compilar
Sketch → Subir
```

## 📊 Pantalla de Diagnóstico

Al iniciar o acceder a **SETTINGS**, se muestra:

```
┌──────────────────────┐
│ You need to jam once │
├──────────────────────┤
│ Radio 1: OK/FAIL     │
│ Radio 2: OK/FAIL     │
│ Radio 3: OK/FAIL     │
└──────────────────────┘
```

## ⚠️ Advertencias Legales

> **RESPONSABILIDAD LEGAL:** Este proyecto es ÚNICAMENTE para propósitos educativos, de investigación y demostración. El usuario es responsable de:
> - Cumplir con todas las regulaciones RF locales
> - Evitar interferencias con sistemas autorizados
> - No usar en áreas donde sea ilegal

**NO SOMOS RESPONSABLES de:**
- Interferencias no autorizadas
- Daños a equipos de terceros
- Violaciones de normativas de telecomunicaciones

## 📝 Créditos

- **Desarrollador:** HubbyDebs
- **Asistencia IA:** Gemini y Claude
- **Prototiping/Testing:** Comunidad OpenSource RF

## 📄 Licencia

MIT License - Úsalo libremente, pero bajo tu responsabilidad legal.

---

### 🔧 Troubleshooting

**Problema:** Radios no detectan
- Verificar voltaje de alimentación (3.3V exactos)
- Revisar conexiones de pines SPI
- Comprobar condensadores de desacople en NRF24

**Problema:** OLED no muestra nada
- Verificar dirección I2C (0x3C por defecto)
- Escanear I2C: `Wire.beginTransmission(0x3C)`

**Problema:** Botones no responden
- Verificar resistencias pull-up
- Ajustar `DEBOUNCE_MS` si es necesario

---

**¡Gracias por usar CYPHER BOX! 🚀**
