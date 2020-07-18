#include <HTTPClient.h>

void pushoverSend(String message) {
  const char *serverName = "https://api.pushover.net/1/messages.json";
  String httpRequestData = "token=" + config.pushoverapptoken + "&user=" + config.pushoveruserkey + "&title=" + config.hostname + "&message=" + message + "&device=" + config.pushoverdevice;

  HTTPClient http;
  
  http.begin(serverName);

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(httpRequestData);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // Free resources
  http.end();
}
