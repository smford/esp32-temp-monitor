#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Syslog.h>
#include <WiFiUdp.h>
#include "webpages.h"

#define FIRMWARE_VERSION "v0.0.1"

const String default_hostname = "tempmon.home.narco.tk";
const String default_appname = "tempmon";
const String default_ssid = "xxx";
const String default_wifipassword = "xxx";
const int default_relaypin = 26;
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const int default_webserverporthttp = 80;
const int default_webserverporthttps = 443;
const bool default_syslogenable = true;
const String default_syslogserver = "192.168.10.21";
const int default_syslogport = 514;

// configuration structure
struct Config {
  String hostname;           // hostname of device
  String appname;            // application name
  String ssid;               // wifi ssid
  String wifipassword;       // wifi password
  int relaypin;              // relay pin number
  String httpuser;           // username to access web admin
  String httppassword;       // password to access web admin
  int webserverporthttp;     // http port number for web admin
  int webserverporthttps;    // https port number for the web admin
  bool syslogenable;         // enable syslog
  String syslogserver;       // hostname or ip of the syslog server
  int syslogport;            // sylog port number
};

// variables
Config config;                       // configuration
bool shouldReboot = false;           // schedule a reboot
AsyncWebServer *server;              // initialise webserver

// internal ESP32 temp sensor
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

// setup udp connection
WiFiUDP udpClient;

// Syslog
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

void setup() {
  Serial.begin(115200);

  Serial.print("Firmware: "); Serial.println(FIRMWARE_VERSION);

  Serial.println("Booting ...");

  Serial.println("Loading Configuration ...");

  config.hostname = default_hostname;
  config.appname = default_appname;
  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.relaypin = default_relaypin;
  config.httpuser = default_httpuser;
  config.httppassword = default_httppassword;
  config.webserverporthttp = default_webserverporthttp;
  config.webserverporthttps = default_webserverporthttps;
  config.syslogenable = default_syslogenable;
  config.syslogserver = default_syslogserver;
  config.syslogport = default_syslogport;

  Serial.print("\nConnecting to Wifi: ");
  WiFi.begin(config.ssid.c_str(), config.wifipassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n\nNetwork Configuration:");
  Serial.println("----------------------");
  Serial.print("         SSID: "); Serial.println(WiFi.SSID());
  Serial.print("  Wifi Status: "); Serial.println(WiFi.status());
  Serial.print("Wifi Strength: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  Serial.print("          MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("           IP: "); Serial.println(WiFi.localIP());
  Serial.print("       Subnet: "); Serial.println(WiFi.subnetMask());
  Serial.print("      Gateway: "); Serial.println(WiFi.gatewayIP());
  Serial.print("        DNS 1: "); Serial.println(WiFi.dnsIP(0));
  Serial.print("        DNS 2: "); Serial.println(WiFi.dnsIP(1));
  Serial.print("        DNS 3: "); Serial.println(WiFi.dnsIP(2));
  Serial.println();

  if (config.syslogenable) {
    Serial.println("Configuring Syslog ...");
    // configure syslog server using loaded configuration
    syslog.server(config.syslogserver.c_str(), config.syslogport);
    syslog.deviceHostname(config.hostname.c_str());
    syslog.appName(config.appname.c_str());
    syslog.defaultPriority(LOG_INFO);
  }

  // configure web server
  Serial.println("Configuring Webserver ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  Serial.println("Starting Webserver ...");
  // startup web server
  server->begin();

  Wire.begin();
  Serial.println(i2cScanner());

}

void loop() {
  // put your main code here, to run repeatedly:

}


String i2cScanner() {
  byte error, address;
  String returnText = "[";
  Serial.println("Scanning I2C");
  if (config.syslogenable) {
    syslog.log("Scanning I2C");
  }
  for (address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      if (returnText.length() > 1) returnText += ",";
      returnText += "{";
      returnText += "\"int\":" + String(address);
      if (address < 16) {
        returnText += ",\"hex\":\"0x00\"";
      } else {
        returnText += ",\"hex\":\"0x" + String(address, HEX) + "\"";
      }
      returnText += ",\"error\":\"none\"";
      returnText += "}";
    } else if (error == 4) {
      if (returnText.length() > 1) returnText += ",";
      returnText += "{";
      returnText += "\"int\":" + String(address);
      if (address < 16) {
        returnText += ",\"hex\":\"0x00\"";
      } else {
        returnText += ",\"hex\":\"0x" + String(address, HEX) + "\"";
      }
      returnText += ",\"error\":\"unknown error\"";
      returnText += "}";
    }
  }
  returnText += "]";
  return returnText;
}

String getESPTemp() {
  return String((temprature_sens_read() - 32) / 1.8) + "C";
}
