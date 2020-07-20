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

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

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
