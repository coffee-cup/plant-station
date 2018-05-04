#include "Wifi.h"

// Constructor
Wifi::Wifi(Stream *s, Stream *d, int8_t r)
    : stream(s), debug(d), pinReset(r), bootMarker("ready\r\n"){};

boolean Wifi::begin() { return test(); }

boolean Wifi::test() {
    clearBuffer();
    writeData("AT");
    return find(NULL);
}

boolean Wifi::hardReset() {
    if (pinReset == -1)
        return false;
    digitalWrite(pinReset, LOW);
    pinMode(pinReset, OUTPUT);
    delay(10);
    pinMode(pinReset, INPUT);
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
    return find(NULL, -1);
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

// Override various timings. Passing 0 for an item keeps current setting.
// void Wifi::setTimeouts(uint32_t rcv, uint32_t rst, uint32_t con, uint32_t
// ipd) {
//     if (rcv) {
//         stream->setTimeout(rcv);
//         receiveTimeout = rcv;
//     }
//     if (rst)
//         resetTimeout = rst;
//     if (con)
//         connectTimeout = con;
//     if (ipd)
//         ipdTimeout = ipd;
// }

// // Override boot marker string, or pass NULL to restore default.
// void Wifi::setBootMarker(Fstr *s) { bootMarker = s ? s : defaultBootMarker; }

// // Anything printed to the EPS8266 object will be split to both the WiFi
// // and debug streams. Saves having to print everything twice in debug code.
// size_t Wifi::write(uint8_t c) {
//     if (debug) {
//         if (!writing) {
//             debug->print(F("---> "));
//             writing = true;
//         }
//         debug->write(c);
//     }
//     return stream->write(c);
// }

// // Equivalent to Arduino Stream find() function, but with search string in
// // flash/PROGMEM rather than RAM-resident. Returns true if string found
// // (any further pending input remains in stream), false if timeout occurs.
// // Can optionally pass NULL (or no argument) to read/purge the OK+CR/LF
// // returned by most AT commands. The ipd flag indicates this call follows
// // a CIPSEND request and might be broken into multiple sections with +IPD
// // delimiters, which must be parsed and handled (as the search string may
// // cross these delimiters and/or contain \r or \n itself).
// boolean Wifi::find(Fstr *str, boolean ipd) {
//     uint8_t stringLength, matchedLength = 0;
//     int c;
//     boolean found = false;
//     uint32_t t, save;
//     uint16_t bytesToGo = 0;

//     if (ipd) { // IPD stream stalls really long occasionally, what gives?
//         save = receiveTimeout;
//         setTimeouts(ipdTimeout);
//     }

//     if (str == NULL)
//         str = F("OK\r\n");
//     stringLength = strlen_P((Pchr *)str);

//     if (debug && writing) {
//         debug->print(F("<--- '"));
//         writing = false;
//     }

//     for (t = millis();;) {
//         if (ipd && !bytesToGo) {    // Expecting next IPD marker?
//             if (find(F("+IPD,"))) { // Find marker in stream
//                 for (;;) {
//                     if ((c = stream->read()) > 0) { // Read subsequent
//                     chars...
//                         if (debug)
//                             debug->write(c);
//                         if (c == ':')
//                             break; // ...until delimiter.
//                         bytesToGo = (bytesToGo * 10) + (c - '0'); // Keep
//                         count t = millis();   // Timeout resets w/each byte
//                         received
//                     } else if (c < 0) { // No data on stream, check for
//                     timeout
//                         if ((millis() - t) > receiveTimeout)
//                             goto bail;
//                     } else
//                         goto bail; // EOD on stream
//                 }
//             } else
//                 break; // OK (EOD) or ERROR
//         }

//         if ((c = stream->read()) > 0) { // Character received?
//             if (debug)
//                 debug->write(c); // Copy to debug stream
//             bytesToGo--;
//             if (c == pgm_read_byte((Pchr *)str +
//                                    matchedLength)) {   // Match next byte?
//                 if (++matchedLength == stringLength) { // Matched whole
//                 string?
//                     found = true;                      // Winner!
//                     break;
//                 }
//             } else { // Character mismatch; reset match pointer+counter
//                 matchedLength = 0;
//             }
//             t = millis();   // Timeout resets w/each byte received
//         } else if (c < 0) { // No data on stream, check for timeout
//             if ((millis() - t) > receiveTimeout)
//                 break; // You lose, good day sir
//         } else
//             break; // End-of-data on stream
//     }

// bail: // Sorry, dreaded goto. Because nested loops.
//     if (debug)
//         debug->println('\'');
//     if (ipd)
//         setTimeouts(save);
//     return found;
// }

// // Read from ESP8266 stream into RAM, up to a given size. Max number of
// // chars read is 1 less than this, so NUL can be appended on string.
// int Wifi::readLine(char *buf, int bufSiz) {
//     if (debug && writing) {
//         debug->print(F("<--- '"));
//         writing = false;
//     }
//     int bytesRead = stream->readBytesUntil('\r', buf, bufSiz - 1);
//     buf[bytesRead] = 0;
//     if (debug) {
//         debug->print(buf);
//         debug->println('\'');
//     }
//     while (stream->read() != '\n')
//         ; // Discard thru newline
//     return bytesRead;
// }

// // ESP8266 is reset by momentarily connecting RST to GND. Level shifting is
// // not necessary provided you don't accidentally set the pin to HIGH output.
// // It's generally safe-ish as the default Arduino pin state is INPUT (w/no
// // pullup) -- setting to LOW provides an open-drain for reset.
// // Returns true if expected boot message is received (or if RST is unused),
// // false otherwise.
// boolean Wifi::hardReset() {
//     if (reset_pin < 0)
//         return true;
//     digitalWrite(reset_pin, LOW);
//     pinMode(reset_pin, OUTPUT); // Open drain; reset -> GND
//     delay(10);                  // Hold a moment
//     pinMode(reset_pin, INPUT);  // Back to high-impedance pin state
//     return find(bootMarker);    // Purge boot message from stream
// }

// // Soft reset. Returns true if expected boot message received, else false.
// boolean Wifi::softReset() {
//     boolean found = false;
//     uint32_t save = receiveTimeout; // Temporarily override recveive timeout,
//     receiveTimeout = resetTimeout;  // reset time is longer than normal I/O.
//     println(F("AT+RST"));           // Issue soft-reset command
//     if (find(bootMarker)) {         // Wait for boot message
//         println(F("ATE0"));         // Turn off echo
//         found = find();             // OK?
//     }
//     receiveTimeout = save; // Restore normal receive timeout
//     return found;
// }

// // For interactive debugging...shuttle data between Serial Console <-> WiFi
// void Wifi::debugLoop() {
//     if (!debug)
//         for (;;)
//             ; // If no debug connection, nothing to do.

//     debug->println(F("\n========================"));
//     for (;;) {
//         if (debug->available())
//             stream->write(debug->read());
//         if (stream->available())
//             debug->write(stream->read());
//     }
// }

// // Connect to WiFi access point. SSID and password are flash-resident
// // strings. May take several seconds to execute, this is normal.
// // Returns true on successful connection, false otherwise.
// boolean Wifi::connectToAP(Fstr *ssid, Fstr *pass) {
//     char buf[256];

//     println(F("AT+CWMODE=1")); // WiFi mode = Sta
//     readLine(buf, sizeof(buf));
//     if (!(strstr_P(buf, (Pchr *)F("OK")) ||
//           strstr_P(buf, (Pchr *)F("no change"))))
//         return false;

//     print(F("AT+CWJAP=\"")); // Join access point
//     print(ssid);
//     print(F("\",\""));
//     print(pass);
//     println('\"');
//     uint32_t save = receiveTimeout;  // Temporarily override recv timeout,
//     receiveTimeout = connectTimeout; // connection time is much longer!
//     boolean found = find();          // Await 'OK' message
//     receiveTimeout = save;           // Restore normal receive timeout
//     if (found) {
//         println(F("AT+CIPMUX=0")); // Set single-client mode
//         found = find();            // Await 'OK'
//     }

//     return found;
// }

// void Wifi::closeAP() {
//     println(F("AT+CWQAP")); // Quit access point
//     find();                 // Purge 'OK'
// }

// // Open TCP connection. Hostname is flash-resident string.
// // Returns true on successful connection, else false.
// boolean Wifi::connectTCP(Fstr *h, int port) {

//     print(F("AT+CIPSTART=\"TCP\",\""));
//     print(h);
//     print(F("\","));
//     println(port);

//     if (find(F("Linked"))) {
//         host = h;
//         return true;
//     }
//     return false;
// }

// void Wifi::closeTCP() {
//     println(F("AT+CIPCLOSE"));
//     find(F("Unlink\r\n"));
// }

// // Requests page from currently-open TCP connection. URL is
// // flash-resident string. Returns true if request issued successfully,
// // else false. Calling function should then handle data returned, may
// // need to parse IPD delimiters (see notes in find() function.
// // (Can call find(F("Unlink"), true) to dump to debug.)
// boolean Wifi::requestURL(Fstr *url) {
//     print(F("AT+CIPSEND="));
//     println(25 + strlen_P((Pchr *)url) + strlen_P((Pchr *)host));
//     if (find(F("> "))) {  // Wait for prompt
//         print(F("GET ")); // 4
//         print(url);
//         print(F(" HTTP/1.1\r\nHost: ")); // 17
//         print(host);
//         print(F("\r\n\r\n")); // 4
//         return (find());      // Gets 'SEND OK' line
//     }
//     return false;
// }

// // Requests page from currently-open TCP connection. URL is
// // character string in SRAM. Returns true if request issued successfully,
// // else false. Calling function should then handle data returned, may
// // need to parse IPD delimiters (see notes in find() function.
// // (Can call find(F("Unlink"), true) to dump to debug.)
// boolean Wifi::requestURL(char *url) {
//     print(F("AT+CIPSEND="));
//     println(25 + strlen(url) + strlen_P((Pchr *)host));
//     if (find(F("> "))) {  // Wait for prompt
//         print(F("GET ")); // 4
//         print(url);
//         print(F(" HTTP/1.1\r\nHost: ")); // 17
//         print(host);
//         print(F("\r\n\r\n")); // 4
//         return (find());      // Gets 'SEND OK' line
//     }
//     return false;
// }
