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

  if (var == "FIRMWARE") {
    return FIRMWARE_VERSION;
  }

  if (var == "FREESPIFFS") {
    return humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
  }

  if (var == "USEDSPIFFS") {
    return humanReadableSize(SPIFFS.usedBytes());
  }

  if (var == "TOTALSPIFFS") {
    return humanReadableSize(SPIFFS.totalBytes());
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
    if (config.syslogenable) {
      syslog.log(logmessage);
    }
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
    if (config.syslogenable) {
      syslog.log(logmessage);
    }
    request->send_P(200, "text/html", index_html, processor);
  });

  server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

    if (checkUserWebAuth(request)) {
      request->send(200, "text/html", reboot_html);
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      if (config.syslogenable) {
        syslog.log(logmessage);
      }
      shouldReboot = true;
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      if (config.syslogenable) {
        syslog.log(logmessage);
      }
      return request->requestAuthentication();
    }
  });

  server->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      if (config.syslogenable) { syslog.log(logmessage); }
      request->send(200, "text/plain", listFiles(true));
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      if (config.syslogenable) { syslog.log(logmessage); }
      return request->requestAuthentication();
    }
  });

  server->on("/file", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      if (config.syslogenable) { syslog.log(logmessage); }

      printWebAdminArgs(request);

      if (request->hasParam("name") && request->hasParam("action")) {
        const char *fileName = request->getParam("name")->value().c_str();
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

        if (!SPIFFS.exists(fileName)) {
          Serial.println(logmessage + " ERROR: file does not exist");
          if (config.syslogenable) { syslog.log(logmessage + " ERROR: file does not exist"); }
          request->send(400, "text/plain", "ERROR: file does not exist");
        } else {
          Serial.println(logmessage + " file exists");
          if (config.syslogenable) { syslog.log(logmessage + " file exists"); }
          if (strcmp(fileAction, "download") == 0) {
            logmessage += " downloaded";
            request->send(SPIFFS, fileName, "application/octet-stream");
          } else if (strcmp(fileAction, "delete") == 0) {
            logmessage += " deleted";
            SPIFFS.remove(fileName);
            request->send(200, "text/plain", "Deleted File: " + String(fileName));
          } else {
            logmessage += " ERROR: invalid action param supplied";
            request->send(400, "text/plain", "ERROR: invalid action param supplied");
          }
          Serial.println(logmessage);
          if (config.syslogenable) { syslog.log(logmessage); }
        }
      } else {
        request->send(400, "text/plain", "ERROR: name and action params required");
      }
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      if (config.syslogenable) { syslog.log(logmessage); }
      return request->requestAuthentication();
    }
  });
}

void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);
  if (config.syslogenable) {
    syslog.log(logmessage);
  }
  request->send(404, "text/plain", "Not found");
}

// used by server.on functions to discern whether a user has the correct httpapitoken OR is authenticated by username and password
bool checkUserWebAuth(AsyncWebServerRequest * request) {
  bool isAuthenticated = false;

  if (request->hasParam("api") && (strcmp(request->getParam("api")->value().c_str(), config.httpapitoken.c_str()) == 0)) {
    isAuthenticated = true;
  }

  if (request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
    Serial.println("is authenticated via username and password");
    isAuthenticated = true;
  }
  return isAuthenticated;
}

void printWebAdminArgs(AsyncWebServerRequest * request) {
  int args = request->args();
  for (int i = 0; i < args; i++) {
    Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
  }
}
