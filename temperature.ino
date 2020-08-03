// https://randomnerdtutorials.com/esp32-ds18b20-temperature-arduino-ide/
// https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/06/ds18b20_esp32_single_normal.png?ssl=1

// http://www.esp32learning.com/code/esp32-and-ds18b20-temperature-sensor-example.php
// https://i2.wp.com/www.esp32learning.com/wp-content/uploads/2017/11/esp32-and-ds18b20_bb.png?w=792

void fixSetupDS18B20() {
  // needed because of https://github.com/milesburton/Arduino-Temperature-Control-Library/issues/113
  sensors.setWaitForConversion(false);
  pinMode(oneWireBus, OUTPUT);
  digitalWrite(oneWireBus, HIGH);
  delay(750);
}

void fixDS18B20() {
  sensors.requestTemperatures();
}

String printTempProbe(DeviceAddress deviceAddress, bool printScale) {
  String returnText;
  if (config.metric) {
    returnText += String(sensors.getTempC(deviceAddress));
    if (printScale) {
      returnText += " C";
    }
  } else {
    returnText += String(roundf(DallasTemperature::toFahrenheit(sensors.getTempC(deviceAddress)) * 100) / 100);
    if (printScale) {
      returnText += " F";
    }
  }
  return returnText;
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i != 7) Serial.print(",");
  }
}

String giveStringDeviceAddress(DeviceAddress deviceAddress)
{
  String returnText;
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) returnText += "0";
    //Serial.print(deviceAddress[i], HEX);
    returnText += String(deviceAddress[i], HEX);
  }
  return returnText;
}

int checkTempProbeKnown(DeviceAddress deviceAddress) {
  // if no probes have been loaded from file, return 0
  if (numberOfLoadedTempProbes <= 0) {
    return 0;
  }


  for (int i = 0; i < numberOfLoadedTempProbes; i++) {
    if (giveStringDeviceAddress(deviceAddress) == giveStringDeviceAddress(myTempProbes[i].address)) {
      // return the position in myTempProbes if device is already known
      return i + 1;
    }
  }

  // device not found, returning 0
  return 0;
}

void printTempProbesSerial(){
  for (int i = 0; i < numberOfTempProbes; i++) {
    Serial.println("==============");
    Serial.println(String(i) + "       name:" + myTempProbes[i].name);
    Serial.println(String(i) + "   location:" + myTempProbes[i].location);
    Serial.println(String(i) + "    address:" + giveStringDeviceAddress(myTempProbes[i].address));
    Serial.println(String(i) + " resolution:" + myTempProbes[i].resolution);
    Serial.println(String(i) + "   lowalarm:" + myTempProbes[i].lowalarm);
    Serial.println(String(i) + "  highalarm:" + myTempProbes[i].highalarm);
    Serial.println(String(i) + "temperature:" + printTempProbe(myTempProbes[i].address, true));
  }
}

String displayConfiguredProbes() {
  syslogSend("Loading DS18B20 Temperature Probes");

  // if no probes have been loaded, return empty json
  if (numberOfLoadedTempProbes == 0) {
    return "[]";
  }

  String returnText = "[";

  int i = 0;
  for (int i = 0; i < numberOfTempProbes; i++) {
    if (returnText.length() > 1) returnText += ",";
    returnText += "{";
    returnText += "\"number\":" + String(i);
    returnText += ",\"address\":\"" + giveStringDeviceAddress(myTempProbes[i].address) + "\"";
    returnText += ",\"name\":\"" + myTempProbes[i].name + "\"";
    returnText += ",\"location\":\"" + myTempProbes[i].location + "\"";
    returnText += ",\"resolution\":" + String(myTempProbes[i].resolution, DEC);
    returnText += ",\"lowalarm\":" + String(myTempProbes[i].lowalarm, DEC);
    returnText += ",\"highalarm\":" + String(myTempProbes[i].highalarm, DEC);
    returnText += ",\"temperature\":\"" + printTempProbe(myTempProbes[i].address, true) + "\"";
    returnText += "}";
  }

  returnText += "]";
  return returnText;
}

String probeScanner() {
  syslogSend("Scanning DS18B20 Temperature Probes");
  DeviceAddress foundDevice;
  char alarmHigh;
  char alarmLow;

  // need to reset_search before each scan
  oneWire.reset_search();

  //int numberProbesFound = sensors.getDeviceCount();
  numberOfTempProbes = sensors.getDeviceCount();
  syslogSend("Found " + String(numberOfTempProbes) + " DS18B20 Probes");

  if (numberOfTempProbes == 0) {
    // if no probes have been found, return empty json
    anyScannedProbes = false;
    return "[]";
  } else {
    // delete any scanned probes already in memory because we are doing a new scan
    delete[] scannedProbes;
    scannedProbes = new TempProbe[numberOfTempProbes];
    anyScannedProbes = true;
  }

  String returnText = "[";

  int i = 0;
  while (oneWire.search(foundDevice)) {
    if (returnText.length() > 1) returnText += ",";
    returnText += "{";

    returnText += "\"number\":" + String(i);

    int isDeviceKnown = checkTempProbeKnown(foundDevice);

    if (isDeviceKnown == 0) {
      // device is not one that has been loaded from file
      scannedProbes[i].name = "New Probe " + String(i);
      scannedProbes[i].location = "New Location " + String(i);
    } else {
      // device is already known, its position in myTempProbes array is (isDeviceKnown - 1)
      scannedProbes[i].name = myTempProbes[isDeviceKnown - 1].name;
      scannedProbes[i].location = myTempProbes[isDeviceKnown - 1].location;
    }

    returnText += ",\"name\":\"" + scannedProbes[i].name + "\"";
    returnText += ",\"location\":\"" + scannedProbes[i].location + "\"";

    for (uint8_t j = 0; j < 8; j++) {
      scannedProbes[i].address[j] = foundDevice[j];
    }

    scannedProbes[i].resolution = sensors.getResolution(foundDevice);
    scannedProbes[i].lowalarm = sensors.getLowAlarmTemp(foundDevice);
    scannedProbes[i].highalarm = sensors.getHighAlarmTemp(foundDevice);

    returnText += ",\"address\":\"" + giveStringDeviceAddress(foundDevice) + "\"";
    returnText += ",\"resolution\":" + String(sensors.getResolution(foundDevice), DEC);
    alarmLow = sensors.getLowAlarmTemp(foundDevice);
    alarmHigh = sensors.getHighAlarmTemp(foundDevice);

    if (config.metric) {
      returnText += ",\"lowalarm\":\"" + String(alarmLow, DEC) + " C" + "\"";
      returnText += ",\"highalarm\":\"" + String(alarmHigh, DEC) + " C" + "\"";
    } else {
      returnText += ",\"lowalarm\":\"" + String(roundf(DallasTemperature::toFahrenheit(alarmLow) * 100) / 100) + " F" + "\"";
      returnText += ",\"highalarm\":\"" + String(roundf(DallasTemperature::toFahrenheit(alarmHigh) * 100) / 100) + " F" + "\"";
    }

    returnText += ",\"temperature\":\"" + printTempProbe(scannedProbes[i].address, true) + "\"";

    returnText += "}";
    i++;
  }

  returnText += "]";
  return returnText;
}

//=== DELETE BELOW ===

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

void printAlarms(uint8_t deviceAddress[])
{
  char temp;
  temp = sensors.getHighAlarmTemp(deviceAddress);
  Serial.print("High Alarm: ");
  Serial.print(temp, DEC);
  Serial.print("C/");
  Serial.print(DallasTemperature::toFahrenheit(temp));
  Serial.print("F | Low Alarm: ");
  temp = sensors.getLowAlarmTemp(deviceAddress);
  Serial.print(temp, DEC);
  Serial.print("C/");
  Serial.print(DallasTemperature::toFahrenheit(temp));
  Serial.print("F");
}

void printAlarmsNew(uint8_t deviceAddress[])
{
  char temp;
  temp = sensors.getHighAlarmTemp(deviceAddress);
  Serial.print("High Alarm: ");
  if (config.metric) {
    Serial.print(temp, DEC);
    Serial.print("C");
  } else {
    Serial.print(DallasTemperature::toFahrenheit(temp));
    Serial.print("F");
  }
  Serial.print(" | Low Alarm: ");
  temp = sensors.getLowAlarmTemp(deviceAddress);
  if (config.metric) {
    Serial.print(temp, DEC);
    Serial.print("C");
  } else {
    Serial.print(DallasTemperature::toFahrenheit(temp));
    Serial.print("F");
  }
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

void checkAlarm(DeviceAddress deviceAddress)
{
  if (sensors.hasAlarm(deviceAddress))
  {
    Serial.print("ALARM: ");
    printData(deviceAddress);
  }
}
