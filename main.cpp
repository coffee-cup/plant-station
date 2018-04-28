#include "Scheduler.h"
#include "timings.h"
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

char row_buf[16];
void updateLcd() {
    int time = millis() / 1000;
    lcd.clear();

    // First row
    lcd.setCursor(0, 0);
    sprintf(row_buf, "%d ms", time);
    lcd.print(row_buf);

    Serial.print("yes\n");
}

void idle(uint32_t idle_period) {
    // Do nothing
}

void setup() {
    pinMode(backLight, OUTPUT);
    digitalWrite(backLight, HIGH);

    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("hello world");

    Serial.begin(9600);

    // Scheduler setup
    SchedulerInit();
    SchedulerStartTask(UPDATE_LCD_DELAY, UPDATE_LCD_PERIOD, updateLcd);
}

void loop() {
    uint32_t idle_period = SchedulerDispatch();
    if (idle_period) {
        idle(idle_period);
    }
}
