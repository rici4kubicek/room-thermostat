#if defined(ARDUINO_ARCH_ESP8266)
#  include <ESP8266WiFi.h>
#  include <ESP8266WebServer.h>
#  include <ESP8266NetBIOS.h>
#else
#  include <WiFi.h>
#  include <WebServer.h>
#  include <NetBIOS.h>
#endif
#include <ArduinoOTA.h>
#include <WiFiConfig.h>
#include <sysvars.hpp>

// Plati pro desticku Witty s modulem ESP-12E
#define PIN_FORCE_CONFIG 4

//#define DEBUG_OUT(a) {}
#define DEBUG_OUT(a) Serial.print(a)

char WiFiDeviceName[32]; // misto pro jmeno zarizeni (dodane do DNS, DHCP NBNS apod...)
#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer wwwserver(80); // webovy server
#else
WebServer wwwserver(80); // webovy server
#endif

static char MAINPAGE[] PROGMEM = R"=====(<!DOCTYPE HTML>
<html>
    Hello world from ESP8266
    <p>
</html>
)=====";

static void handleRoot(void)
{

    wwwserver.send_P(200, "text/html", MAINPAGE);
}

void ICACHE_FLASH_ATTR wcb(wificonfigstate_t state)
{

    switch (state) {
    case WCS_CONNECTSTART:
        DEBUG_OUT(F("Starting connect...\r\n"));
        break;

    case WCS_CONNECTING:
        break;

    case WCS_CONNECTED:
        DEBUG_OUT(F("Connected.\r\n"));
        break;

    case WCS_CONFIGSTART:
        DEBUG_OUT(F("Starting config...\r\n"));
        break;

    case WCS_CONFIGWAIT:
        break;

    case WCS_CONFIGTIMEOUT:
        DEBUG_OUT(F("Config timeout...\r\n"));
        break;
    }
}

void ICACHE_FLASH_ATTR saveDevname(const char *param)
{
    String p = param;

    svSetV(F("devname"), p);
}

void ICACHE_FLASH_ATTR setup()
{
    WiFiConfig wifi; // konfigurace ESP modulu
    WiFiConfigUsrParameter devname("devname", "Jméno zařízení", (const char *)WiFiDeviceName, 32, saveDevname);

    LittleFS.begin();

    pinMode(PIN_FORCE_CONFIG, INPUT_PULLUP); // pin, co slouzi jako vstup tlacitka
    int fc = digitalRead(PIN_FORCE_CONFIG); // pozadavek na vynucene vyvolani konfigurace
    Serial.begin(115200);
    String dn = svGetV<String>(F("devname"));
    strcpy(WiFiDeviceName, dn.c_str());
    wifi.addParameter(&devname);

    if (WCR_OK != wifi.begin(fc, wcb)) // startujeme pripojeni
        ESP.restart();

    wwwserver.on("/", handleRoot);
    wwwserver.begin(); // startujeme webovy server

    if (strlen(WiFiDeviceName) > 0) {
        NBNS.begin(WiFiDeviceName);
        ArduinoOTA.setHostname(WiFiDeviceName);
    }

    ArduinoOTA.onStart([]() {
        DEBUG_OUT(F("Start\r\n"));
    });
    ArduinoOTA.onEnd([]() {
        DEBUG_OUT(F("End\r\n"));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        DEBUG_OUT(F("Error["));
        DEBUG_OUT(error);
        DEBUG_OUT(F("]: "));
        switch (error) {
        case OTA_AUTH_ERROR:
            DEBUG_OUT(F("Auth Failed\r\n"));
            break;

        case OTA_BEGIN_ERROR:
            DEBUG_OUT(F("Begin Failed\r\n"));
            break;

        case OTA_CONNECT_ERROR:
            DEBUG_OUT(F("Connect Failed\r\n"));
            break;

        case OTA_RECEIVE_ERROR:
            DEBUG_OUT(F("Receive Failed\r\n"));
            break;

        case OTA_END_ERROR:
            DEBUG_OUT(F("End Failed\r\n"));
            break;

        default:
            DEBUG_OUT(F("\r\n"));
        }
    });
    ArduinoOTA.begin();

}

void loop()
{
    wwwserver.handleClient(); // osetrujeme praci serveru
    ArduinoOTA.handle();
}