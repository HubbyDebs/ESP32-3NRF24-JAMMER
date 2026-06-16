// =============================================================
//  CYPHER BOX — ESP32 + OLED SSD1306 + 3x NRF24L01 + botones
//  Versión reorganizada:
//   - Máquina de estados real (loop() despacha al handler activo)
//   - Antirrebote no bloqueante por botón (sin delay() ni while)
//   - Inicialización VSPI explícita y compartida por las 3 radios
//   - executeSelectedMenuItem() solo cambia de estado, no bloquea
// =============================================================

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "RF24.h"

// Para BT / WiFi descomentar cuando implementes esos estados:
// #include <BluetoothSerial.h>
// #include <BLEDevice.h>
// #include <BleKeyboard.h>
// #include <WiFi.h>
// #include <DNSServer.h>
// #include <WebServer.h>

// =============================================================
//  PINES Y CONSTANTES
// =============================================================

// OLED
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDR      0x3C

// Botones / LED
#define UP_BUTTON_PIN     14
#define DOWN_BUTTON_PIN   12
#define SELECT_BUTTON_PIN 13
#define LED_PIN            2

// VSPI explícito (mejor que dejar los pines por defecto del SDK)
#define VSPI_SCK   18
#define VSPI_MISO  19
#define VSPI_MOSI  23

// Radios NRF24 — (CE, CSN)
#define R1_CE 27
#define R1_CS 15
#define R2_CE 26
#define R2_CS 25
#define R3_CE 17
#define R3_CS 32   

// Antirrebote
#define DEBOUNCE_MS 25

// =============================================================
//  OBJETOS GLOBALES
// =============================================================

struct Button {
  uint8_t  pin;
  bool     lastReading;
  bool     stableState;
  uint32_t lastChangeMs;
  bool     risingPress;   // true sólo durante el ciclo en que se detecta pulsación
};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

SPIClass vspi(VSPI);
RF24 radio(R1_CE, R1_CS, 16000000);
RF24 radio2(R2_CE, R2_CS, 16000000);
RF24 radio3(R3_CE, R3_CS, 16000000);

bool r1_status = false;
bool r2_status = false;
bool r3_status = false;

// =============================================================
//  MÁQUINA DE ESTADOS
// =============================================================
enum AppState {
  STATE_MENU,
  STATE_BT_JAM,
  STATE_DRONE_JAM,
  STATE_WIFI_JAM,
  STATE_MULTI_JAM,
  STATE_SETTINGS,
  STATE_HELP
};

AppState currentState   = STATE_MENU;
bool     stateJustChanged = true;   // permite a cada handler dibujar al entrar

inline void setState(AppState s) {
  currentState = s;
  stateJustChanged = true;
}

// =============================================================
//  MENÚ
// =============================================================
enum MenuItem {
  BT_JAM, DRONE_JAM, WIFI_JAM, MULTI_JAM, SETTINGS, HELP, NUM_MENU_ITEMS
};

static const char *menuLabels[NUM_MENU_ITEMS] = {
  "BT_JAM", "DRONE_JAM", "WIFI_JAM", "MULTI_JAM", "SETTINGS", "HELP"
};

int      firstVisibleMenuItem = 0;
MenuItem selectedMenuItem     = BT_JAM;

// =============================================================
//  BOTÓN — ANTIRREBOTE NO BLOQUEANTE
// =============================================================

Button btnUp, btnDown, btnSelect;

void buttonInit(Button &b, uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
  b.pin          = pin;
  b.lastReading  = HIGH;
  b.stableState  = HIGH;
  b.lastChangeMs = 0;
  b.risingPress  = false;
}

// Actualiza la máquina de estados del botón. Devuelve true UNA VEZ por pulsación.
bool buttonPoll(Button &b) {
  bool reading = digitalRead(b.pin);
  b.risingPress = false;

  if (reading != b.lastReading) {
    b.lastChangeMs = millis();
    b.lastReading  = reading;
  }

  if ((millis() - b.lastChangeMs) > DEBOUNCE_MS) {
    if (reading != b.stableState) {
      b.stableState = reading;
      if (b.stableState == LOW) {
        b.risingPress = true;   // flanco de bajada estable = pulsación
      }
    }
  }
  return b.risingPress;
}

// Llamar una vez por loop, antes de leer los flags
inline void buttonsUpdate() {
  buttonPoll(btnUp);
  buttonPoll(btnDown);
  buttonPoll(btnSelect);
}

inline bool anyButtonHeld() {
  return (btnUp.stableState     == LOW) ||
         (btnDown.stableState   == LOW) ||
         (btnSelect.stableState == LOW);
}

// =============================================================
//  UTILIDADES
// =============================================================
void nonBlockingDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    yield();
  }
}

// =============================================================
//  BITMAPS (splash)
// =============================================================
static const unsigned char PROGMEM image_EviSmile1_bits[] = { 0x30, 0x03, 0x00, 0x60, 0x01, 0x80, 0xe0, 0x01, 0xc0, 0xf3, 0xf3, 0xc0, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xc0, 0x7f, 0xff, 0x80, 0x7f, 0xff, 0x80, 0x7f, 0xff, 0x80, 0xef, 0xfd, 0xc0, 0xe7, 0xf9, 0xc0, 0xe3, 0xf1, 0xc0, 0xe1, 0xe1, 0xc0, 0xf1, 0xe3, 0xc0, 0xff, 0xff, 0xc0, 0x7f, 0xff, 0x80, 0x7b, 0xf7, 0x80, 0x3d, 0x2f, 0x00, 0x1e, 0x1e, 0x00, 0x0f, 0xfc, 0x00, 0x03, 0xf0, 0x00 };
static const unsigned char PROGMEM image_Ble_connected_bits[] = { 0x07, 0xc0, 0x1f, 0xf0, 0x3e, 0xf8, 0x7e, 0x7c, 0x76, 0xbc, 0xfa, 0xde, 0xfc, 0xbe, 0xfe, 0x7e, 0xfc, 0xbe, 0xfa, 0xde, 0x76, 0xbc, 0x7e, 0x7c, 0x3e, 0xf8, 0x1f, 0xf0, 0x07, 0xc0 };
static const unsigned char PROGMEM image_MHz_bits[] = { 0xc3, 0x61, 0x80, 0x00, 0xe7, 0x61, 0x80, 0x00, 0xff, 0x61, 0x80, 0x00, 0xff, 0x61, 0xbf, 0x80, 0xdb, 0x7f, 0xbf, 0x80, 0xdb, 0x7f, 0x83, 0x00, 0xdb, 0x61, 0x86, 0x00, 0xc3, 0x61, 0x8c, 0x00, 0xc3, 0x61, 0x98, 0x00, 0xc3, 0x61, 0xbf, 0x80, 0xc3, 0x61, 0xbf, 0x80 };
static const unsigned char PROGMEM image_Error_bits[] = { 0x03, 0xf0, 0x00, 0x0f, 0xfc, 0x00, 0x1f, 0xfe, 0x00, 0x3f, 0xff, 0x00, 0x73, 0xf3, 0x80, 0x71, 0xe3, 0x80, 0xf8, 0xc7, 0xc0, 0xfc, 0x0f, 0xc0, 0xfe, 0x1f, 0xc0, 0xfe, 0x1f, 0xc0, 0xfc, 0x0f, 0xc0, 0xf8, 0xc7, 0xc0, 0x71, 0xe3, 0x80, 0x73, 0xf3, 0x80, 0x3f, 0xff, 0x00, 0x1f, 0xfe, 0x00, 0x0f, 0xfc, 0x00, 0x03, 0xf0, 0x00 };
static const unsigned char PROGMEM image_Bluetooth_Idle_bits[] = { 0x20, 0xb0, 0x68, 0x30, 0x30, 0x68, 0xb0, 0x20 };
static const unsigned char PROGMEM image_off_text_bits[] = { 0x67, 0x70, 0x94, 0x40, 0x96, 0x60, 0x94, 0x40, 0x64, 0x40 };
static const unsigned char PROGMEM image_wifi_not_connected_bits[] = { 0x21, 0xf0, 0x00, 0x16, 0x0c, 0x00, 0x08, 0x03, 0x00, 0x25, 0xf0, 0x80, 0x42, 0x0c, 0x40, 0x89, 0x02, 0x20, 0x10, 0xa1, 0x00, 0x23, 0x58, 0x80, 0x04, 0x24, 0x00, 0x08, 0x52, 0x00, 0x01, 0xa8, 0x00, 0x02, 0x04, 0x00, 0x00, 0x42, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_volume_muted_bits[] = { 0x01, 0xc0, 0x00, 0x02, 0x40, 0x00, 0x04, 0x40, 0x00, 0x08, 0x40, 0x00, 0xf0, 0x50, 0x40, 0x80, 0x48, 0x80, 0x80, 0x45, 0x00, 0x80, 0x42, 0x00, 0x80, 0x45, 0x00, 0x80, 0x48, 0x80, 0xf0, 0x50, 0x40, 0x08, 0x40, 0x00, 0x04, 0x40, 0x00, 0x02, 0x40, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_network_not_connected_bits[] = { 0x82, 0x0e, 0x44, 0x0a, 0x28, 0x0a, 0x10, 0x0a, 0x28, 0xea, 0x44, 0xaa, 0x82, 0xaa, 0x00, 0xaa, 0x0e, 0xaa, 0x0a, 0xaa, 0x0a, 0xaa, 0x0a, 0xaa, 0xea, 0xaa, 0xaa, 0xaa, 0xee, 0xee, 0x00, 0x00 };
static const unsigned char PROGMEM image_microphone_muted_bits[] = { 0x87, 0x00, 0x4f, 0x80, 0x26, 0x80, 0x13, 0x80, 0x09, 0x80, 0x04, 0x80, 0x0a, 0x00, 0x0d, 0x00, 0x2e, 0xa0, 0x27, 0x40, 0x10, 0x20, 0x0f, 0x90, 0x02, 0x08, 0x02, 0x04, 0x0f, 0x82, 0x00, 0x00 };
static const unsigned char PROGMEM image_mute_text_bits[] = { 0x8a, 0x5d, 0xe0, 0xda, 0x49, 0x00, 0xaa, 0x49, 0xc0, 0x8a, 0x49, 0x00, 0x89, 0x89, 0xe0 };
static const unsigned char PROGMEM image_cross_contour_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x51, 0x40, 0x8a, 0x20, 0x44, 0x40, 0x20, 0x80, 0x11, 0x00, 0x20, 0x80, 0x44, 0x40, 0x8a, 0x20, 0x51, 0x40, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00 };

// =============================================================
//  DISPLAY
// =============================================================
void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) yield();
  }
  display.display();
  nonBlockingDelay(1500);
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);
}

void drawBorder() {
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
}

void drawMenu() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);

  // Barra de título
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(5, 4);
  display.setTextSize(1);
  display.println("Home");

  // Items visibles (2 a la vez)
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < 2; i++) {
    int menuIndex = (firstVisibleMenuItem + i) % NUM_MENU_ITEMS;
    int16_t y = 20 + (i * 20);

    if (selectedMenuItem == menuIndex) {
      display.fillRect(0, y - 2, SCREEN_WIDTH, 15, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(5, y);
    display.setTextSize(1);
    display.println(menuLabels[menuIndex]);
  }
  display.display();
}

void displayInfo(const char *title,
                 const char *info1 = "",
                 const char *info2 = "",
                 const char *info3 = "") {
  display.clearDisplay();
  drawBorder();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 4);
  display.println(title);
  display.drawLine(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);
  display.setCursor(4, 18); display.println(info1);
  display.setCursor(4, 28); display.println(info2);
  display.setCursor(4, 38); display.println(info3);
  display.display();
}

void demonSHIT() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr);
  display.setTextWrap(false);
  u8g2_for_adafruit_gfx.setCursor(30, 18);
  u8g2_for_adafruit_gfx.print("2.4 G H Z");
  u8g2_for_adafruit_gfx.setCursor(40, 35);
  u8g2_for_adafruit_gfx.print("J A M R");
  display.drawBitmap(56, 40, image_EviSmile1_bits, 18, 21, 1);
  display.drawBitmap(106, 19, image_Ble_connected_bits, 15, 15, 1);
  display.drawBitmap(2, 50, image_MHz_bits, 25, 11, 1);
  display.drawBitmap(1, 1, image_Error_bits, 18, 18, 1);
  display.drawBitmap(25, 38, image_Bluetooth_Idle_bits, 5, 8, 1);
  display.drawBitmap(83, 55, image_off_text_bits, 12, 5, 1);
  display.drawBitmap(109, 2, image_wifi_not_connected_bits, 19, 16, 1);
  display.drawBitmap(4, 31, image_volume_muted_bits, 18, 16, 1);
  display.drawBitmap(109, 45, image_network_not_connected_bits, 15, 16, 1);
  display.drawBitmap(92, 33, image_microphone_muted_bits, 15, 16, 1);
  display.drawBitmap(1, 23, image_mute_text_bits, 19, 5, 1);
  display.drawBitmap(32, 49, image_cross_contour_bits, 11, 16, 1);
  display.display();
}

void displayInfoScreen() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);
  u8g2_for_adafruit_gfx.setCursor(0, 22);
  u8g2_for_adafruit_gfx.print("HubbyDebs AI Jammer");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("Made with Gemini and Claude");
  u8g2_for_adafruit_gfx.setCursor(0, 38);
  u8g2_for_adafruit_gfx.print("Not responsible of anything!!!");
  u8g2_for_adafruit_gfx.setCursor(0, 54);
  u8g2_for_adafruit_gfx.print("Have fun and enjoy!");
  display.display();
}

// =============================================================
//  NRF24 — INIT COMPARTIDO EN VSPI
// =============================================================
static bool initOneRadio(RF24 &r, const char *label) {
  if (!r.begin(&vspi)) {
    Serial.printf("[%s] no responde\n", label);
    return false;
  }
  r.setAutoAck(false);
  r.stopListening();
  r.setRetries(0, 0);
  r.setPALevel(RF24_PA_MAX, true);
  r.setDataRate(RF24_2MBPS);
  r.setCRCLength(RF24_CRC_DISABLED);
  Serial.printf("[%s] OK\n", label);
  r.printPrettyDetails();
  r.startConstCarrier(RF24_PA_MAX, 45);
  return true;
}

void initRadios() {
  // Pines VSPI explícitos.
  // El -1 en SS evita que el SDK reserve un GPIO de chip-select por defecto:
  // cada RF24 maneja su propio CSN, no queremos hardware-CS automático.
  vspi.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, -1);

  r1_status = initOneRadio(radio, "R1");
  r2_status = initOneRadio(radio2, "R2");
  r3_status = initOneRadio(radio3, "R3");
}

 
void btJam() {
  radio2.setChannel(random(80));
  radio3.setChannel(random(80));
  radio.setChannel(random(80));
}


void droneJam() {
  ////RANDOM CHANNEL
  radio2.setChannel(random(126));
  radio3.setChannel(random(126));
  radio.setChannel(random(126));
  delayMicroseconds(random(60));  //////REMOVE IF SLOW
  /*  YOU CAN DO -----SWEEP CHANNEL
  for (int i = 0; i < 79; i++) {
    radio.setChannel(i);
*/
}

void singleChannel() {
  ////RANDOM CHANNEL
  radio2.setChannel(random(81));
  radio3.setChannel(random(15));
  radio.setChannel(random(15));
  delayMicroseconds(random(60));  //////REMOVE IF SLOW
}
void wifiJam() {
  // Define the set of channels you want to choose from
  int numbers[] = { 1, 6, 14 };
  int sizeOfArray = sizeof(numbers) / sizeof(numbers[0]);  // Calculate the size of the array

  // Generate a random index
  int randomIndex = random(sizeOfArray);  // random(max) generates a number from 0 to max-1

  // Select the random number from the array
  int randomNumber = numbers[randomIndex];

  radio.setChannel(randomNumber);
  radio2.setChannel(randomNumber);
  radio3.setChannel(randomNumber);

  // Output the result to the Serial Monitor
  Serial.print("Randomly selected channel: ");
  Serial.println(randomNumber);
}

void channelRange() {
  int randomNumber = random(40, 81);  // 81 because the upper bound is exclusive

  // Output the result to the Serial Monitor
  Serial.print("Randomly selected number between 40 and 80: ");
  Serial.println(randomNumber);

  // Example usage with radio.setChannel
  radio.setChannel(randomNumber);
  radio2.setChannel(randomNumber);
  radio3.setChannel(randomNumber);
}

// =============================================================
//  HANDLERS DE ESTADO
// =============================================================

// Sólo cambia de estado. NO bloquea con while/delay.
void executeSelectedMenuItem() {
  switch (selectedMenuItem) {
    case BT_JAM:      setState(STATE_BT_JAM);      break;
    case DRONE_JAM:   setState(STATE_DRONE_JAM);   break;
    case WIFI_JAM:    setState(STATE_WIFI_JAM);    break;
    case MULTI_JAM:   setState(STATE_MULTI_JAM);   break;
    case SETTINGS:     setState(STATE_SETTINGS);     break;
    case HELP:         setState(STATE_HELP);         break;
    default: break;
  }
}

void handleMenu() {
  if (stateJustChanged) {
    drawMenu();
    stateJustChanged = false;
  }

  if (btnUp.risingPress) {
    selectedMenuItem = static_cast<MenuItem>(
      (selectedMenuItem == 0) ? (NUM_MENU_ITEMS - 1) : (selectedMenuItem - 1));
    if (selectedMenuItem == (NUM_MENU_ITEMS - 1)) {
      firstVisibleMenuItem = NUM_MENU_ITEMS - 2;
    } else if (selectedMenuItem < firstVisibleMenuItem) {
      firstVisibleMenuItem = selectedMenuItem;
    }
    drawMenu();
  }

  if (btnDown.risingPress) {
    selectedMenuItem = static_cast<MenuItem>(
      (selectedMenuItem + 1) % NUM_MENU_ITEMS);
    if (selectedMenuItem == 0) {
      firstVisibleMenuItem = 0;
    } else if (selectedMenuItem >= (firstVisibleMenuItem + 2)) {
      firstVisibleMenuItem = selectedMenuItem - 1;
    }
    drawMenu();
  }

  if (btnSelect.risingPress) {
    executeSelectedMenuItem();
  }
}

// =================================================================
// LÓGICA DE ESTADOS (TOTALMENTE NO BLOQUEANTE)
// =================================================================

void handleBT() {
  // 1. Lo que ocurre la PRIMERA VEZ que entramos al estado
  if (stateJustChanged) { 
    Serial.println("BT YOLO button pressed");
    displayInfo("BT YOLO", "ACTIVATING RADIOS", "Starting....");
    initRadios();
    // Actualizamos la pantalla sin usar delays bloqueantes
    displayInfo("BT YOLO", "RADIOS ACTIVE", "Running....");
    stateJustChanged = false; 
  }
  
  // 2. Lo que ocurre de forma continua en cada vuelta del loop()
  btJam(); 

  // 3. Cómo salimos de este estado
  if (btnSelect.risingPress) {
    // radio.stopConstCarrier(); // Opcional: apagar radios al salir
    setState(STATE_MENU);
  }
}

void handleDrone() {
  if (stateJustChanged) { 
    Serial.println("DRONE YOLO button pressed");
    displayInfo("DRONE YOLO", "ACTIVATING RADIOS", "Starting....");
    initRadios();
    displayInfo("DRONE YOLO", "RADIOS ACTIVE", "Running....");
    stateJustChanged = false; 
  }
  
  droneJam(); 
  
  if (btnSelect.risingPress) {
    setState(STATE_MENU);
  }
}

void handleWifi() {
  if (stateJustChanged) { 
    Serial.println("WIFI YOLO button pressed");
    displayInfo("WIFI YOLO", "ACTIVATING RADIOS", "Starting....");
    initRadios();
    displayInfo("WIFI YOLO", "RADIOS ACTIVE", "Running....");
    stateJustChanged = false; 
  }
  
  wifiJam(); 
  
  if (btnSelect.risingPress) {
    setState(STATE_MENU);
  }
}

void handleMulti() {
  if (stateJustChanged) { 
    Serial.println("MULTI YOLO button pressed");
    displayInfo("MULTI CH YOLO", "ACTIVATING RADIOS", "Starting....");
    initRadios();
    displayInfo("MULTI CH YOLO", "RADIOS ACTIVE", "Running....");
    stateJustChanged = false; 
  }
  
  droneJam(); // Nota: En tu código base llamaba a droneYOLO aquí, respetado tal cual.
  
  if (btnSelect.risingPress) {
    setState(STATE_MENU);
  }
}

void handleSettings() {
  if (stateJustChanged) {
    char line1[24], line2[24], line3[24];
    snprintf(line1, sizeof(line1), "Radio 1: %s", r1_status ? "OK" : "FAIL");
    snprintf(line2, sizeof(line2), "Radio 2: %s", r2_status ? "OK" : "FAIL");
    snprintf(line3, sizeof(line3), "Radio 3: %s", r3_status ? "OK" : "FAIL");
    displayInfo("You need to jam once", line1, line2, line3);
    stateJustChanged = false;
  }

  if (btnSelect.risingPress) {
    setState(STATE_MENU);
  }
}

void handleHelp() {
  if (stateJustChanged) {
    displayInfo("Help",
                "UP/DOWN: navega",
                "SELECT: entrar",
                "SELECT en pantalla: volver");
    stateJustChanged = false;
  }
  
  if (btnSelect.risingPress) {
    setState(STATE_MENU);
  }
}
// =============================================================
//  SETUP / LOOP
// =============================================================
void setup() {
  Serial.begin(115200);
  nonBlockingDelay(10);

  pinMode(LED_PIN, OUTPUT);
  buttonInit(btnUp,     UP_BUTTON_PIN);
  buttonInit(btnDown,   DOWN_BUTTON_PIN);
  buttonInit(btnSelect, SELECT_BUTTON_PIN);
  Serial.println("Buttons init'd");

  initDisplay();

  // Splash
  demonSHIT();
  nonBlockingDelay(3000);
  displayInfoScreen();
  nonBlockingDelay(2000);

  initRadios();

  setState(STATE_MENU);
}

void loop() {
  // 1) actualiza todos los botones una sola vez por ciclo
  buttonsUpdate();
  
  // 2) feedback visual del LED mientras hay un botón mantenido
  digitalWrite(LED_PIN, anyButtonHeld() ? HIGH : LOW);
  
  // 3) despacho según estado activo (Lógica LIMPIA)
  switch (currentState) {
    case STATE_MENU:       handleMenu();     break;
    case STATE_BT_JAM:     handleBT();       break;
    case STATE_DRONE_JAM:  handleDrone();    break;
    case STATE_WIFI_JAM:   handleWifi();     break;
    case STATE_MULTI_JAM:  handleMulti();    break;
    case STATE_SETTINGS:   handleSettings(); break;
    case STATE_HELP:       handleHelp();     break;
  }
}
