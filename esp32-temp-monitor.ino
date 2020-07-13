#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Syslog.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include "webpages.h"

#define FIRMWARE_VERSION "v0.0.1"

const String default_hostname = "tempmon.home.narco.tk";
const String default_appname = "tempmon";
const String default_ssid = "xxx";
const String default_wifipassword = "xxx";
const int default_relaypin = 26;
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const String default_httpapitoken = "xyz";
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
  String httpapitoken;       // api token used to authenticate against the device
  int webserverporthttp;     // http port number for web admin
  int webserverporthttps;    // https port number for the web admin
  bool syslogenable;         // enable syslog
  String syslogserver;       // hostname or ip of the syslog server
  int syslogport;            // sylog port number
};

// function defaults
String listFiles(bool ishtml = false);

// variables
const char *filename = "/config.txt"; // filename where configuration is stored
Config config;                        // configuration
bool shouldReboot = false;            // schedule a reboot
AsyncWebServer *server;               // initialise webserver

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

  Serial.println("Mounting SPIFFS ...");
  if (!SPIFFS.begin(true)) {
    Serial.println("ERROR: Cannot mount SPIFFS, Rebooting");
    rebootESP("ERROR: Cannot mount SPIFFS, Rebooting");
  }

  Serial.print("SPIFFS Free: "); Serial.println(humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes())));
  Serial.print("SPIFFS Used: "); Serial.println(humanReadableSize(SPIFFS.usedBytes()));
  Serial.print("SPIFFS Total: "); Serial.println(humanReadableSize(SPIFFS.totalBytes()));

  Serial.println(listFiles());

  Serial.println("Loading Configuration ...");

  config.hostname = default_hostname;
  config.appname = default_appname;
  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.relaypin = default_relaypin;
  config.httpuser = default_httpuser;
  config.httppassword = default_httppassword;
  config.httpapitoken = default_httpapitoken;
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
  // reboot if we've told it to reboot
  if (shouldReboot) {
    rebootESP("Web Admin Initiated Reboot");
  }
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

void rebootESP(String message) {
  Serial.print("Rebooting ESP32: "); Serial.println(message);
  if (config.syslogenable) { syslog.logf("Rebooting ESP32:%s", message); }
  // wait 10 seconds to allow syslog to be sent
  delay(10000);
  ESP.restart();
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  Serial.println("Listing files stored on SPIFFS");
  File root = SPIFFS.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
      //"<td><a href='/file?name=" + String(foundfile.name()) + "&action=download'>Download</a></td><td><a href='/file?name=" + String(foundfile.name()) + "&action=delete'>Delete</a></td>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    } else {
      returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
