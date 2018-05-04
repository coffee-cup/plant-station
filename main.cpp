#include "Scheduler.h"
#include "Wifi.h"
#include "timings.h"
#include <Arduino.h>
// #include <LiquidCrystal.h>
#include <SoftwareSerial.h>

#define MOISTURE_WATER (160)
#define MOISTURE_AIR (697)

#define WIFI_SSID ("Corner G & The Rub")
#define WIFI_PASSWORD ("sexyfishtalk")

#define PLANTS_HOST ("192.168.0.14")
#define PLANTS_PORT (4000)
#define PLANTS_PATH ("/sensor/plants")

#define MOISTURE_PIN (A0)
#define LIGHT_PIN (A1)
#define TEMP_PIN (A2)

#define ALPHA (0.25) // < 0.5 weights previous values heavier
#define MOVING_AVERAGE(new_value, old_value)                                   \
    ((new_value * ALPHA) + (old_value * (1 - ALPHA)))

SoftwareSerial ESPserial(2, 3); // RX | TX
Wifi wifi(&ESPserial, &Serial, -1);

extern HardwareSerial Serial;

// Connections:
// rs (LCD pin 4) to Arduino pin 12
// rw (LCD pin 5) to Arduino pin 11
// enable (LCD pin 6) to Arduino pin 10
// LCD pin 15 to Arduino pin 8
// LCD pins d4, d5, d6, d7 to Arduino pins 5, 4, 3, 2
// LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);
int backLight = 8;

uint8_t moisture_value = 0;
uint8_t light_value = 0;
float temp_value = 0;

char row_buf[16];
// void updateLcd() {
//     lcd.clear();

//     // First row
//     lcd.setCursor(0, 0);
//     sprintf(row_buf, "Moisture : %d", moisture_value);
//     lcd.print(row_buf);

//     // Second row
//     lcd.setCursor(0, 1);
//     sprintf(row_buf, "Light    : %d", light_value);
//     lcd.print(row_buf);
// }

void updateMoisture() {
    uint16_t value = analogRead(MOISTURE_PIN);
    uint16_t new_moisture_value =
        constrain(map(value, MOISTURE_WATER, MOISTURE_AIR, 100, 0), 0, 100);
    moisture_value = MOVING_AVERAGE(new_moisture_value, moisture_value);
}

void updateLight() {
    uint16_t value = analogRead(LIGHT_PIN);
    uint8_t new_light_value = constrain(map(value, 0, 1023, 0, 100), 0, 100);
    light_value = MOVING_AVERAGE(new_light_value, light_value);
}

void updateTemp() {
    uint16_t value = analogRead(TEMP_PIN);
    float voltage = value * 5.0; // 5 V
    voltage /= 1024.0;

    // converting from 10 mv per degree wit 500 mV offset
    // to degrees ((voltage - 500mV) times 100)
    float new_temp_value = (voltage - 0.5) * 100;
    temp_value = MOVING_AVERAGE(new_temp_value, temp_value);
}

String createJson() {
    // {"m": 1, "l": 1, "t": 1}
    String data = "{\"m\":";
    data += moisture_value;
    data += ",\"l\":";
    data += light_value;
    data += ",\"t\":";
    data += temp_value;
    data += "}";
    return data;
}

void sendData() {
    wifi.postRequest(PLANTS_HOST, PLANTS_PATH, createJson(), PLANTS_PORT, 4000);
}

void idle(uint32_t idle_period) { delay(idle_period); }

void setup() {
    // pinMode(backLight, OUTPUT);
    // digitalWrite(backLight, HIGH);

    // lcd.begin(16, 2);
    // lcd.clear();
    // lcd.print("it is time.");

    Serial.begin(19200);
    ESPserial.begin(115200);

    // Force baud rate to 9600 because that is all software serial can handle
    ESPserial.println("AT+UART=9600,8,1,0,0");
    delay(1000);
    ESPserial.end();
    ESPserial.begin(9600);

    wifi.begin();
    delay(10);
    wifi.connectToAP(WIFI_SSID, WIFI_PASSWORD);

    // Scheduler setup
    SchedulerInit();
    // SchedulerStartTask(UPDATE_LCD_DELAY, UPDATE_LCD_PERIOD, updateLcd);
    SchedulerStartTask(MOISTURE_DELAY, MOISTURE_PERIOD, updateMoisture);
    SchedulerStartTask(LIGHT_DELAY, LIGHT_PERIOD, updateLight);
    SchedulerStartTask(TEMP_DELAY, TEMP_PERIOD, updateTemp);
    SchedulerStartTask(DATA_DELAY, DATA_PERIOD, sendData);
}

void loop() {
    uint32_t idle_period = SchedulerDispatch();
    if (idle_period) {
        idle(idle_period);
    }
}
