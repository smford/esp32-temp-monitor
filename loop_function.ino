void loop() {
  // display ntp sync events on serial
  events();

  // reboot if we've told it to reboot
  if (shouldReboot) {
    rebootESP("Web Admin Initiated Reboot");
  }

  // do nothing and return if trying to ship metrics too fast
  if ((millis() - telegrafLastRunTime) > (config.telegrafshiptime * 1000)) {
    shipMetric("cputemp", getESPTemp(false));
    shipMetric("wifisignal", String(WiFi.RSSI()));

    // remove this
    shipMetric("telegrafdelay", String(millis() - telegrafLastRunTime));

    if (numberOfLoadedTempProbes > 0) {
    fixDS18B20();
      for (int i = 0; i < numberOfLoadedTempProbes; i++) {
        String cleanName = myTempProbes[i].name;
        cleanName.replace(" ", "_");
        cleanName.toLowerCase();
        shipMetric("probe-" + cleanName, printTempProbe(myTempProbes[i].address, false));
      }
    }
    telegrafLastRunTime = millis();
  }

  if ((millis() - tempCheckLastRunTime) > (config.tempchecktime * 1000)) {
    fixDS18B20();
    printLCD("  CPU:" + getESPTemp(), "Probe:" + printTempProbe(myTempProbes[0].address));
    tempCheckLastRunTime = millis();
  }

}
