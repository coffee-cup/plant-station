#include "Scheduler.h"
#include "timings.h"
#include <Arduino.h>
#include <LiquidCrystal.h>

#define MAX_MOISTURE (400)

extern HardwareSerial Serial;

// Connections:
// rs (LCD pin 4) to Arduino pin 12
// rw (LCD pin 5) to Arduino pin 11
// enable (LCD pin 6) to Arduino pin 10
// LCD pin 15 to Arduino pin 13
// LCD pins d4, d5, d6, d7 to Arduino pins 5, 4, 3, 2
LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);
int backLight = 8;

uint8_t moisture_value = 0;
uint8_t light_value = 0;

char row_buf[16];
void updateLcd() {
    lcd.clear();

    // First row
    lcd.setCursor(0, 0);
    sprintf(row_buf, "Moisture : %d", moisture_value);
    lcd.print(row_buf);

    // Second row
    lcd.setCursor(0, 1);
    sprintf(row_buf, "Light    : %d", light_value);
    lcd.print(row_buf);
}

void updateMoisture() {
    uint16_t value = analogRead(A0);
    moisture_value = constrain(map(value, MAX_MOISTURE, 1020, 100, 0), 0, 100);
}

void updateLight() {
    uint16_t value = analogRead(A1);
    light_value = constrain(map(value, 0, 1023, 0, 100), 0, 100);
}

void idle(uint32_t idle_period) {
    // Do nothing
}

void setup() {
    pinMode(backLight, OUTPUT);
    digitalWrite(backLight, HIGH);

    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("it is time.");

    Serial.begin(19200);

    // Scheduler setup
    SchedulerInit();
    SchedulerStartTask(UPDATE_LCD_DELAY, UPDATE_LCD_PERIOD, updateLcd);
    SchedulerStartTask(MOISTURE_DELAY, MOISTURE_PERIOD, updateMoisture);
    SchedulerStartTask(LIGHT_DELAY, LIGHT_PERIOD, updateLight);
}

void loop() {
    uint32_t idle_period = SchedulerDispatch();
    if (idle_period) {
        idle(idle_period);
    }
}
