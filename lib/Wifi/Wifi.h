#ifndef _WIFI_H_
#define _WIFI_H_

#include <Arduino.h>

#define WIFI_MODE_STATION 0x01
#define WIFI_MODE_AP 0x02
#define WIFI_MODE_BOTH 0x03

#define WIFI_TCP_DISABLE 0x00
#define WIFI_TCP_ENABLE 0x01

#define HTTP_GET_LENGTH (44)

class Wifi {
  public:
    Wifi(Stream *s = &Serial, Stream *d = NULL, int8_t r = -1);

    boolean begin();
    boolean test();
    boolean hardReset();
    boolean softReset();
    boolean find(String *str = NULL);

    boolean connectToAP(String ssid, String pass);
    boolean connectTCP(String host, int port);
    boolean getRequest(String host, String path, int port = 80);

    String readLine();

    void closeAP();
    void closeTCP();

  private:
    void clearBuffer();
    void flush();

    void setMode(uint8_t mode);
    void writeData(String str);

    Stream *stream; // -> ESP8266, e.g. SoftwareSerial or Serial1
    Stream *debug;  // -> host, e.g. Serial

    boolean tcpConnected;
    int8_t pinReset;

    String bootMarker;
};

#endif
