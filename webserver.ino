// parses and processes webpages
String processor(const String& var) {
  if (var == "HOSTNAME") {
    return config.hostname;
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

  // run handleUpload function when any file is uploaded
  server->onFileUpload(handleUpload);

  server->on("/logout", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->requestAuthentication();
    request->send(401);
  });

  server->on("/logged-out", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);
    syslogSend(logmessage);
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
    syslogSend(logmessage);
    request->send_P(200, "text/html", index_html, processor);
  });

  server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

    if (checkUserWebAuth(request)) {
      request->send(200, "text/html", reboot_html);
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      syslogSend(logmessage);
      shouldReboot = true;
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      syslogSend(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/set1", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

    if (checkUserWebAuth(request)) {
      if (request->hasParam("name") && request->hasParam("value")) {
        const char *Name = request->getParam("name")->value().c_str();
        const char *Value = request->getParam("value")->value().c_str();
        if (setValue(Name, Value)) {
          request->send(200, "text/plain", "Set: " + String(Name) + "=" + String(Value));
        } else {
          request->send(200, "text/plain", "Failed Set: " + String(Name) + "=" + String(Value));
        }
      }
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      syslogSend(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/fullconfig", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      syslog.log(logmessage);
      request->send(200, "application/json", getConfig());
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      syslog.log(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/set", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

    if (checkUserWebAuth(request)) {

      int paramsNr = request->params();
      Serial.println(paramsNr);
      for (int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        Serial.println("------");
      }

      if (request->hasParam("hostname")) {
        config.hostname = request->getParam("hostname")->value();
        request->send(200, "text/plain", "Set: hostname=" + config.hostname);
      } else if (request->hasParam("appname")) {
        config.appname = request->getParam("appname")->value();
        request->send(200, "text/plain", "Set: appname=" + config.appname);
      } else {
        Serial.println("no matching params supplied");
        request->send(200, "text/plain", "no setting supplied");
      }
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      syslogSend(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String logmessage = "Client: " + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      syslogSend(logmessage);
      request->send(200, "text/plain", listFiles(true));
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      syslogSend(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/file", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client: " + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      syslogSend(logmessage);

      printWebAdminArgs(request);

      if (request->hasParam("name") && request->hasParam("action")) {
        const char *fileName = request->getParam("name")->value().c_str();
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client: " + request->client()->remoteIP().toString() + " " + request->url() + " ?name=" + String(fileName) + "&action=" + String(fileAction);

        if (!SPIFFS.exists(fileName)) {
          Serial.println(logmessage + " ERROR : file does not exist");
          syslogSend(logmessage + " ERROR : file does not exist");
          request->send(400, "text/plain", "ERROR : file does not exist");
        } else {
          Serial.println(logmessage + " file exists");
          syslogSend(logmessage + " file exists");
          if (strcmp(fileAction, "download") == 0) {
            logmessage += " downloaded";
            request->send(SPIFFS, fileName, "application/octet-stream");
          } else if (strcmp(fileAction, "delete") == 0) {
            logmessage += " deleted";
            SPIFFS.remove(fileName);
            request->send(200, "text/plain", "Deleted File : " + String(fileName));
          } else {
            logmessage += " ERROR : invalid action param supplied";
            request->send(400, "text/plain", "ERROR : invalid action param supplied");
          }
          Serial.println(logmessage);
          syslogSend(logmessage);
        }
      } else {
        request->send(400, "text/plain", "ERROR : name and action params required");
      }
    } else {
      logmessage += " Auth : Failed";
      Serial.println(logmessage);
      syslogSend(logmessage);
      return request->requestAuthentication();
    }
  });
}

void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client : " + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);
  syslogSend(logmessage);
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
    Serial.printf("ARG[ % s] : % s\n", request->argName(i).c_str(), request->arg(i).c_str());
  }
}

// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // make sure authenticated before allowing upload
  if (!request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
    return request->requestAuthentication();
  }

  String logmessage = "Client : " + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);
  syslogSend(logmessage);

  if (!index) {
    logmessage = "Upload Start : " + String(filename);
    // open the file on first call and store the file handle in the request object
    request->_tempFile = SPIFFS.open("/" + filename, "w");
    Serial.println(logmessage);
    syslogSend(logmessage);
  }

  if (len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    logmessage = "Writing file : " + String(filename) + " index=" + String(index) + " len=" + String(len);
    Serial.println(logmessage);
    syslogSend(logmessage);
  }

  if (final) {
    logmessage = "Upload Complete : " + String(filename) + " size: " + String(index + len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    Serial.println(logmessage);
    syslogSend(logmessage);
    request->redirect("/");
  }
}

bool setValue(const char *Name, const char *Value) {
  if (strcmp(Name, "hostname") == 0) {
    Serial.printf("Name = % s equals hostname", Name);
  } else {
    Serial.printf("Name = % s does not equal hostname", Name);
  }

}
