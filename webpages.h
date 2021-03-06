const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <style>
  td.top {
    vertical-align: text-top;
  }
  </style>
</head>
<body>
  <p>
  <table>
  <tr><td><span id="hostname">%HOSTNAME%</span></td></tr>
  <tr><td>Firmware:</td><td>%FIRMWARE%</td></tr>
  <tr><td>Storage:</td><td>Free: <span id="freespiffs">%FREESPIFFS%</span> | Used: <span id="usedspiffs">%USEDSPIFFS%</span> | Total: <span id="totalspiffs">%TOTALSPIFFS%</span></td></tr>
  <tr><td>ESP32 Temp:</td><td><span id="cputemp">%TEMP%</span></td></tr>
  <tr><td>Time:</td><td><span id="time">%TIME%</span></td></tr>
  <tr><td>Status:</td><td><span id="status"> </span></td></tr>
  <tr><td class='top'>LCD:</td><td><span id="lcddisplay">%LCDDISPLAY%</span></td></tr>
  </table>
  </p>
  <p>
  <button onclick="logoutButton()">Logout</button>
  <button onclick="rebootButton()">Reboot</button>
  <button onclick="listFilesButton()">List Files</button>
  <button onclick="showUploadButtonFancy()">Upload File</button>
  <button onclick="displayEditConfig()">Display/Edit Config</button>
  <button onclick="updateHeader()">Refresh Information</button>
  <button onclick="scani2c()">Scan I2C Devices</button>
  <button onclick="scanProbes()">Scan for DS18B20 Temp Probes</button>
  <button id="saveprobes" onclick="saveProbes()" %SAVESPROBESSCAN%>Save Scanned DS18B20 Temp Probes</button>
  <button id="editprobes" onclick="editProbes()" %ANYLOADEDPROBES%>Edit Known DS18B20 Temp Probes</button>
  <button onclick="displayWifi()">Display WiFi Networks</button>
  <button onclick="refreshNTP()">Refresh NTP</button>
  <button onclick="changeBacklightButton('on')">LCD Backlight On</button>
  <button onclick="changeBacklightButton('off')">LCD Backlight Off</button>
  </p>
  <p id="detailsheader"></p>
  <p id="details"></p>
<script>
function refreshNTP() {
  document.getElementById("status").innerHTML = "Refreshing NTP ...";
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/ntprefresh", true);
  xhr.send();
  setTimeout(function(){
    document.getElementById("status").innerHTML = "Refreshed NTP";
    document.getElementById("time").innerHTML = xhr.responseText;
  },5000);
}
function changeBacklightButton(state) {
  document.getElementById("status").innerHTML = "Turning LCD Backlight " . state;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/backlight?state=" + state, true);
  xhr.send();
  document.getElementById("status").innerHTML = "LCD Backlight " + state;
}
function displayWifi() {
  document.getElementById("status").innerHTML = "Scanning for WiFi Networks ...";
  document.getElementById("detailsheader").innerHTML = "<h3>Available WiFi Networks<h3>";
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/scanwifi", false);
  xmlhttp.send();
  var mydata = JSON.parse(xmlhttp.responseText);
  var displaydata = "<table><tr><th align='left'>SSID</th><th align='left'>BSSID</th><th align='left'>RSSI</th><th align='left'>Channel</th><th align='left'>Secure</th></tr>";
  for (var key of Object.keys(mydata)) {
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["ssid"] + "</td><td align='left'>" + mydata[key]["bssid"] + "</td><td align='left'>" + mydata[key]["rssi"] + "</td><td align='left'>" + mydata[key]["channel"] + "</td><td align='left'>"+ mydata[key]["secure"] + "</td></tr>";
  }
  displaydata = displaydata + "</table>";
  document.getElementById("status").innerHTML = "WiFi Networks Scanned";
  document.getElementById("details").innerHTML = displaydata;
}
function scani2c() {
  document.getElementById("status").innerHTML = "Scanning for I2C Devices";
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/scani2c", false);
  xmlhttp.send();
  var mydata = JSON.parse(xmlhttp.responseText);
  var displaydata = "<table><tr><th align='left'>Int</th><th align='left'>Hex</th><th align='left'>Error</th></tr>";
  for (var key of Object.keys(mydata)) {
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["int"] + "</td><td align='left'>" + mydata[key]["hex"] + "</td><td align='left'>" + mydata[key]["error"] + "</td></tr>";
  }
  displaydata = displaydata + "</table>";
  document.getElementById("status").innerHTML = "I2C Devices Scanned";
  document.getElementById("detailsheader").innerHTML = "<h3>Found I2C Devices<h3>";
  document.getElementById("details").innerHTML = displaydata;
}
function scanProbes() {
  document.getElementById("status").innerHTML = "Scanning for DS18B20 Temperature Probes";
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/scanprobes", false);
  xmlhttp.send();
  var mydata = JSON.parse(xmlhttp.responseText);
  var displaydata = "<table>";
  displaydata = displaydata + "<tr><th align='left'>Number</th>";
  displaydata = displaydata + "<th align='left'>Name</th>";
  displaydata = displaydata + "<th align='left'>Location</th>";
  displaydata = displaydata + "<th align='left'>Address</th>";
  displaydata = displaydata + "<th align='left'>Current Temperature</th>";
  displaydata = displaydata + "<th align='left'>Bit Res</th>";
  displaydata = displaydata + "<th align='left'>Low Alarm</th>";
  displaydata = displaydata + "<th align='left'>High Alarm</th></tr>";
  for (var key of Object.keys(mydata)) {
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["number"] + "</td>";
    displaydata = displaydata + "<td align='left'>" + mydata[key]["name"] + "</td>";
    displaydata = displaydata + "<td align='left'>" + mydata[key]["location"] + "</td>";
    displaydata = displaydata + "<td align='left'>" + mydata[key]["address"] + "</td>";
    displaydata = displaydata + "<td align='left'>" + mydata[key]["temperature"] + "</td>";
    displaydata = displaydata + "<td align='left'>" + mydata[key]["resolution"] + "</td>";
    displaydata = displaydata + "<td align='left'>" + mydata[key]["lowalarm"] + "</td>";
    displaydata = displaydata + "<td align='left'>" + mydata[key]["highalarm"] + "</td></tr>";
  }
  displaydata = displaydata + "</table>";
  if (mydata.count == 0) {
    document.getElementById("saveprobes").disabled = true;
  } else {
    document.getElementById("saveprobes").disabled = false;
  }
  document.getElementById("status").innerHTML = "DS18B20 Temperature Probes Scanned";
  document.getElementById("detailsheader").innerHTML = "<h3>Found DS18B20 Temperature Probes<h3>";
  document.getElementById("details").innerHTML = displaydata;
}
function saveProbes() {
  document.getElementById("status").innerHTML = "Saving Scanned DS18B20 Temperature Probes";
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/scanprobessave", false);
  xmlhttp.send();
  var mydata = JSON.parse(xmlhttp.responseText);
  if (mydata["number"] > 0) {
    document.getElementById("editprobes").disabled = false;
    document.getElementById("saveprobes").disabled = true;
  }
  document.getElementById("status").innerHTML = mydata["message"];
}
function updateHeader() {
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/shortstatus", false);
  xmlhttp.send();
  var mydata = JSON.parse(xmlhttp.responseText);
  document.getElementById("hostname").innerHTML = mydata["Hostname"];
  document.getElementById("freespiffs").innerHTML = mydata["FreeSPIFFS"];
  document.getElementById("usedspiffs").innerHTML = mydata["UsedSPIFFS"];
  document.getElementById("totalspiffs").innerHTML = mydata["TotalSPIFFS"];
  document.getElementById("time").innerHTML = mydata["Time"];
  document.getElementById("cputemp").innerHTML = mydata["CPUTemp"];
  document.getElementById("lcddisplay").innerHTML = mydata["LCD"];
}
function submitForm(oFormElement) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    document.getElementById("status").innerHTML = xhr.responseText;
    displayEditConfig();
  }
  xhr.open(oFormElement.method, oFormElement.getAttribute("action"));
  xhr.send(new FormData(oFormElement));
  setTimeout(function(){
    updateHeader();
  }, 2000);
  return false;
}
function displayEditConfig() {
  document.getElementById("detailsheader").innerHTML = "<h3>Configuration<h3>";
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/fullconfig", false);
  xmlhttp.send();
  var mydata = JSON.parse(xmlhttp.responseText);
  var displaydata = "<table><tr><th align='left'>Setting</th><th align='left'>Current</th><th align='left'>New</th></tr>";
  for (var key of Object.keys(mydata)) {
    /*if (key.toLowerCase().includes("password") || key.toLowerCase().includes("token") || key.toLowerCase().includes("key")) {
      displaydata = displaydata + "<tr><td align='left'>" + key + "</td><td align='left'>" + "**********" + "</td><td align='left'><form method='POST' onsubmit='return submitForm(this);' action='/set'><input type='text' name='" + key.toLowerCase() + "'>" + "<input type='submit' value='Submit'></form>" + "</td></tr>";
    } else {
      displaydata = displaydata + "<tr><td align='left'>" + key + "</td><td align='left'>" + mydata[key] + "</td><td align='left'><form method='POST' onsubmit='return submitForm(this);' action='/set'><input type='text' name='" + key.toLowerCase() + "'>" + "<input type='submit' value='Submit'></form>" + "</td></tr>";
    }*/
    displaydata = displaydata + "<tr><td align='left'>" + key + "</td><td align='left'>" + mydata[key] + "</td><td align='left'><form method='POST' onsubmit='return submitForm(this);' action='/set'><input type='text' name='" + key.toLowerCase() + "'>" + "<input type='submit' value='Submit'></form>" + "</td></tr>";

  }
  displaydata = displaydata + "</table>";
  document.getElementById("details").innerHTML = displaydata;
}
function logoutButton() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/logout", true);
  xhr.send();
  setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
}
function rebootButton() {
  document.getElementById("status").innerHTML = "Invoking Reboot ...";
  //var xhr = new XMLHttpRequest();
  //xhr.open("GET", "/reboot", true);
  //xhr.send();
  window.open("/reboot","_self");
}
function listFilesButton() {
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/listfiles", false);
  xmlhttp.send();
  document.getElementById("detailsheader").innerHTML = "<h3>Files<h3>";
  document.getElementById("details").innerHTML = xmlhttp.responseText;
}
function downloadDeleteButton(filename, action) {
  var urltocall = "/file?name=" + filename + "&action=" + action;
  xmlhttp=new XMLHttpRequest();
  if (action == "delete") {
    xmlhttp.open("GET", urltocall, false);
    xmlhttp.send();
    document.getElementById("status").innerHTML = xmlhttp.responseText;
    xmlhttp.open("GET", "/listfiles", false);
    xmlhttp.send();
    document.getElementById("details").innerHTML = xmlhttp.responseText;
    updateHeader();
  }
  if (action == "download") {
    document.getElementById("status").innerHTML = "";
    window.open(urltocall,"_blank");
  }
}
function showUploadButtonFancy() {
  document.getElementById("detailsheader").innerHTML = "<h3>Upload File<h3>"
  document.getElementById("status").innerHTML = "";
  var uploadform = "<form method = \"POST\" action = \"/\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\"/><input type=\"submit\" name=\"upload\" value=\"Upload\" title = \"Upload File\"></form>"
  document.getElementById("details").innerHTML = uploadform;
  var uploadform =
  "<form id=\"upload_form\" enctype=\"multipart/form-data\" method=\"post\">" +
  "<input type=\"file\" name=\"file1\" id=\"file1\" onchange=\"uploadFile()\"><br>" +
  "<progress id=\"progressBar\" value=\"0\" max=\"100\" style=\"width:300px;\"></progress>" +
  "<h3 id=\"status\"></h3>" +
  "<p id=\"loaded_n_total\"></p>" +
  "</form>";
  document.getElementById("details").innerHTML = uploadform;
}
function _(el) {
  return document.getElementById(el);
}
function uploadFile() {
  var file = _("file1").files[0];
  // alert(file.name+" | "+file.size+" | "+file.type);
  var formdata = new FormData();
  formdata.append("file1", file);
  var ajax = new XMLHttpRequest();
  ajax.upload.addEventListener("progress", progressHandler, false);
  ajax.addEventListener("load", completeHandler, false); // doesnt appear to ever get called even upon success
  ajax.addEventListener("error", errorHandler, false);
  ajax.addEventListener("abort", abortHandler, false);
  ajax.open("POST", "/");
  ajax.send(formdata);
}
function progressHandler(event) {
  //_("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes of " + event.total; // event.total doesnt show accurate total file size
  _("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes";
  var percent = (event.loaded / event.total) * 100;
  _("progressBar").value = Math.round(percent);
  _("status").innerHTML = Math.round(percent) + "% uploaded... please wait";
  if (percent >= 100) {
    _("status").innerHTML = "Please wait, writing file to filesystem";
  }
}
function completeHandler(event) {
  _("status").innerHTML = "Upload Complete";
  _("progressBar").value = 0;
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/listfiles", false);
  xmlhttp.send();
  document.getElementById("status").innerHTML = "File Uploaded";
  document.getElementById("detailsheader").innerHTML = "<h3>Files<h3>";
  document.getElementById("details").innerHTML = xmlhttp.responseText;
  updateHeader();
}
function errorHandler(event) {
  _("status").innerHTML = "Upload Failed";
}
function abortHandler(event) {
  _("status").innerHTML = "Upload Aborted";
}
function editProbes() {
  document.getElementById("status").innerHTML = "Loading Known DS18B20 Temperature Probes";
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/loadprobes", false);
  xmlhttp.send();
  var mydata = JSON.parse(xmlhttp.responseText);
  var displaydata = "";
  for (var key of Object.keys(mydata)) {
    displaydata = displaydata + "<table><tr><th align='left'>ID</th><th align='left'>Setting</th><th align='left'>Current</th><th align='left'>New</th></tr>";
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["number"] + "</td><td align='left'>Address</td><td align='left'>" + mydata[key]["address"] + "</td><td></td></tr>";
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["number"] + "</td><td align='left'>Current Temperature</td><td align='left'>" + mydata[key]["temperature"] + "</td><td></td></tr>";
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["number"] + "</td><td align='left'>Name</td><td align='left'>" + mydata[key]["name"] + "</td><td align='left'><form method='POST' onsubmit='return submitFormProbes(this);' action='/setprobe?probe=" + mydata[key]["number"] + "'><input type='text' name='name'>" + "<input type='submit' value='Submit'></form></td></tr>";
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["number"] + "</td><td align='left'>Location</td><td align='left'>" + mydata[key]["location"] + "</td><td align='left'><form method='POST' onsubmit='return submitFormProbes(this);' action='/setprobe?probe=" + mydata[key]["number"] + "'><input type='text' name='location'>" + "<input type='submit' value='Submit'></form></td></tr>";
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["number"] + "</td><td align='left'>Bit Resolution</td><td align='left'>" + mydata[key]["resolution"] + "</td><td align='left'><form method='POST' onsubmit='return submitFormProbes(this);' action='/setprobe?probe=" + mydata[key]["number"] + "'><input type='text' name='resolution'>" + "<input type='submit' value='Submit'></form></td></tr>";
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["number"] + "</td><td align='left'>Low Alarm</td><td align='left'>" + mydata[key]["lowalarm"] + "</td><td align='left'><form method='POST' onsubmit='return submitFormProbes(this);' action='/setprobe?probe=" + mydata[key]["number"] + "'><input type='text' name='lowalarm'>" + "<input type='submit' value='Submit'></form></td></tr>";
    displaydata = displaydata + "<tr><td align='left'>" + mydata[key]["number"] + "</td><td align='left'>High Alarm</td><td align='left'>" + mydata[key]["highalarm"] + "</td><td align='left'><form method='POST' onsubmit='return submitFormProbes(this);' action='/setprobe?probe=" + mydata[key]["number"] + "'><input type='text' name='highalarm'>" + "<input type='submit' value='Submit'></form></td></tr>";
    displaydata = displaydata + "</table>";
  }
  document.getElementById("status").innerHTML = "DS18B20 Temperature Probes Loaded";
  document.getElementById("detailsheader").innerHTML = "<h3>Loaded Known DS18B20 Temperature Probes<h3>";
  document.getElementById("details").innerHTML = displaydata;
}
function submitFormProbes(oFormElement) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    document.getElementById("status").innerHTML = xhr.responseText;
    editProbes();
  }
  xhr.open(oFormElement.method, oFormElement.getAttribute("action"));
  xhr.send(new FormData(oFormElement));
  setTimeout(function(){
    updateHeader();
  }, 2000);
  return false;
}
</script>
</body>
</html>
)rawliteral";

const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <p><a href="/">Log Back In</a></p>
</body>
</html>
)rawliteral";

// reboot.html base upon https://gist.github.com/Joel-James/62d98e8cb3a1b6b05102
const char reboot_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
</head>
<body>
<h3>
  Rebooting, returning to main page in <span id="countdown">30</span> seconds
</h3>
<script type="text/javascript">
  var seconds = 20;
  function countdown() {
    seconds = seconds - 1;
    if (seconds < 0) {
      window.location = "/";
    } else {
      document.getElementById("countdown").innerHTML = seconds;
      window.setTimeout("countdown()", 1000);
    }
  }
  countdown();
</script>
</body>
</html>
)rawliteral";
