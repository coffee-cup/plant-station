#ifndef _WIFI_H_
#define _WIFI_H_

#include <Arduino.h>

//AT+GMR
//AT version:1.2.0.0(Jul  1 2016 20:04:45)
//SDK version:1.5.4.1(39cb9a32)
//Ai-Thinker Technology Co. Ltd.
//Dec  2 2016 14:21:16
//OK

//2nd boot version : 1.5
//  SPI Speed      : 40MHz
//  SPI Mode       : DIO
//  SPI Flash Size & Map: 8Mbit(512KB+512KB)

#define WIFI_MODE_STATION 0x01
#define WIFI_MODE_AP 0x02
#define WIFI_MODE_BOTH 0x03

#define WIFI_TCP_DISABLE 0x00
#define WIFI_TCP_ENABLE 0x01

#define HTTP_GET_LENGTH (44)
#define HTTP_POST_LENGTH (97)

#define DEFAULT_TIMEOUT (4000) // ms

class Wifi {
  public:
    Wifi(Stream *s = &Serial, Stream *d = NULL, int8_t r = -1);

    boolean begin();
    boolean test();
    boolean hardReset();
    boolean softReset();
    boolean find(String *str = NULL, int timeout = DEFAULT_TIMEOUT);

    // AP
    boolean connectToAP(String ssid, String pass);
    boolean closeAP();

    // TCP
    boolean connectTCP(String host, int port);
    boolean closeTCP();

    // HTTP
    boolean getRequest(String host, String path, int port = 80,
                       int timeout = DEFAULT_TIMEOUT);
    boolean postRequest(String host, String path, String body, int port = 80,
                        int timeout = DEFAULT_TIMEOUT);

    String readLine();

  private:
    void clearBuffer();
    void flush();

    boolean setMode(uint8_t mode);
    void writeData(String str);

    Stream *stream; // -> ESP8266, e.g. SoftwareSerial or Serial1
    Stream *debug;  // -> host, e.g. Serial

    int8_t pinReset;

    String bootMarker;
};

#endif
