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

  if (var == "LCDDISPLAY") {
    String lcdTable;
    lcdTable += "<table>";
    for (int i = 0; i < LCDROWS; i++) {
      lcdTable += "<tr><td>Line " + String(i + 1) + ":</td><td>" + lcdDisplay[i] + "</td></tr>";
    }
    lcdTable += "</table>";
    return lcdTable;
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

  if (var == "TIME") {
    return printTime();
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
    syslogSend("Client:" + request->client()->remoteIP().toString() + " " + request->url());
    request->send_P(401, "text/html", logout_html, processor);
  });

  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    // not using checkUserWebAuth here because this page is not presented via api
    if (!request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
      syslogSend("Client:" + request->client()->remoteIP().toString() + " " + request->url() + " Auth:Failed, Requesting Authentication");
      return request->requestAuthentication();
    }

    /*
    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++) {
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
    */

    syslogSend("Client:" + request->client()->remoteIP().toString() + + " " + request->url() + " Auth:Success");
    request->send_P(200, "text/html", index_html, processor);
  });

  server->on("/scani2c", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      syslog.log(logmessage);
      request->send(200, "application/json", i2cScanner());
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      syslog.log(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (checkUserWebAuth(request)) {
      request->send(200, "text/html", reboot_html);
      shouldReboot = true;
    } else {
      return request->requestAuthentication();
    }
  });

  server->on("/pushover", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (checkUserWebAuth(request)) {
      pushoverSend("Current Temp = " + getESPTemp() + "C");
      request->send(200, "text/html", "sent pushover message");
    } else {
      return request->requestAuthentication();
    }
  });

  server->on("/fullconfig", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (checkUserWebAuth(request)) {
      request->send(200, "application/json", getConfig());
    } else {
      return request->requestAuthentication();
    }
  });

  server->on("/shortstatus", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (checkUserWebAuth(request)) {
      request->send(200, "application/json", shortStatus());
    } else {
      return request->requestAuthentication();
    }
  });

  server->on("/set", HTTP_POST, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

    if (checkUserWebAuth(request)) {

      int numberOfParams = request->params();

      Serial.println("Param number:" + String(numberOfParams));
      for (int i = 0; i < numberOfParams; i++) {
        AsyncWebParameter* p = request->getParam(i);
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        Serial.println("------");
        if (checkSetting(p->name().c_str())) {
          Serial.printf("Param:%s is valid\n", p->name());
        } else {
          Serial.printf("Param:%s is invalid\n", p->name());
        }
      }

      if (numberOfParams == 0) {
        request->send(200, "text/plain", "ERROR: Too few params supplied");
        return;
      }

      if (numberOfParams > 1) {
        request->send(200, "text/plain", "ERROR: Too many params supplied");
        return;
      }

      const char *myParam = request->getParam(0)->name().c_str();
      const char *myValue = request->getParam(0)->value().c_str();

      if (checkSetting(myParam)) {
        Serial.printf("Param:%s is valid\n", myParam);
      } else {
        Serial.printf("Param:%s is invalid\n", myParam);
        request->send(200, "text/plain", "ERROR: Invalid config param supplied");
        return;
      }

      //printWebAdminArgs(request);

      bool initiatesave = false;

      if (strcmp(myParam, "hostname") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.hostname + " To:" + String(myValue));
        config.hostname = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: Hostname=" + config.hostname);
      } else if (strcmp(myParam, "appname") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.appname + " To:" + String(myValue));
        config.appname = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: AppName=" + config.appname);
      } else if (strcmp(myParam, "ssid") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.ssid + " To:" + String(myValue));
        config.ssid = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: SSID=" + config.ssid);
      }
      else if (strcmp(myParam, "wifipassword") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + "<redacted>" + " To:" + "<redacted>");
        config.wifipassword = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: WifiPassword=" + config.wifipassword);
      }
      else if (strcmp(myParam, "httpuser") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.httpuser + " To:" + String(myValue));
        config.httpuser = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: HTTPUser=" + config.httpuser);
      }
      else if (strcmp(myParam, "httppassword") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + "<redacted>" + " To:" + "<redacted>");
        config.httppassword = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: HTTPPassword=" + config.httppassword);
      }
      else if (strcmp(myParam, "httpapitoken") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + "<redacted>" + " To:" + "<redacted>");
        config.httpapitoken = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: HTTPAPIToken=" + config.httpapitoken);
      }
      else if (strcmp(myParam, "webserverporthttp") == 0) {
        if ((atoi(myValue) <= 0) || (atoi(myValue) > 65535)) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + String(config.webserverporthttp) + " To:" + String(myValue) + " Invalid port number");
          request->send(200, "text/plain", "ERROR: Invalid WebServerPortHTTP=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + String(config.webserverporthttp) + " To:" + String(myValue));
          config.webserverporthttp = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: WebServerPortHTTP=" + String(config.webserverporthttp));
        }
      }
      else if (strcmp(myParam, "webserverporthttps") == 0) {
        if ((atoi(myValue) <= 0) || (atoi(myValue) > 65535)) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + String(config.webserverporthttps) + " To:" + String(myValue) + " Invalid port number");
          request->send(200, "text/plain", "ERROR: Invalid WebServerPortHTTPS=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + String(config.webserverporthttps) + " To:" + String(myValue));
          config.webserverporthttps = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: WebServerPortHTTPS=" + String(config.webserverporthttps));
        }
      }
      else if (strcmp(myParam, "syslogenable") == 0) {
        if (strcmp(myValue, "true") == 0) {
          if (config.syslogenable) {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "true" + " To:true");
          } else {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "false" + " To:true");
          }
          config.syslogenable = true;
          initiatesave = true;
          request->send(200, "text/plain", "Updated: SyslogEnable=true");
        }
        else if (strcmp(myValue, "false") == 0) {
          if (config.syslogenable) {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "true" + " To:false");
          } else {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "false" + " To:false");
          }
          config.syslogenable = false;
          initiatesave = true;
          request->send(200, "text/plain", "Updated: SyslogEnable=false");
        } else {
          request->send(200, "text/plain", "ERROR: Invalid SyslogEnable value=" + String(myValue));
        }
      }
      else if (strcmp(myParam, "syslogserver") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.syslogserver + " To:" + String(myValue));
        config.syslogserver = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: SyslogServer=" + config.syslogserver);
      }
      else if (strcmp(myParam, "syslogport") == 0) {
        if ((atoi(myValue) <= 0) || (atoi(myValue) > 65535)) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + String(config.syslogport) + " To:" + String(myValue) + " Invalid port number");
          request->send(200, "text/plain", "ERROR: Invalid SyslogPort value=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + String(config.syslogport) + " To:" + String(myValue));
          config.syslogport = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: SyslogPort=" + String(config.syslogport));
        }
      }
      else if (strcmp(myParam, "telegrafenable") == 0) {
        if (strcmp(myValue, "true") == 0) {
          if (config.telegrafenable) {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "true" + " To:true");
          } else {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "false" + " To:true");
          }
          config.telegrafenable = true;
          initiatesave = true;
          request->send(200, "text/plain", "Updated: TelegrafEnable=true");
        } if (strcmp(myValue, "false") == 0) {
          if (config.telegrafenable) {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "true" + " To:false");
          } else {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "false" + " To:false");
          }
          config.telegrafenable = false;
          initiatesave = true;
          request->send(200, "text/plain", "Updated: TelegrafEnable=false");
        } else {
          request->send(200, "text/plain", "ERROR: Invalid TelegrafEnable value=" + String(myValue));
        }
      }
      else if (strcmp(myParam, "telegrafserver") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.telegrafserver + " To:" + String(myValue));
        config.telegrafserver = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: TelegrafServer=" + config.telegrafserver);
      }
      else if (strcmp(myParam, "telegrafserverport") == 0) {
        if ((atoi(myValue) <= 0) || (atoi(myValue) > 65535)) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + String(config.telegrafserverport) + " To:" + String(myValue) + " Invalid port number");
          request->send(200, "text/plain", "ERROR: Invalid TelegrafServerPort value=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + String(config.telegrafserverport) + " To:" + String(myValue));
          config.telegrafserverport = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: TelegrafServerPort=" + String(config.telegrafserverport));
        }
      }
      else if (strcmp(myParam, "telegrafshiptime") == 0) {
        if (atoi(myValue) <= 0) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + config.telegrafshiptime + " To:" + String(myValue) + "Invalid ship time");
          request->send(200, "text/plain", "ERROR: Invalid TelegrafShipTime value=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + config.telegrafshiptime + " To:" + String(myValue));
          config.telegrafshiptime = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: TelegrafShipTime=" + String(config.telegrafshiptime));
        }
      }
      else if (strcmp(myParam, "tempchecktime") == 0) {
        if (atoi(myValue) <= 0) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + config.tempchecktime + " To:" + String(myValue) + "Invalid temp check time");
          request->send(200, "text/plain", "ERROR: Invalid TempCheckTime value=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + config.tempchecktime + " To:" + String(myValue));
          config.tempchecktime = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: TempCheckTime=" + String(config.tempchecktime));
        }
      }
      else if (strcmp(myParam, "tempchecktime") == 0) {
        if (atoi(myValue) <= 0) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + config.tempchecktime + " To:" + String(myValue) + "Invalid temp check time");
          request->send(200, "text/plain", "ERROR: Invalid TempCheckTime value=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + config.tempchecktime + " To:" + String(myValue));
          config.tempchecktime = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: TempCheckTime=" + String(config.tempchecktime));
        }
      }
      else if (strcmp(myParam, "ntpserver") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.ntpserver + " To:" + String(myValue));
        config.ntpserver = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: NTPServer=" + config.ntpserver);
      }
      else if (strcmp(myParam, "ntptimezone") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.ntptimezone + " To:" + String(myValue));
        config.ntptimezone = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: NTPTimeZone=" + config.ntptimezone);
      }
      else if (strcmp(myParam, "ntpsynctime") == 0) {
        if (atoi(myValue) <= 0) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + config.ntpsynctime + " To:" + String(myValue) + "Invalid ntp sync time");
          request->send(200, "text/plain", "ERROR: Invalid NTPSyncTime value=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + config.ntpsynctime + " To:" + String(myValue));
          config.ntpsynctime = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: NTPSyncTime=" + String(config.ntpsynctime));
        }
      }
      else if (strcmp(myParam, "ntpwaitsynctime") == 0) {
        if (atoi(myValue) <= 0) {
          syslogSend("Setting Change Failed: " + String(myParam) + " From:" + config.ntpwaitsynctime + " To:" + String(myValue) + "Invalid ntp wait sync time");
          request->send(200, "text/plain", "ERROR: Invalid NTPSyncTime value=" + String(myValue));
        } else {
          syslogSend("Setting Change:" + String(myParam) + " From:" + config.ntpwaitsynctime + " To:" + String(myValue));
          config.ntpwaitsynctime = atoi(myValue);
          initiatesave = true;
          request->send(200, "text/plain", "Updated: NTPWaitSyncTime=" + String(config.ntpwaitsynctime));
        }
      }
      else if (strcmp(myParam, "pushoverenable") == 0) {
        if (strcmp(myValue, "true") == 0) {
          if (config.pushoverenable) {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "true" + " To:true");
          } else {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "false" + " To:true");
          }
          config.pushoverenable = true;
          initiatesave = true;
          request->send(200, "text/plain", "Updated: PushoverEnable=true");
        } if (strcmp(myValue, "false") == 0) {
          if (config.pushoverenable) {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "true" + " To:false");
          } else {
            syslogSend("Setting Change:" + String(myParam) + " From:" + "false" + " To:false");
          }
          config.pushoverenable = false;
          initiatesave = true;
          request->send(200, "text/plain", "Updated: PushoverEnable=false");
        } else {
          request->send(200, "text/plain", "ERROR: Invalid PushverEnable value=" + String(myValue));
        }
      }
      else if (strcmp(myParam, "pushoverapptoken") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.pushoverapptoken + " To:" + String(myValue));
        config.pushoverapptoken = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: PushoverAppToken=" + config.pushoverapptoken);
      }
      else if (strcmp(myParam, "pushoveruserkey") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.pushoveruserkey + " To:" + String(myValue));
        config.pushoveruserkey = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: PushoverUserKey=" + config.pushoveruserkey);
      }
      else if (strcmp(myParam, "pushoverdevice") == 0) {
        syslogSend("Setting Change:" + String(myParam) + " From:" + config.pushoverdevice + " To:" + String(myValue));
        config.pushoverdevice = myValue;
        initiatesave = true;
        request->send(200, "text/plain", "Updated: PushoverDevice=" + config.pushoverdevice);
      }
      //=====================
      else {
        syslogSend("ERROR: no valid config options supplied");
        request->send(200, "text/plain", "ERROR: No valid config options supplied");
      }

      if (initiatesave) {
        syslogSend("Saving Configuration:" + String(filename));
        saveConfiguration(filename, config);
      }

    } else {
      logmessage += " Auth: Failed";
      syslogSend(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    if (checkUserWebAuth(request)) {
      request->send(200, "text/plain", listFiles(true));
    } else {
      return request->requestAuthentication();
    }
  });

  server->on("/file", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      syslogSend(logmessage);

      printWebAdminArgs(request);

      if (request->hasParam("name") && request->hasParam("action")) {
        const char *fileName = request->getParam("name")->value().c_str();
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

        if (!SPIFFS.exists(fileName)) {
          syslogSend(logmessage + " ERROR:File does not exist");
          request->send(400, "text/plain", "ERROR:File does not exist");
        } else {
          syslogSend(logmessage + " file exists");
          if (strcmp(fileAction, "download") == 0) {
            logmessage += " downloaded";
            request->send(SPIFFS, fileName, "application/octet-stream");
          } else if (strcmp(fileAction, "delete") == 0) {
            logmessage += " deleted";
            SPIFFS.remove(fileName);
            request->send(200, "text/plain", "Deleted File:" + String(fileName));
          } else {
            logmessage += " ERROR:Invalid action param supplied";
            request->send(400, "text/plain", "ERROR:Invalid action param supplied");
          }
          syslogSend(logmessage);
        }
      } else {
        request->send(400, "text/plain", "ERROR:Name and action params required");
      }
    } else {
      logmessage += " Auth:Failed";
      syslogSend(logmessage);
      return request->requestAuthentication();
    }
  });
}

void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  syslogSend(logmessage);
  request->send(404, "text/plain", "Not found");
}

// used by server.on functions to discern whether a user has the correct httpapitoken OR is authenticated by username and password
bool checkUserWebAuth(AsyncWebServerRequest * request) {
  bool isAuthenticated = false;

  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

  if (request->hasParam("api") && (strcmp(request->getParam("api")->value().c_str(), config.httpapitoken.c_str()) == 0)) {
    syslogSend(logmessage + " Authenticated via API");
    isAuthenticated = true;
  }

  if (request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
    syslogSend(logmessage + " Authenticated via username and password");
    isAuthenticated = true;
  }

  if (isAuthenticated == false) {
    syslogSend(logmessage + " Failed Authentication");
  }
  
  return isAuthenticated;
}

void printWebAdminArgs(AsyncWebServerRequest * request) {
  Serial.println();
  int args = request->args();
  for (int i = 0; i < args; i++) {
    Serial.printf("ARG[%s]:%s\n", request->argName(i).c_str(), request->arg(i).c_str());
  }
}

// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // make sure authenticated before allowing upload, upload no available by api yet
  if (!request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
    return request->requestAuthentication();
  }

  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  syslogSend(logmessage);

  if (!index) {
    logmessage = "Upload Start:" + String(filename);
    // open the file on first call and store the file handle in the request object
    request->_tempFile = SPIFFS.open("/" + filename, "w");
    syslogSend(logmessage);
  }

  if (len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    logmessage = "Writing file:" + String(filename) + " index:" + String(index) + " len:" + String(len);
    syslogSend(logmessage);
  }

  if (final) {
    logmessage = "Upload Complete:" + String(filename) + " size:" + String(index + len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    syslogSend(logmessage);
    request->redirect("/");
  }
}
