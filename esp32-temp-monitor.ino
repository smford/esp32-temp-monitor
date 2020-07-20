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
#include <ezTime.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "webpages.h"
#include "defaults.h"

#define FIRMWARE_VERSION "v0.1.1"
#define LCDWIDTH 16
#define LCDROWS 2

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
  String ntpserver;          // hostname or ip of the ntpserver
  String ntptimezone;        // ntp time zone to use, use the TZ database name from https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
  int ntpsynctime;           // how frequently to schedule the regular syncing of ntp time
  int ntpwaitsynctime;       // upon boot, wait these many seconds to get ntp time from server
  bool pushoverenable;       // enable pushover
  String pushoverapptoken;   // pushover app token
  String pushoveruserkey;    // pushover user key
  String pushoverdevice;     // pushover device name
};

const int oneWireBus = 4;
const char *validConfSettings[] = {"hostname", "appname",
                                   "ssid", "wifipassword",
                                   "httpuser", "httppassword", "httpapitoken",
                                   "webserverporthttp", "webserverporthttps",
                                   "syslogenable", "syslogserver", "syslogport",
                                   "telegrafenable", "telegrafserver", "telegrafserverport", "telegrafshiptime",
                                   "tempchecktime",
                                   "ntpserver", "ntptimezone", "ntpsynctime", "ntpwaitsynctime",
                                   "pushoverenable", "pushoverapptoken", "pushoveruserkey", "pushoverdevice"};

// function defaults
String listFiles(bool ishtml = false);
void printLCD(String line1 = "", String line2 = "", String line3 = "", String line4 = "");

// variables
const char *filename = "/config.txt";   // filename where configuration is stored
Config config;                          // configuration
bool shouldReboot = false;              // schedule a reboot
AsyncWebServer *server;                 // initialise webserver
unsigned long telegrafLastRunTime = 0;  // keeps track of the time when the last metrics were shipped for influxdb
unsigned long tempCheckLastRunTime = 0; // keeps track of the time when the last temp check was run
String bootTime;                        // used to record when the device was booted

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

// NTP
Timezone myTZ;
ezDebugLevel_t NTPDEBUG = ERROR; // NONE, ERROR, INFO, DEBUG

// LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
String lcdDisplay[LCDROWS];

// OneWire
OneWire oneWire(oneWireBus);

// DS18B20
DallasTemperature sensors(&oneWire);
//DallasTemperature(sensors(&oneWire), uint8_t);
//DallasTemperature(sensors(&oneWire*, uint8_t)
//DallasTemperature(oneWire*, uint8_t)
DeviceAddress insideThermometer, outsideThermometer;

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
  Serial.print("   Temp Check Time: "); Serial.println(config.tempchecktime);
  Serial.print("        NTP Server: "); Serial.println(config.ntpserver);
  Serial.print("      NTP Timezone: "); Serial.println(config.ntptimezone);
  Serial.print("     NTP Sync Time: "); Serial.println(config.ntpsynctime);
  Serial.print("NTP Wait Sync Time: "); Serial.println(config.ntpwaitsynctime);
  if (config.pushoverenable) {
    Serial.println("  Pushover Enabled: true");
    Serial.print("Pushover APP Token: "); Serial.println(config.pushoverapptoken);
    Serial.print(" Pushover User Key: "); Serial.println(config.pushoveruserkey);
    Serial.print("   Pushover Device: "); Serial.println(config.pushoverdevice);
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

  waitForSync(config.ntpwaitsynctime);
  setInterval(config.ntpsynctime);
  setServer(config.ntpserver);
  setDebug(NTPDEBUG);
  myTZ.setLocation(config.ntptimezone);
  Serial.print(config.ntptimezone + ": "); Serial.println(printTime());

  syslogSend("Configuring Webserver ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  syslogSend("Starting Webserver ...");
  server->begin();

  // need to do a scan here, else first scan will fail.  Bug in scanNetworks
  WiFi.scanNetworks(true, true);

  bootTime = printTime();
  syslogSend("Booted at: " + bootTime);

  printLCD("Ready", "Getting Temp");

  sensors.begin();
  Serial.print("Found "); Serial.print(sensors.getDeviceCount(), DEC); Serial.println(" devices.");
  Serial.println("Probe1 Temp:" + String(sensors.getTempCByIndex(0)));
  // search for devices on the bus and assign based on an index.
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
  //if (!sensors.getAddress(outsideThermometer, 1)) Serial.println("Unable to find address for Device 1");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  Serial.print("Device 0 Alarms: ");
  printAlarms(insideThermometer);
  Serial.println();

  /*
    Serial.print("Device 1 Address: ");
    printAddress(outsideThermometer);
    Serial.println();

    Serial.print("Device 1 Alarms: ");
    printAlarms(outsideThermometer);
    Serial.println();
  */

  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  pinMode(oneWireBus, OUTPUT);
  digitalWrite(oneWireBus, HIGH);
  delay(750);

  Serial.println(sensors.getTempCByIndex(0));

}

void loop() {
  // display ntp sync events on serial
  events();

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
    //sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
    //pinMode(oneWireBus, OUTPUT);
    //digitalWrite(oneWireBus, HIGH);
    //delay(750);
    printLCD("  CPU:" + getESPTemp() + " C", "Probe:" + String(sensors.getTempCByIndex(0)) + " C");
    //printLCD(getESPTemp() + " C", "");
    Serial.println(sensors.getTempCByIndex(0));
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
  lcd.begin(LCDWIDTH, LCDROWS);
  lcd.home();
}

void printLCD(String line1, String line2, String line3, String line4) {
  lcd.clear();
  if (LCDROWS >= 1) {
    lcdDisplay[0] = line1;
    lcd.setCursor(0, 0);
    lcd.print(lcdDisplay[0]);
  }
  if (LCDROWS >= 2) {
    lcdDisplay[1] = line2;
    lcd.setCursor(0, 1);
    lcd.print(lcdDisplay[1]);
  }
  if (LCDROWS >= 3) {
    lcdDisplay[2] = line3;
    lcd.setCursor(0, 2);
    lcd.print(lcdDisplay[2]);
  }
  if (LCDROWS == 4) {
    lcdDisplay[3] = line4;
    lcd.setCursor(0, 3);
    lcd.print(lcdDisplay[3]);
  }
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
  configDoc["TempCheckTime"] = config.tempchecktime;
  configDoc["NTPServer"] = config.ntpserver;
  configDoc["NTPTimezone"] = config.ntptimezone;
  configDoc["NTPSyncTime"] = config.ntpsynctime;
  configDoc["NTPWaitSyncTime"] = config.ntpwaitsynctime;
  configDoc["PushoverEnable"] = config.pushoverenable;
  configDoc["PushoverAppToken"] = config.pushoverapptoken;
  configDoc["PushoverUserKey"] = config.pushoveruserkey;
  configDoc["PushoverDevice"] = config.pushoverdevice;
  String fullConfig = "";
  serializeJson(configDoc, fullConfig);
  return fullConfig;
}

String shortStatus() {
  StaticJsonDocument<400> shortStatusDoc;
  shortStatusDoc["Hostname"] = config.hostname;
  shortStatusDoc["FreeSPIFFS"] = humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
  shortStatusDoc["UsedSPIFFS"] = humanReadableSize(SPIFFS.usedBytes());
  shortStatusDoc["TotalSPIFFS"] = humanReadableSize(SPIFFS.totalBytes());
  shortStatusDoc["CPUTemp"] = getESPTemp();
  shortStatusDoc["Time"] = printTime();
  String lcdTable;
    lcdTable += "<table>";
    for (int i = 0; i < 4; i++) {
      lcdTable += "<tr><td>Line " + String(i + 1) + ":</td><td>" + lcdDisplay[i] + "</td></tr>";
    }
  lcdTable += "</table>";
  shortStatusDoc["LCD"] = lcdTable;
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

String printTime() {
  return myTZ.dateTime();
}

String getTimeStatus() {
  String myTimeStatus;
  switch (timeStatus()) {
    case 0:
      myTimeStatus = "Time Not Set";
      break;
    case 1:
      myTimeStatus = "Time Needs Syncing";
      break;
    case 2:
      myTimeStatus = "Time Set";
      break;
    default:
      myTimeStatus = "Unknown";
  }
  return myTimeStatus;
}
