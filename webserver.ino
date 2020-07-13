// parses and processes webpages
// if the webpage has %SOMETHING% or %SOMETHINGELSE% it will replace those strings with the ones defined
String processor(const String& var) {
  if (var == "SOMETHING") {
    String returnText = "";
    returnText = "something";
    return returnText;
  }

  if (var == "SOMETHINGELSE") {
    String returnText = "";
    returnText = "something else";
    return returnText;
  }

  if (var == "TEMP") {
    return getESPTemp();
  }

}

void configureWebServer() {
  // configure web server

  // if url isn't found
  server->onNotFound(notFound);

  server->on("/logout", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->requestAuthentication();
    request->send(401);
  });

  server->on("/logged-out", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);
    if (config.syslogenable) { syslog.log(logmessage); }
    request->send_P(401, "text/html", logout_html, processor);
  });

  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
      return request->requestAuthentication();
    }


    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++) {
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }


    String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();
    Serial.println(logmessage);
    if (config.syslogenable) { syslog.log(logmessage); }
    request->send_P(200, "text/html", index_html, processor);
  });
}

void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);
  if (config.syslogenable) { syslog.log(logmessage); }
  request->send(404, "text/plain", "Not found");
}
