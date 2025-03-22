#include <Wire.h>
#include "DFRobot_INA219.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <MFRC522.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RFID Pins
#define SS_PIN 53
#define RST_PIN 5
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// LED Pins
#define LED_R 9
#define LED_G 10
#define LED_B 11

// Auth Flags
enum AuthStatus { NOT_CHECKED, GRANTED, DENIED };
AuthStatus authStatus = NOT_CHECKED;
bool showEnergyCost = false;
unsigned long displayStartTime = 0;

// Power Monitoring
DFRobot_INA219_IIC ina219(&Wire, INA219_I2C_ADDRESS4);
const float FLAT_RATE = 1.443; 
const float ENERGY_SCALE = 1000000.0;
float totalEnergy = 0;
unsigned long lastUpdate = 0;

// Authorized UID
const byte AUTHORIZED_UID[4] = {0x4C, 0x22, 0x7E, 0x21};

void setup() {
  Serial.begin(115200);
  SPI.begin();
  
  // Initialize RFID
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  
  // Initialize LEDs
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLED(0, 0, 0);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed"));
    for (;;);
  }
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
     


  // Initialize INA219
  while (ina219.begin() != true) {
    Serial.println("INA219 failed");
    delay(2000);
  }
}

void loop() {
  // Update power data
  updatePowerData();
  
  // Check RFID
  checkRFID();

  // Control OLED display
  if (showEnergyCost && (millis() - displayStartTime < 2000)) {
    displayEnergyCost();
  } else {
    showEnergyCost = false;
    displayPowerData();
  }
}

void updatePowerData() {
  ina219.linearCalibrate(1000, 1000);
  float busVoltage = ina219.getBusVoltage_V();
  float shuntVoltage = ina219.getShuntVoltage_mV();
  float current = ina219.getCurrent_mA();
  float power = ina219.getPower_mW();

  unsigned long now = millis();
  totalEnergy += (power / 1e6) * ((now - lastUpdate) / 3600000.0);
  lastUpdate = now;
}

void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  if (memcmp(rfid.uid.uidByte, AUTHORIZED_UID, 4) == 0) {
    authStatus = GRANTED;
    setLED(0, 255, 0);  // Green
    showEnergyCost = true;
    displayStartTime = millis();
  } else {
    authStatus = DENIED;
    setLED(255, 0, 0);  // Red
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(500);
  setLED(0, 0, 0);
}

void displayPowerData() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("V: "); display.print(ina219.getBusVoltage_V(), 1); display.println(" V");
  display.setCursor(0, 16);
  display.print("I: "); display.print(ina219.getCurrent_mA(), 1); display.println(" mA");
  display.setCursor(0, 32);
  display.print("P: "); display.print(ina219.getPower_mW(), 1); display.println(" mW");
  display.display();
}

void displayEnergyCost() {
  float scaledEnergy = totalEnergy * ENERGY_SCALE;
  float cost = scaledEnergy * FLAT_RATE;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("mWh: "); display.println(scaledEnergy, 1);
  display.setCursor(0, 16);
  display.print("Cost: HK$"); display.println(cost, 2);
  display.display();
  delay(5000);
}

void setLED(int r, int g, int b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}
