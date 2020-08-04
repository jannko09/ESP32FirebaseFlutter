#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <FirebaseESP32.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>

void printResult(FirebaseData &data);
void updateDatabaseRules(FirebaseData &dataObj);

//Replace your SSID below
const char *ssid = "InsertYourNetworkNameHere";
//Replace your network password below
const char *password = "InsertYourNetworkPasswordHere";
const char *PARAM_USER = "devicename";
const char *PARAM_SECRET = "devicesecret";


String path = "/ESP32_Device";
String wifiMacString = WiFi.macAddress();
String parsedAddress;

int hours;
int minutes;
int seconds;

float prev_temp;
float prev_humidity;

#define DHTPIN 4
#define DHTTYPE DHT11 // DHT 11
#define FIREBASE_HOST "FirebaseHost"
#define FIREBASE_AUTH "FirebaseDatabaseAUTH"

FirebaseData firebaseData;
FirebaseJson json;

DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}


String readDHTTemperature()
{

  float t = dht.readTemperature();
  if (isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else
  {
    Serial.println(t);
    if (prev_temp != t)
    {
      prev_temp = t;
      Firebase.setDouble(firebaseData, path + "/" + parsedAddress + "/Temperature/", t);
    }
    return String(t);
  }
}

String readDHTHumidity()
{
  float h = dht.readHumidity();
  if (isnan(h))
  {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else
  {
    Serial.println(h);

    if (prev_humidity != h)
    {
      prev_humidity = h;
      Firebase.setDouble(firebaseData, path + "/" + parsedAddress + "/Humidity/", h);
    }
    return String(h);
  }
}

void parseAddress(String address)
{
  Serial.println(address);
  String parsedPath = address;
  parsedPath.replace(":", "");
  parsedAddress = parsedPath;
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    body {
      width: 100%;
      height: 100%;
      box-sizing: border-box;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    } 
    input {
      padding: 10px;
    }
    .detail__content {
      display: flex;
      flex-direction: column;
      justify-content: space-between;
    }
  </style>
</head>

<body>
  <h2>ESP32 Sauna sensor</h2>
   <section class="detail__content">
   <h3>Device details : </h3>
   <h4><i class="fas fa-desktop"></i> %devicename%</h4>
   <h4><i class="fas fa-key"></i> %devicesecret%</h4>
   <h4><i class="fas fa-wifi"></i> %connectednetwork%</h4>
   <h4><i class="far fa-address-card"></i> %macaddress%</h4>
   </section>
   <br>
    <section class="fields__content">
    <ul class="fields__content-sensors">
    
    </ul>

   </section>
   <br>

 <form action="/user">
  <label for="dname">Device name:</label><br>
   <input type="text" id="dname" name="devicename" placeholder="%devicename%"><br>
  <label for="dsecret">Device secret:</label><br>
 <input type="text" id="dsec" name="devicesecret" placeholder="%devicesecret%"><br><br>
  <input type="submit" value="Add.." onclick="submitMessage()>
  <iframe style="display:none" name="hidden-form"></iframe>
</form> 

<iframe style="display:none" name="hidden-form"></iframe>
  <br>
  
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
</body>
<script>
  function submitMessage() {
    alert("Device saved");
    setTimeout(function(){ document.location.reload(false); }, 500);
  }
</script>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- write failed");
  }
}
String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

void setUserCredentials()
{
  String _userName = readFile(SPIFFS, "/inputString.txt");
  String _userSecret = readFile(SPIFFS, "/inputSecret.txt");
  _userName != "" ? Firebase.setString(firebaseData, path + "/" + parsedAddress + "/name", _userName) : Serial.print("No Name");
  _userSecret != "" ? Firebase.setString(firebaseData, path + "/" + parsedAddress + "/secret", _userSecret) : Serial.print("No Secret");
};

String processor(const String &var)
{
  if (var == "macaddress")
  {
    return WiFi.macAddress();
  }
  else if (var == "devicename")
  {
    return readFile(SPIFFS, "/inputString.txt");
  }
  else if (var == "devicesecret")
  {
    return readFile(SPIFFS, "/inputSecret.txt");
  }
  else if (var == "TEMPERATURE")
  {
    return readDHTTemperature();
  }
  else if (var == "HUMIDITY")
  {
    return readDHTHumidity();
  }
  else if (var == "connectednetwork")
  {
    return ssid;
  }
  return String();
}

void setup()
{
  Serial.begin(9600);
  SPIFFS.begin(true);
  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(" . ");
  }

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  Serial.println(WiFi.localIP());

  String parsedAddress = wifiMacString;
  parseAddress(parsedAddress);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  server.on("/user", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputName;
    String inputSecret;

    if (request->hasParam(PARAM_USER))
    {
      inputName = request->getParam(PARAM_USER)->value();
      writeFile(SPIFFS, "/inputString.txt", inputName.c_str());
    }
    if (request->hasParam(PARAM_SECRET))
    {
      inputSecret = request->getParam(PARAM_SECRET)->value();
      writeFile(SPIFFS, "/inputSecret.txt", inputSecret.c_str());
    }
    request->send(200, "text/html", "New field (" + inputName + ") created with value: " + inputSecret + "<br>" + "<br><a href=\"/\">Return to Home Page</a>");
    setUserCredentials();
  });

  server.onNotFound(notFound);
  server.begin();
  setUserCredentials();
}

void loop()
{
}