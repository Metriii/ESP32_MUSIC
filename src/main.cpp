#include <Arduino.h>
#include <LittleFS.h>

static const uint8_t BUZZER_PINS[] = {18, 19, 21};
static const size_t BUZZER_COUNT = sizeof(BUZZER_PINS) / sizeof(BUZZER_PINS[0]);
static const uint8_t BUZZER_CHANNEL = 0;
static const uint32_t TONE_HZ = 2000;
static const uint8_t PWM_BITS = 10;
static const uint32_t MAX_DUTY = 512;
static uint32_t duty = MAX_DUTY;
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

static void setBuzzer(bool on) {
  buzzerOn = on;
  ledcWrite(BUZZER_CHANNEL, on ? duty : 0);
}

static void setVolume(uint32_t percent) {
  if (percent > 100) percent = 100;
  duty = percent * percent * MAX_DUTY / 10000;
  if (buzzerOn) ledcWrite(BUZZER_CHANNEL, duty);
}

static void loadSongList() {
  File dir = LittleFS.open("/");
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
  File f = LittleFS.open("/" + songNames[n] + ".txt", "r");
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

static void sendStatus() {
  if (state == PLAYING) {
    Serial.printf("S:tocando:%u\n", (unsigned)current);
  } else if (state == PAUSED) {
    Serial.printf("S:pausado:%u\n", (unsigned)current);
  } else {
    Serial.println("S:parado");
  }
}

static void sendList() {
  for (size_t i = 0; i < songCount; i++) Serial.printf("M%u:%s\n", (unsigned)i, songNames[i].c_str());
  sendStatus();
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
  if (stepIndex == 0) Serial.printf("C:%u\n", (unsigned)current);
  setBuzzer(stepIndex % 2 == 0);
}

static void readCommands() {
  static char line[16];
  static size_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') {
      if (len < sizeof(line) - 1) line[len++] = c;
      continue;
    }
    if (len == 0) continue;
    line[len] = '\0';
    len = 0;
    if (line[0] == 'L') {
      sendList();
      continue;
    }
    if (line[0] == 'W') {
      setVolume(strtoul(line + 1, nullptr, 10));
      continue;
    }
    if (line[0] == 'T') {
      playSong(strtoul(line + 1, nullptr, 10));
    } else if (line[0] == 'P') {
      pauseSong();
    }
    sendStatus();
  }
}

void setup() {
  Serial.begin(115200);
  ledcSetup(BUZZER_CHANNEL, TONE_HZ, PWM_BITS);
  for (size_t i = 0; i < BUZZER_COUNT; i++) ledcAttachPin(BUZZER_PINS[i], BUZZER_CHANNEL);
  setBuzzer(false);
  if (LittleFS.begin(true)) loadSongList();
  sendList();
}

void loop() {
  readCommands();
  updatePlayer();
}
