#include "Wifi.h"

// Constructor
Wifi::Wifi(Stream *s, Stream *d, int8_t r)
    : stream(s), debug(d), pinReset(r), bootMarker("ready\r\n"){};

boolean Wifi::begin() {
    if (pinReset != -1) {
        digitalWrite(pinReset, HIGH);
        pinMode(pinReset, OUTPUT);
    }
    return test(); }

boolean Wifi::test() {
    clearBuffer();
    writeData("AT");
    return find(NULL);
}

boolean Wifi::hardReset() {
    if (pinReset == -1)
        return false;
    digitalWrite(pinReset, LOW);
    delay(10);
    digitalWrite(pinReset, HIGH);
    return find(&bootMarker);
}

boolean Wifi::softReset() {
    clearBuffer();
    writeData("AT+RST");
    return find(&bootMarker);
}

boolean Wifi::connectToAP(String ssid, String pass) {
    setMode(WIFI_MODE_STATION);
    writeData("AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"");
    return find(NULL, 20000);
}

boolean Wifi::connectTCP(String host, int port) {
    String command = "AT+CIPSTART=\"TCP\",\"" + host + "\"," + String(port);
    writeData(command);
    return find();
}

boolean Wifi::getRequest(String host, String path, int port, int timeout) {
    clearBuffer();
    flush();

    if (!connectTCP(host, port)) {
        return false;
    }

    int dataLength = host.length() + path.length() + HTTP_GET_LENGTH;
    writeData("AT+CIPSEND=" + String(dataLength));

    String promptString = "> ";
    if (find(&promptString)) {
        String request = "GET " + path + " HTTP/1.1\r\n"; // 15
        request += "Host: " + host + "\r\n";              // 8
        request += "Connection: close\r\n\r\n";           // 21
        writeData(request);

        String sendOk = "SEND OK\r\n";
        return find(&sendOk, timeout);
    }
    return false;
}

boolean Wifi::postRequest(String host, String path, String body, int port,
                          int timeout) {
    clearBuffer();
    flush();

    if (!connectTCP(host, port)) {
        return false;
    }

    int bodyLength = body.length();
    int dataLength = host.length() + path.length() + bodyLength +
                     String(bodyLength).length() + HTTP_POST_LENGTH;
    writeData("AT+CIPSEND=" + String(dataLength));

    String promptString = "> ";
    if (find(&promptString)) {
        String request = "POST " + path + " HTTP/1.1\r\n";               // 16
        request += "Host: " + host + "\r\n";                             // 8
        request += "Connection: close\r\n";                              // 21
        request += "Content-Type: application/json\r\n";                 // 32
        request += "Content-Length: " + String(bodyLength) + "\r\n\r\n"; // 20
        request += body;

        writeData(request);

        String sendOk = "SEND OK\r\n";
        return find(&sendOk, timeout);
    }
    return false;
}

boolean Wifi::closeAP() {
    flush();
    return find();
}

boolean Wifi::closeTCP() {
    stream->println("AT+CIPCLOSE");
    return find();
}

boolean Wifi::setMode(uint8_t mode) {
    clearBuffer();
    writeData("AT+CWMODE=" + String(mode));
    return find();
}

void Wifi::writeData(String str) {
    stream->println(str);
    if (debug) {
        debug->println("--> " + str);
    }
    flush();
}

String Wifi::readLine() {
    String data = "";
    while (stream->available()) {
        char r = stream->read();
        if (r == '\n') {
            break;
        } else if (r == '\r') {
            // Wait for \n
        } else {
            data += r;
        }
    }
    if (debug && data.length() > 0) {
        debug->println("<-- " + data);
    }
    return data;
}

// Blocks until String `str` is read through the `stream`
// TODO: Add timeout option
boolean Wifi::find(String *str, int timeout) {
    String s;
    if (str == NULL) {
        s = "OK\r\n";
        str = &s;
    }

    uint8_t stringLength = str->length();
    uint8_t matchedLength = 0;

    unsigned long start_time = millis();

    while (matchedLength != stringLength) {
        if (stream->available()) {
            char c = stream->read();
            // if (debug) {
            //     debug->print(c);
            // }

            if (c == str->charAt(matchedLength)) {
                matchedLength += 1;
            } else {
                matchedLength = 0;
            }
        }

        // Check timeout
        if (timeout != -1 && millis() - start_time > timeout) {
            break;
        }
    }

    boolean found = matchedLength == stringLength;
    if (debug && found) {
        debug->println("<-- " + *str);
    }

    if (!found) {
        debug->println("--- timeout");
    }

    return found;
}

void Wifi::clearBuffer() {
    while (stream->available()) {
        readLine();
    }
}

void Wifi::flush() { stream->flush(); }
