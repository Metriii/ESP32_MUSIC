#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

static const char *HOSTNAME = "esp32-music";
static const unsigned long WIFI_TIMEOUT_MS = 15000;

static const uint8_t BUZZER_PINS[] = {18, 19, 21};
static const size_t BUZZER_COUNT = sizeof(BUZZER_PINS) / sizeof(BUZZER_PINS[0]);
static const uint8_t BUZZER_CHANNEL = 0;
static const uint32_t TONE_HZ = 2000;
static const uint8_t PWM_BITS = 10;
static const uint32_t MAX_DUTY = 512;
static uint32_t duty = MAX_DUTY;
static uint32_t volumePercent = 100;
static bool buzzerOn = false;

static const size_t MAX_SONGS = 20;
static const size_t MAX_STEPS = 512;

static String songNames[MAX_SONGS];
static size_t songCount = 0;

static uint16_t pattern[MAX_STEPS];
static size_t patternCount = 0;

enum State { STOPPED, PLAYING, PAUSED };
static State state = STOPPED;
static size_t current = 0;
static size_t stepIndex = 0;
static unsigned long stepStart = 0;
static unsigned long pausedElapsed = 0;

static uint32_t cycleTotal = 0;
static size_t cycleSong = 0;

static WebServer server(80);

static void setBuzzer(bool on) {
  buzzerOn = on;
  ledcWrite(BUZZER_CHANNEL, on ? duty : 0);
}

static void setVolume(uint32_t percent) {
  if (percent > 100) percent = 100;
  volumePercent = percent;
  duty = percent * percent * MAX_DUTY / 10000;
  if (buzzerOn) ledcWrite(BUZZER_CHANNEL, duty);
}

static void loadSongList() {
  File dir = LittleFS.open("/musicas");
  if (!dir) return;
  File f = dir.openNextFile();
  while (f && songCount < MAX_SONGS) {
    String name = f.name();
    if (!f.isDirectory() && name.endsWith(".txt")) {
      songNames[songCount++] = name.substring(0, name.length() - 4);
    }
    f = dir.openNextFile();
  }
  for (size_t i = 1; i < songCount; i++) {
    for (size_t j = i; j > 0 && songNames[j] < songNames[j - 1]; j--) {
      String tmp = songNames[j];
      songNames[j] = songNames[j - 1];
      songNames[j - 1] = tmp;
    }
  }
}

static void addStep(uint32_t value) {
  if (patternCount < MAX_STEPS) pattern[patternCount++] = value > 65535 ? 65535 : value;
}

static bool loadPattern(size_t n) {
  File f = LittleFS.open("/musicas/" + songNames[n] + ".txt", "r");
  if (!f) return false;
  patternCount = 0;
  uint32_t value = 0;
  bool inNumber = false;
  while (f.available()) {
    char c = f.read();
    if (c >= '0' && c <= '9') {
      value = value * 10 + (c - '0');
      inNumber = true;
    } else if (inNumber) {
      addStep(value);
      value = 0;
      inNumber = false;
    }
  }
  if (inNumber) addStep(value);
  f.close();
  patternCount -= patternCount % 2;
  return patternCount > 0;
}

static void playSong(size_t n) {
  if (n >= songCount) return;
  if (state == PAUSED && n == current) {
    stepStart = millis() - pausedElapsed;
  } else {
    if (!loadPattern(n)) return;
    current = n;
    stepIndex = 0;
    stepStart = millis();
  }
  state = PLAYING;
  setBuzzer(stepIndex % 2 == 0);
}

static void pauseSong() {
  if (state != PLAYING) return;
  pausedElapsed = millis() - stepStart;
  state = PAUSED;
  setBuzzer(false);
}

static void updatePlayer() {
  if (state != PLAYING) return;
  if (millis() - stepStart < pattern[stepIndex]) return;
  stepIndex = (stepIndex + 1) % patternCount;
  stepStart = millis();
  if (stepIndex == 0) {
    cycleTotal++;
    cycleSong = current;
  }
  setBuzzer(stepIndex % 2 == 0);
}

static String jsonText(const String &text) {
  String out = "\"";
  for (size_t i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == '"' || c == '\\') out += '\\';
    out += c;
  }
  out += "\"";
  return out;
}

static const char *stateName() {
  if (state == PLAYING) return "tocando";
  if (state == PAUSED) return "pausado";
  return "parado";
}

static String stateJson() {
  String json = "{\"estado\":\"";
  json += stateName();
  json += "\",\"musica\":";
  json += state == STOPPED ? -1 : (int)current;
  json += ",\"volume\":";
  json += volumePercent;
  json += ",\"ciclos\":";
  json += cycleTotal;
  json += ",\"cicloMusica\":";
  json += (int)cycleSong;
  json += "}";
  return json;
}

static void sendJson(const String &json) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

static void sendFile(const char *path, const char *type) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    server.send(404, "text/plain", "Arquivo nao encontrado");
    return;
  }
  server.streamFile(f, type);
  f.close();
}

static void handleIndex() {
  sendFile("/web/index.html", "text/html; charset=utf-8");
}

static void handleStyle() {
  sendFile("/web/style.css", "text/css; charset=utf-8");
}

static void handleSongs() {
  String json = "[";
  for (size_t i = 0; i < songCount; i++) {
    if (i > 0) json += ",";
    json += jsonText(songNames[i]);
  }
  json += "]";
  sendJson(json);
}

static void handleState() {
  sendJson(stateJson());
}

static void handlePlay() {
  if (!server.hasArg("n")) {
    server.send(400, "application/json", "{\"erro\":\"parametro n ausente\"}");
    return;
  }
  playSong(strtoul(server.arg("n").c_str(), nullptr, 10));
  sendJson(stateJson());
}

static void handlePause() {
  pauseSong();
  sendJson(stateJson());
}

static void handleVolume() {
  if (!server.hasArg("v")) {
    server.send(400, "application/json", "{\"erro\":\"parametro v ausente\"}");
    return;
  }
  setVolume(strtoul(server.arg("v").c_str(), nullptr, 10));
  sendJson(stateJson());
}

static void handleNotFound() {
  server.send(404, "text/plain", "Nao encontrado");
}

static void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) delay(250);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi conectado. Acesse http://%s.local ou http://%s\n", HOSTNAME, WiFi.localIP().toString().c_str());
  } else {
    Serial.println("Sem Wi-Fi: confira WIFI_SSID e WIFI_PASSWORD em src/secrets.h");
  }
}

static void startServer() {
  MDNS.begin(HOSTNAME);
  server.on("/", HTTP_GET, handleIndex);
  server.on("/style.css", HTTP_GET, handleStyle);
  server.on("/api/musicas", HTTP_GET, handleSongs);
  server.on("/api/estado", HTTP_GET, handleState);
  server.on("/api/tocar", HTTP_POST, handlePlay);
  server.on("/api/pausar", HTTP_POST, handlePause);
  server.on("/api/volume", HTTP_POST, handleVolume);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  ledcSetup(BUZZER_CHANNEL, TONE_HZ, PWM_BITS);
  for (size_t i = 0; i < BUZZER_COUNT; i++) ledcAttachPin(BUZZER_PINS[i], BUZZER_CHANNEL);
  setBuzzer(false);
  if (LittleFS.begin(true)) loadSongList();
  connectWifi();
  startServer();
}

void loop() {
  server.handleClient();
  updatePlayer();
}
