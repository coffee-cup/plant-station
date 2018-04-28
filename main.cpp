#include <Arduino.h>
#include <LiquidCrystal.h>

extern HardwareSerial Serial;

// Connections:
// rs (LCD pin 4) to Arduino pin 12
// rw (LCD pin 5) to Arduino pin 11
// enable (LCD pin 6) to Arduino pin 10
// LCD pin 15 to Arduino pin 13
// LCD pins d4, d5, d6, d7 to Arduino pins 5, 4, 3, 2
LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);
int backLight = 8;

void setup() {
    pinMode(backLight, OUTPUT);
    digitalWrite(backLight, HIGH);

    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("hello world");

    Serial.begin(19200);
}

void loop() {
}
