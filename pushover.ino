#include <HTTPClient.h>

void pushoverSend(String message) {
  const char *serverName = "https://api.pushover.net/1/messages.json";
  String httpRequestData = "token=" + default_potoken + "&user=" + default_pouser + "&title=" + config.hostname + "&message=" + message + "&device=" + default_podevice;

  HTTPClient http;
  
  http.begin(serverName);

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(httpRequestData);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // Free resources
  http.end();
}
