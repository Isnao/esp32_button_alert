#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <WiFi.h>
#include "LittleFS.h"

#define BTN_PIN 32
#define LED_PIN 23

AsyncWebServer server(80);

struct Config {
  String ssid;
  String pass;
  String ip;
  String gateway;
  String message;
  String tel1;
  String key1;
  String tel2;
  String key2;
  String tel3;
  String key3;
  String tel4;
  String key4;
};

Config config;

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);
IPAddress dns(8, 8, 8, 8);

unsigned long previousMillis = 0;
const long interval = 10000;

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
}

void loadConfiguration(fs::FS &fs, Config& config) {
  File file = fs.open("/config.txt");
  if(!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  // Allocate a temporary JsonDocument
  JsonDocument doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  config.ssid = doc["ssid"] | "";
  config.pass = doc["pass"] | "";
  config.ip = doc["ip"] | "192.168.1.184";
  config.gateway = doc["gateway"] | "192.168.1.1";
  config.message = doc["message"] | "Alerte hypo";
  config.tel1 = doc["tel1"] | "";
  config.key1 = doc["key1"] | "";
  config.tel2 = doc["tel2"] | "";
  config.key2 = doc["key2"] | "";
  config.tel3 = doc["tel3"] | "";
  config.key3 = doc["key3"] | "";
  config.tel4 = doc["tel4"] | "";
  config.key4 = doc["key4"] | "";

  file.close();
}

void saveConfiguration(fs::FS &fs, Config& config) {
  fs.remove("/config.txt");

  File file = fs.open("/config.txt", FILE_WRITE);
  if(!file || file.isDirectory()) {
    Serial.println("- failed to create file");
    return;
  }

  // Allocate a temporary JsonDocument
  JsonDocument doc;

  doc["ssid"] = config.ssid;
  doc["pass"] = config.pass;
  doc["ip"] = config.ip;
  doc["gateway"] = config.gateway;
  doc["message"] = config.message;
  doc["tel1"] = config.tel1;
  doc["key1"] = config.key1;
  doc["tel2"] = config.tel2;
  doc["key2"] = config.key2;
  doc["tel3"] = config.tel3;
  doc["key3"] = config.key3;
  doc["tel4"] = config.tel4;
  doc["key4"] = config.key4;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  file.close();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  ESP.restart();
}

// Initialize WiFi
bool initWiFi() {
  if(config.ssid=="" || config.ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(config.ip.c_str());
  localGateway.fromString(config.gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet, dns)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(config.ssid.c_str(), config.pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  Serial.println(WiFi.localIP());
  return true;
}

void sendMessage(String phoneNumber, String apiKey, String message){
  // Data to send with HTTP POST
  String url = "http://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode != 200){
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(http.errorToString(httpResponseCode));
  }

  // Free resources
  http.end();
}

const int DEBOUNCE_DELAY = 50;
int lastSteadyState = LOW;
int lastFlickerableState = LOW;
int currentState;
unsigned long lastDebounceTime = 0;

void setup() {
  Serial.begin(115200);

  initLittleFS();

  loadConfiguration(LittleFS, config);

  if(!initWiFi()) {
    WiFi.softAP("ACADIA-BUTTON-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  }

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->addHeader("Server", "ACADIA Button Manager");
    response->print("<!DOCTYPE html><html><head><title>ACADIA Button Manager</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"></head><body><div class=\"topnav\"><h1>ACADIA Button Manager</h1></div><div class=\"content\"><div class=\"card-grid\"><div class=\"card\"><form action=\"/\" method=\"POST\"><p><label for=\"ssid\">SSID</label>");
    response->printf("<input type=\"text\" id =\"ssid\" name=\"ssid\" value=\"%s\"><br>", config.ssid.c_str());
    response->printf("<label for=\"pass\">Password</label><input type=\"text\" id =\"pass\" name=\"pass\" value=\"%s\"><br>", config.pass.c_str());
    response->printf("<label for=\"ip\">Adresse IP</label><input type=\"text\" id =\"ip\" name=\"ip\" value=\"%s\"><br>", config.ip.c_str());
    response->printf("<label for=\"gateway\">Adresse Gateway</label><input type=\"text\" id =\"gateway\" name=\"gateway\" value=\"%s\"><br>", config.gateway.c_str());
    response->printf("<label for=\"message\">Message a envoyer</label><input type=\"text\" id =\"message\" name=\"message\" value=\"%s\"><br><br>", config.message.c_str());
    response->printf("<label for=\"tel1\">Numero de tel 1</label><input type=\"text\" id =\"tel1\" name=\"tel1\" placeholder=\"ex: +33123123123\" value=\"%s\"><br>", config.tel1.c_str());
    response->printf("<label for=\"key1\">Clef API Whatsapp 1</label><input type=\"text\" id =\"key1\" name=\"key1\" value=\"%s\"><br>", config.key1.c_str());
    response->printf("<label for=\"tel2\">Numero de tel 2</label><input type=\"text\" id =\"tel2\" name=\"tel2\" placeholder=\"ex: +33123123123\" value=\"%s\"><br>", config.tel2.c_str());
    response->printf("<label for=\"key2\">Clef API Whatsapp 2</label><input type=\"text\" id =\"key2\" name=\"key2\" value=\"%s\"><br>", config.key2.c_str());
    response->printf("<label for=\"tel3\">Numero de tel 3</label><input type=\"text\" id =\"tel3\" name=\"tel3\" placeholder=\"ex: +33123123123\" value=\"%s\"><br>", config.tel3.c_str());
    response->printf("<label for=\"key3\">Clef API Whatsapp 3</label><input type=\"text\" id =\"key3\" name=\"key3\" value=\"%s\"><br>", config.key3.c_str());
    response->printf("<label for=\"tel4\">Numero de tel 4</label><input type=\"text\" id =\"tel4\" name=\"tel4\" placeholder=\"ex: +33123123123\" value=\"%s\"><br>", config.tel4.c_str());
    response->printf("<label for=\"key4\">Clef API Whatsapp 4</label><input type=\"text\" id =\"key4\" name=\"key4\" value=\"%s\"><br>", config.key4.c_str());
    response->print("<input type =\"submit\" value =\"Submit\"></p></form></div></div></div></body></html>");
    request->send(response);
  });
  
  server.serveStatic("/", LittleFS, "/");
  
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      const AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST ssid value
        if (p->name() == "ssid") {
          config.ssid = p->value().c_str();
        }
        // HTTP POST pass value
        if (p->name() == "pass") {
          config.pass = p->value().c_str();
        }
        // HTTP POST message value
        if (p->name() == "message") {
          config.message = p->value().c_str();
        }
        if (p->name() == "tel1") {
          config.tel1 = p->value().c_str();
        }
        if (p->name() == "key1") {
          config.key1 = p->value().c_str();
        }
        if (p->name() == "tel2") {
          config.tel2 = p->value().c_str();
        }
        if (p->name() == "key2") {
          config.key2 = p->value().c_str();
        }
        if (p->name() =="tel3") {
          config.tel3 = p->value().c_str();
        }
        if (p->name() == "key3") {
          config.key3 = p->value().c_str();
        }
        if (p->name() == "tel4") {
          config.tel4 = p->value().c_str();
        }
        if (p->name() == "key4") {
          config.key4 = p->value().c_str();
        }
        if (p->name() == "ip") {
          config.ip = p->value().c_str();
        }
        if (p->name() == "gateway") {
          config.gateway = p->value().c_str();
        }
      }
    }
    saveConfiguration(LittleFS, config);
    request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + config.ip);
    delay(10000);
    ESP.restart();
  });
  server.begin();

  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  currentState = digitalRead(BTN_PIN);
  if (currentState != lastFlickerableState) {
    lastDebounceTime = millis();
    lastFlickerableState = currentState;
  }
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if(lastSteadyState == HIGH && currentState == LOW)
    {
      digitalWrite(LED_PIN, HIGH);
      if(config.tel1!="" && config.key1!="") {
        sendMessage(config.tel1, config.key1, config.message);
      }
      if(config.tel2!="" && config.key2!="") {
        sendMessage(config.tel2, config.key2, config.message);
      }
      if(config.tel3!="" && config.key3!="") {
        sendMessage(config.tel3, config.key3, config.message);
      }
      if(config.tel4!="" && config.key4!="") {
        sendMessage(config.tel4, config.key4, config.message);
      }
    } else if(lastSteadyState == LOW && currentState == HIGH) {
      delay(2000);
      digitalWrite(LED_PIN, LOW);
    }
    lastSteadyState = currentState;
  }
}