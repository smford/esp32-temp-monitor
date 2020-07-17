#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Syslog.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "webpages.h"
#include "defaults.h"

#define FIRMWARE_VERSION "v0.0.3"

// configuration structure
struct Config {
  String hostname;           // hostname of device
  String appname;            // application name
  String ssid;               // wifi ssid
  String wifipassword;       // wifi password
  String httpuser;           // username to access web admin
  String httppassword;       // password to access web admin
  String httpapitoken;       // api token used to authenticate against the device
  int webserverporthttp;     // http port number for web admin
  int webserverporthttps;    // https port number for the web admin
  bool syslogenable;         // enable syslog
  String syslogserver;       // hostname or ip of the syslog server
  int syslogport;            // sylog port number
  bool telegrafenable;       // turns on or off shipping of metrics to telegraf
  String telegrafserver;     // hostname or ip of telegraf server
  int telegrafserverport;    // port number for telegraf
  int telegrafshiptime;      // how often in seconds we should ship metrics to telegraf
  int tempchecktime;         // how often in seconds to check temperature
};

const char *validConfSettings[] = {"hostname", "appname", "ssid", "wifipassword", "httpuser", "httppassword", "httpapitoken", "webserverporthttp", "webserverporthttps", "syslogenable", "syslogserver", "syslogport", "telegrafenable", "telegrafserver", "telegrafserverport", "telegrafshiptime", "tempchecktime"};

// function defaults
String listFiles(bool ishtml = false);

// variables
const char *filename = "/config.txt";   // filename where configuration is stored
Config config;                          // configuration
bool shouldReboot = false;              // schedule a reboot
AsyncWebServer *server;                 // initialise webserver
unsigned long telegrafLastRunTime = 0;  // keeps track of the time when the last metrics were shipped for influxdb
unsigned long tempCheckLastRunTime = 0; // keeps track of the time when the last temp check was run

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

// LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

void setup() {
  Serial.begin(115200);

  setupLCD();

  Serial.println("Booting ...");
  printLCD("Booting ...", "");

  Serial.print("Firmware: "); Serial.println(FIRMWARE_VERSION);

  Serial.println("Mounting SPIFFS ...");
  if (!SPIFFS.begin(true)) {
    Serial.println("ERROR: Cannot mount SPIFFS, Rebooting");
    rebootESP("ERROR: Cannot mount SPIFFS, Rebooting");
  }

  Serial.print("SPIFFS Free: "); Serial.println(humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes())));
  Serial.print("SPIFFS Used: "); Serial.println(humanReadableSize(SPIFFS.usedBytes()));
  Serial.print("SPIFFS Total: "); Serial.println(humanReadableSize(SPIFFS.totalBytes()));

  Serial.println(listFiles());

  loadConfiguration(filename, config);
  printConfig();

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
  if (config.syslogenable) {
    Serial.println("Syslog Enabled: true");
    Serial.print(" Syslog Server: "); Serial.println(config.syslogserver);
    Serial.print("   Syslog Port: "); Serial.println(config.syslogport);
  }
  if (config.telegrafenable) {
    Serial.println("  Telegraf Enabled: true");
    Serial.print("   Telegraf Server: "); Serial.println(config.telegrafserver);
    Serial.print("     Telegraf Port: "); Serial.println(config.telegrafserverport);
    Serial.print("Telegraf Ship Time: "); Serial.println(config.telegrafshiptime);
  }

  Serial.println();

  if (config.syslogenable) {
    Serial.println("Configuring Syslog ...");
    // configure syslog server using loaded configuration
    syslog.server(config.syslogserver.c_str(), config.syslogport);
    syslog.deviceHostname(config.hostname.c_str());
    syslog.appName(config.appname.c_str());
    syslog.defaultPriority(LOG_INFO);
  }

  syslogSend("Configuring Webserver ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  syslogSend("Starting Webserver ...");
  server->begin();

  Wire.begin();
  Serial.println(i2cScanner());

  printLCD("Ready", "Getting Temp");

}

void loop() {
  // reboot if we've told it to reboot
  if (shouldReboot) {
    rebootESP("Web Admin Initiated Reboot");
  }

  // do nothing and return if trying to ship metrics too fast
  if ((millis() - telegrafLastRunTime) > (config.telegrafshiptime * 1000)) {
    shipMetric("cputemp", getESPTemp());
    shipMetric("wifisignal", String(WiFi.RSSI()));
    shipMetric("telegrafdelay", String(millis() - telegrafLastRunTime));
    telegrafLastRunTime = millis();
  }

  if ((millis() - tempCheckLastRunTime) > (config.tempchecktime * 1000)) {
    printLCD("CPUTemp", getESPTemp() + " C");
    //printLCD(getESPTemp() + " C", "");
    tempCheckLastRunTime = millis();
  }

}


String i2cScanner() {
  byte error, address;
  String returnText = "[";
  syslogSend("Scanning I2C");
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
  return String((temprature_sens_read() - 32) / 1.8);
}

void rebootESP(String message) {
  syslogSend("Rebooting ESP32: " + message);
  // wait 10 seconds to allow syslog to be sent
  delay(10000);
  ESP.restart();
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  syslogSend("Listing files stored on SPIFFS");
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

void shipMetric(String metric, String value) {
  String line = config.appname + "-" + metric + " value=" + value;
  //Serial.print("Shipping: "); Serial.println(line);
  udpClient.beginPacket(config.telegrafserver.c_str(), config.telegrafserverport);
  udpClient.print(line);
  udpClient.endPacket();
}

void syslogSend(String message) {
  Serial.println(message);
  if (config.syslogenable) {
    syslog.log(message);
  }
}

void setupLCD() {
  lcd.begin(16, 2);
  lcd.home();
}

void printLCD(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor (0, 1);
  lcd.print(line2);
}

String getConfig() {
  StaticJsonDocument<2200> configDoc;
  configDoc["Hostname"] = config.hostname;
  configDoc["AppName"] = config.appname;
  configDoc["SSID"] = config.ssid;
  configDoc["WifiPassword"] = config.wifipassword;
  configDoc["HTTPUser"] = config.httpuser;
  configDoc["HTTPPassword"] = config.httppassword;
  configDoc["HTTPAPIToken"] = config.httpapitoken;
  configDoc["WebServerPortHTTP"] = config.webserverporthttp;
  configDoc["WebServerPortHTTPS"] = config.webserverporthttps;
  configDoc["SyslogEnable"] = config.syslogenable;
  configDoc["SyslogServer"] = config.syslogserver;
  configDoc["SyslogPort"] = config.syslogport;
  configDoc["TelegrafEnable"] = config.telegrafenable;
  configDoc["TelegrafServer"] = config.telegrafserver;
  configDoc["TelegrafServerPort"] = config.telegrafserverport;
  configDoc["TelegrafShipTime"] = config.telegrafshiptime;
  String fullConfig = "";
  serializeJson(configDoc, fullConfig);
  return fullConfig;
}

String shortStatus() {
  StaticJsonDocument<200> shortStatusDoc;
  shortStatusDoc["Hostname"] = config.hostname;
  shortStatusDoc["FreeSPIFFS"] = humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
  shortStatusDoc["UsedSPIFFS"] = humanReadableSize(SPIFFS.usedBytes());
  shortStatusDoc["TotalSPIFFS"] = humanReadableSize(SPIFFS.totalBytes());
  shortStatusDoc["CPUTemp"] = getESPTemp();
  String shortStatus = "";
  serializeJson(shortStatusDoc, shortStatus);
  return shortStatus;
}

bool checkSetting(const char *Name) {
  int arraySize = sizeof(validConfSettings) / sizeof(validConfSettings[0]);
  for (int i = 0; i < arraySize; i++) {
    //Serial.printf("validConfSettings[%d]=%s\n", i, validConfSettings[i]);
    if (strcmp(Name, validConfSettings[i]) == 0) {
      syslogSend("Valid Configuration setting found: " + String(Name));
      return true;
    }
  }
  return false;
}
