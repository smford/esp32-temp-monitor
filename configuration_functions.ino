// based upon https://arduinojson.org/v6/example/config/
void loadConfiguration(const char *filename, Config &config) {
  // not using syslogSend here because it can be called before syslog has been configured
  Serial.println("Loading configuration from " + String(filename));

  // flag used to detect if a default value is loaded, if default value loaded initiate a save after load
  bool initiatesave = false;

  if (!SPIFFS.exists(filename)) {
    Serial.println(String(filename) + " not found");
    initiatesave = true;
  } else {
    Serial.println(String(filename) + " found");
  }

  // Open file for reading
  Serial.println("Opening " + String(filename));
  File file = SPIFFS.open(filename);

  if (!file) {
    Serial.println("ERROR: Failed to open file" + String(filename));
    return;
  }

  StaticJsonDocument<2000> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to process configuration file, will load default configuration"));
    Serial.println("===ERROR===");
    Serial.println(error.c_str());
    Serial.println("===========");
  }

  // Copy values from the JsonDocument to the Config
  config.hostname = doc["hostname"].as<String>();
  if (config.hostname == "null") {
    initiatesave = true;
    config.hostname = default_hostname;
  }

  config.appname = doc["appname"].as<String>();
  if (config.appname == "null") {
    initiatesave = true;
    config.appname = default_appname;
  }

  config.ssid = doc["ssid"].as<String>();
  if (config.ssid == "null") {
    initiatesave = true;
    config.ssid = default_ssid;
  }

  config.wifipassword = doc["wifipassword"].as<String>();
  if (config.wifipassword == "null") {
    initiatesave = true;
    config.wifipassword = default_wifipassword;
  }

  config.httpuser = doc["httpuser"].as<String>();
  if (config.httpuser == "null") {
    initiatesave = true;
    config.httpuser = default_httpuser;
  }

  config.httppassword = doc["httppassword"].as<String>();
  if (config.httppassword == "null") {
    initiatesave = true;
    config.httppassword = default_httppassword;
  }

  config.httpapitoken = doc["httpapitoken"].as<String>();
  if (config.httpapitoken == "null") {
    initiatesave = true;
    config.httpapitoken = default_httpapitoken;
  }

  config.webserverporthttp = doc["webserverporthttp"];
  if (config.webserverporthttp == 0) {
    initiatesave = true;
    config.webserverporthttp = default_webserverporthttp;
  }

  config.webserverporthttps = doc["webserverporthttps"];
  if (config.webserverporthttps == 0) {
    initiatesave = true;
    config.webserverporthttps = default_webserverporthttps;
  }
  
  if (doc.containsKey("syslogenable")) {
    config.syslogenable = doc["syslogenable"].as<bool>();
  } else {
    initiatesave = true;
    config.syslogenable = default_syslogenable;
  }
  
  config.syslogserver = doc["syslogserver"].as<String>();
  if (config.syslogserver == "null") {
    initiatesave = true;
    config.syslogserver = default_syslogserver;
  }

  config.syslogport = doc["syslogport"];
  if (config.syslogport == 0) {
    initiatesave = true;
    config.syslogport = default_syslogport;
  }

  if (doc.containsKey("telegrafenable")) {
    config.telegrafenable = doc["telegrafenable"].as<bool>();
  } else {
    initiatesave = true;
    config.telegrafenable = default_telegrafenable;
  }

  config.telegrafserver = doc["telegrafserver"].as<String>();
  if (config.telegrafserver == "null") {
    initiatesave = true;
    config.telegrafserver = default_telegrafserver;
  }

  config.telegrafserverport = doc["telegrafserverport"];
  if (config.telegrafserverport == 0) {
    initiatesave = true;
    config.telegrafserverport = default_telegrafserverport;
  }

  config.telegrafshiptime = doc["telegrafshiptime"];
  if (config.telegrafshiptime == 0) {
    initiatesave = true;
    config.telegrafshiptime = default_telegrafshiptime;
  }

  config.tempchecktime = doc["tempchecktime"];
  if (config.tempchecktime == 0) {
    initiatesave = true;
    config.tempchecktime = default_tempchecktime;
  }

  config.ntpserver = doc["ntpserver"].as<String>();
  if (config.ntpserver == "null") {
    initiatesave = true;
    config.ntpserver = default_ntpserver;
  }

  config.ntptimezone = doc["ntptimezone"].as<String>();
  if (config.ntptimezone == "null") {
    initiatesave = true;
    config.ntptimezone = default_ntptimezone;
  }

  config.ntpsynctime = doc["ntpsynctime"];
  if (config.ntpsynctime == 0) {
    initiatesave = true;
    config.ntpsynctime = default_ntpsynctime;
  }

  config.ntpwaitsynctime = doc["ntpwaitsynctime"];
  if (config.ntpwaitsynctime == 0) {
    initiatesave = true;
    config.ntpwaitsynctime = default_ntpwaitsynctime;
  }

  if (doc.containsKey("pushoverenable")) {
    config.pushoverenable = doc["pushoverenable"].as<bool>();
  } else {
    initiatesave = true;
    config.pushoverenable = default_pushoverenable;
  }

  config.pushoverapptoken = doc["pushoverapptoken"].as<String>();
  if (config.pushoverapptoken == "null") {
    initiatesave = true;
    config.pushoverapptoken = default_pushoverapptoken;
  }

  config.pushoveruserkey = doc["pushoveruserkey"].as<String>();
  if (config.pushoveruserkey == "null") {
    initiatesave = true;
    config.pushoveruserkey = default_pushoveruserkey;
  }

  config.pushoverdevice = doc["pushoverdevice"].as<String>();
  if (config.pushoverdevice == "null") {
    initiatesave = true;
    config.pushoverdevice = default_pushoverdevice;
  }

  if (doc.containsKey("metric")) {
    config.metric = doc["metric"].as<bool>();
  } else {
    initiatesave = true;
    config.metric = default_metric;
  }

  file.close();

  if (initiatesave) {
    Serial.println("Default configuration values loaded, saving configuration to " + String(filename));
    saveConfiguration(filename, config);
  }
}

void saveConfiguration(const char *filename, const Config &config) {
  // not using syslogSend here because it can be called before syslog has been configured


  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);

  // Open file for writing
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  StaticJsonDocument<2000> doc;

  // Set the values in the document
  doc["hostname"] = config.hostname;
  doc["appname"] = config.appname;
  doc["ssid"] = config.ssid;
  doc["wifipassword"] = config.wifipassword;
  doc["httpuser"] = config.httpuser;
  doc["httppassword"] = config.httppassword;
  doc["httpapitoken"] = config.httpapitoken;
  doc["webserverporthttp"] = config.webserverporthttp;
  doc["webserverporthttps"] = config.webserverporthttps;
  doc["syslogenable"] = config.syslogenable;
  doc["syslogserver"] = config.syslogserver;
  doc["syslogport"] = config.syslogport;
  doc["telegrafenable"] = config.telegrafenable;
  doc["telegrafserver"] = config.telegrafserver;
  doc["telegrafserverport"] = config.telegrafserverport;
  doc["telegrafshiptime"] = config.telegrafshiptime;
  doc["tempchecktime"] = config.tempchecktime;
  doc["ntpserver"] = config.ntpserver;
  doc["ntptimezone"] = config.ntptimezone;
  doc["ntpsynctime"] = config.ntpsynctime;
  doc["ntpwaitsynctime"] = config.ntpwaitsynctime;
  doc["pushoverenable"] = config.pushoverenable;
  doc["pushoverapptoken"] = config.pushoverapptoken;
  doc["pushoveruserkey"] = config.pushoveruserkey;
  doc["pushoverdevice"] = config.pushoverdevice;
  doc["metric"] = config.metric;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // need to print out the deserialisation to discern size

  // Close the file
  file.close();
}


// Prints the content of a file to the Serial
void printFile(const char *filename) {
  // Open file for reading
  File file = SPIFFS.open(filename);
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }
  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();
  // Close the file
  file.close();
}

void printConfig() {
  Serial.print("            hostname: "); Serial.println(config.hostname);
  Serial.print("             appname: "); Serial.println(config.appname);
  Serial.print("                ssid: "); Serial.println(config.ssid);
  Serial.print("        wifipassword: "); Serial.println("**********");
  Serial.print("            httpuser: "); Serial.println(config.httpuser);
  Serial.print("        httppassword: "); Serial.println("**********");
  Serial.print("        httpapitoken: "); Serial.println("**********");
  Serial.print("   webserverporthttp: "); Serial.println(config.webserverporthttp);
  Serial.print("  webserverporthttps: "); Serial.println(config.webserverporthttps);
  Serial.print("        syslogenable: "); Serial.println(config.syslogenable);
  Serial.print("        syslogserver: "); Serial.println(config.syslogserver);
  Serial.print("          syslogport: "); Serial.println(config.syslogport);
  Serial.print("      telegrafenable: "); Serial.println(config.telegrafenable);
  Serial.print("      telegrafserver: "); Serial.println(config.telegrafserver);
  Serial.print("  telegrafserverport: "); Serial.println(config.telegrafserverport);
  Serial.print("    telegrafshiptime: "); Serial.println(config.telegrafshiptime);
  Serial.print("       tempchecktime: "); Serial.println(config.tempchecktime);
  Serial.print("           ntpserver: "); Serial.println(config.ntpserver);
  Serial.print("         ntptimezone: "); Serial.println(config.ntptimezone);
  Serial.print("         ntpsynctime: "); Serial.println(config.ntpsynctime);
  Serial.print("     ntpwaitsynctime: "); Serial.println(config.ntpwaitsynctime);
  Serial.print("      pushoverenable: "); Serial.println(config.pushoverenable);
  Serial.print("    pushoverapptoken: "); Serial.println(config.pushoverapptoken);
  Serial.print("     pushoveruserkey: "); Serial.println(config.pushoveruserkey);
  Serial.print("      pushoverdevice: "); Serial.println(config.pushoverdevice);
  Serial.print("              metric: "); Serial.println(config.metric);
}

void saveConfigurationProbes(const char *filename) {
  // not using syslogSend here because it can be called before syslog has been configured

  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);

  // Open file for writing
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  StaticJsonDocument<1000> doc;

  if (numberOfTempProbes > 0) {
    JsonArray Probes = doc.createNestedArray("Probes");
    JsonObject myProbes[numberOfTempProbes];

    for (int i = 0; i < numberOfTempProbes; i++) {
      myProbes[i] = Probes.createNestedObject();
      myProbes[i]["number"] = i;
      myProbes[i]["name"] = myTempProbes[i].name;
      myProbes[i]["location"] = myTempProbes[i].location;
      myProbes[i]["address"] = giveStringDeviceAddress(myTempProbes[i].address);
      myProbes[i]["resolution"] = sensors.getResolution(myTempProbes[i].address);
      myProbes[i]["lowalarm"] = sensors.getLowAlarmTemp(myTempProbes[i].address);
      myProbes[i]["highalarm"] = sensors.getHighAlarmTemp(myTempProbes[i].address);
    }
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // need to print out the deserialisation to discern size

  // Close the file
  file.close();
}

int loadConfigurationProbes(const char *filename) {
  // not using syslogSend here because it can be called before syslog has been configured
  Serial.println("Loading configuration from " + String(filename));

  if (!SPIFFS.exists(filename)) {
    Serial.println(String(filename) + " not found");
    // return 0 loaded probes
    return 0;
  } else {
    Serial.println(String(filename) + " found");
  }

  // Open file for reading
  Serial.println("Opening " + String(filename));
  File file = SPIFFS.open(filename);

  //Serial.println("File contents=");
  //while (file.available()) {
  //  Serial.write(file.read());
  //}
  //Serial.println("\n========");

  if (!file) {
    Serial.println("ERROR: Failed to open file" + String(filename));
    return 0;
  }

  StaticJsonDocument<300> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Failed to process probes configuration file"); Serial.println(error.c_str());
    return 0;
  }

  JsonObject Probes_0 = doc["Probes"][0];
  int Probes_0_number = Probes_0["number"]; // 0
  const char* Probes_0_name = Probes_0["name"]; // "Probe 0"
  const char* Probes_0_location = Probes_0["location"]; // "Location 0"
  const char* Probes_0_address = Probes_0["address"]; // "28ff9ba36115351"
  int Probes_0_resolution = Probes_0["resolution"]; // 10
  int Probes_0_lowalarm = Probes_0["lowalarm"]; // 10
  int Probes_0_highalarm = Probes_0["highalarm"]; // 75

  Serial.println("===========");
  Serial.print("Size=doc[\"Probes\"]="); Serial.println(doc["Probes"].size());
  Serial.println("===========");
  Serial.print("Size=doc[\"Probes\"][0]="); Serial.println(Probes_0.size()); // 1
  Serial.println(Probes_0_number);
  Serial.println(Probes_0_name);
  Serial.println(Probes_0_location);
  Serial.println(Probes_0_address);
  Serial.println(Probes_0_resolution);
  Serial.println(Probes_0_lowalarm);
  Serial.println(Probes_0_highalarm);
  Serial.println("===========");

  for (int i = 0; i < doc["Probes"].size(); i++) {
    myTempProbes[i].name = doc["Probes"][i]["name"].as<String>();
    myTempProbes[i].location = doc["Probes"][i]["location"].as<String>();
    //myTempProbes[i].address = doc["Probes"][i]["address"];
    const char* temp = doc["Probes"][i]["address"].as<char*>();
    //const char* temp = doc["Probes"][i]["address"];

    Serial.print("temp="); Serial.println(temp);
    // https://forum.arduino.cc/index.php?topic=205352.msg1511851#msg1511851
    int addrv[8];
    sscanf(temp, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]);  // parse the 8 ascii hex bytes in 8 ints

    for(int j = 0; j < 8; j++) {
      myTempProbes[i].address[j] = (__typeof__(myTempProbes[i].address[0])) addrv[j]; //fill in device address bytes using a cast
    }

    Serial.println("after copying in address:" + giveStringDeviceAddress(myTempProbes[i].address));
    /*
    //const char* temp = "28ff9ba36115351";
    for (int j = 0; j < 8; j++) {
      myTempProbes[i].address[j] = temp[j];
      Serial.print(temp[j]); Serial.print("="); Serial.println(myTempProbes[i].address[j]);
    }
    */

    //myTempProbes[i].address = doc["Probes"][i]["address"].as<unsigned char>();
    myTempProbes[i].resolution = doc["Probes"][i]["resolution"];
    myTempProbes[i].lowalarm = doc["Probes"][i]["lowalarm"];
    myTempProbes[i].highalarm = doc["Probes"][i]["highalarm"];
  }

  file.close();

  numberOfLoadedTempProbes = doc["Probes"].size();

  // return the number of loaded probes
  return numberOfLoadedTempProbes;
}
