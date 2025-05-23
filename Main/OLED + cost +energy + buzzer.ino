#include <Wire.h>
#include "DFRobot_INA219.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// LED pins
int redLED = 10;
int greenLED = 9;
int blueLED = 8;  // Unused but retained
// Define buzzer pin
int buzzerPin = 7;  // Change this to the pin you're using for the buzzer

DFRobot_INA219_IIC ina219(&Wire, INA219_I2C_ADDRESS4);

// ============== CHANGED: SCALING FOR DEMO ==============
const float FLAT_RATE = 1.443; // Now 1.443 HKD/mWh (artificially scaled)
const float ENERGY_SCALE = 1000000.0; // Convert kWh to mWh
// =======================================================

// Energy tracking
float totalEnergy = 0;  // Cumulative kWh (internally tracked)
unsigned long lastUpdate = 0;

void setup() {
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(blueLED, OUTPUT); // Initialize (unused)
  // Initialize buzzer pin as output
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(115200);
  while(!Serial);
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
  // Read sensor data
  ina219.linearCalibrate(1000, 1000);
  float busVoltage = ina219.getBusVoltage_V();
  float shuntVoltage = ina219.getShuntVoltage_mV();
  float current = ina219.getCurrent_mA();
  float power = ina219.getPower_mW();

  // Update energy (kWh)
  unsigned long now = millis();
  totalEnergy += (power / 1e6) * ((now - lastUpdate) / 3600000.0);
  lastUpdate = now;

  // LED control based on shunt voltage
  if (shuntVoltage < 6.0 && shuntVoltage > 0.5) {
        digitalWrite(greenLED, HIGH); // Turn on green LED
        digitalWrite(redLED, LOW);   // Turn off red LED
        digitalWrite(buzzerPin, LOW); // Turn off buzzer
    } else if (shuntVoltage > 6.0) {
        digitalWrite(redLED, HIGH);  // Turn on red LED
        digitalWrite(greenLED, LOW); // Turn off green LED
        digitalWrite(buzzerPin, HIGH); // Turn on buzzer
    } else {
        // If shunt voltage is between 5mV and 6mV, turn off both LEDs and buzzer
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, LOW);
        digitalWrite(buzzerPin, LOW);
    }

  // ============== CHANGED: SCALED ENERGY & COST ==============
  float scaledEnergy = totalEnergy * ENERGY_SCALE; // kWh → mWh
  float cost = scaledEnergy * FLAT_RATE;
  // =======================================================

  // OLED display (unchanged except labels)
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Shunt: "); display.print(shuntVoltage, 2); display.print(" mV");
  display.setCursor(0, 16);
  display.print("I: "); display.print(current, 1); display.print(" mA");
  display.setCursor(0, 32);
  display.print("mWh: "); display.print(scaledEnergy, 1); // Changed to mWh
  display.setCursor(0, 48);
  display.print("Cost:"); display.print(cost, 2);
  display.display();

  // Serial prints (updated labels)
  Serial.print("BusVoltage:   ");
  Serial.print(busVoltage, 2);
  Serial.println(" V");
  Serial.print("ShuntVoltage: ");
  Serial.print(shuntVoltage, 3);
  Serial.println(" mV");
  Serial.print("Current:      ");
  Serial.print(current, 1);
  Serial.println(" mA");
  Serial.print("Power:        ");
  Serial.print(power, 1);
  Serial.println(" mW");
  Serial.print("Total Energy: ");
  Serial.print(scaledEnergy, 1); // Changed to mWh
  Serial.println(" mWh");
  Serial.print("Cost:         HK$");
  Serial.println(cost, 2);
  Serial.println("----------------------");
  delay(1000);
}
