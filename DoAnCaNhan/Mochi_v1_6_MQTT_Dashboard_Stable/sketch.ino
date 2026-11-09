#include <Arduino.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "MochiTypes.h"
#include "MochiDisplay.h"

// Step 4B: Wokwi, offline. OLED SDA=21, SCL=22.
constexpr uint8_t DHT_PIN = 27;
constexpr uint8_t LDR_PIN = 35;
constexpr uint8_t TOUCH_PIN = 25;
constexpr uint8_t SOUND_PIN = 34; // Wokwi potentiometer: simulated sound only
constexpr uint8_t MODE_PIN = 32;  // HIGH=PET, LOW=MONITOR (switch 3V3/GND)
constexpr uint32_t SENSOR_MS = 2000;
constexpr uint32_t MODE_DEBOUNCE_MS = 50;
constexpr uint32_t MONITOR_REFRESH_MS = 350;
constexpr uint32_t LIGHT_MS = 100;
constexpr uint32_t INPUT_MS = 10;
constexpr uint32_t STATUS_MS = 2000;
constexpr uint32_t TOUCH_DEBOUNCE_MS = 40;
constexpr uint32_t TOUCH_REACTION_MS = 2000;
constexpr uint32_t ANGER_REACTION_MS = 2000;
constexpr uint32_t MULTITAP_WINDOW_MS = 2000;
constexpr uint8_t ANGER_TAP_COUNT = 3;
constexpr uint32_t SOUND_REACTION_MS = 1000;
constexpr uint32_t YAWN_MS = 2000;
constexpr uint32_t DARK_REENTRY_MS = 10000;
constexpr uint32_t SOUND_COOLDOWN_MS = 1500;
constexpr int ADC_DARK = 4063;
constexpr int ADC_BRIGHT = 32;
constexpr int SOUND_TRIGGER_ADC = 3000; // Tune for Wokwi potentiometer

DHTesp dht;
DisplayState temperatureState = STATE_NORMAL;
DisplayState faceState = STATE_NORMAL;
bool sleeping = false; // required by MochiDisplay.cpp
bool dark = false;
float brightness = 100.0f;
int lightRaw = 0;
int soundRaw = 0;
float temperatureC = NAN, humidityPct = NAN;
bool dhtOk = false;
enum class OperatingMode : uint8_t { PET, MONITOR };
OperatingMode mode = OperatingMode::PET;
bool modeRawHigh = true;
uint32_t modeChangedAt = 0, lastMonitorAt = 0;
uint8_t monitorPage = 0;
bool monitorTouchReady = false;
bool monitorDirty = true;

enum class SleepPhase : uint8_t { AWAKE, REACTION, WAIT_DARK, YAWNING, ASLEEP };
SleepPhase phase = SleepPhase::AWAKE;
uint32_t phaseSince = 0;
uint32_t reactionDuration = 0;
DisplayState reactionFace = STATE_TOUCH;
uint32_t lastDhtAt = 0, lastLightAt = 0, lastInputAt = 0, lastStatusAt = 0;
uint32_t touchChangedAt = 0, lastSoundEventAt = 0;
bool touchRawPrevious = false, touchStable = false, soundArmed = true;
uint32_t tapWindowStartedAt = 0;
uint8_t tapCount = 0;
bool angerLocked = false; // prevent continuous taps from extending ANGER

const char* nameOf(DisplayState s) {
  switch(s) {
    case STATE_NORMAL: return "NORMAL";
    case STATE_HOT: return "HOT";
    case STATE_COLD: return "COLD";
    case STATE_SLEEP: return "SLEEP";
    case STATE_YAWN: return "YAWN";
    case STATE_TOUCH: return "TOUCH";
    case STATE_ANGER: return "ANGER";
    case STATE_SURPRISE: return "SURPRISE";
    default: return "OTHER";
  }
}
const char* phaseName() {
  switch (phase) {
    case SleepPhase::AWAKE: return "AWAKE";
    case SleepPhase::REACTION: return "REACTION";
    case SleepPhase::WAIT_DARK: return "WAIT_DARK";
    case SleepPhase::YAWNING: return "YAWNING";
    case SleepPhase::ASLEEP: return "ASLEEP";
  }
  return "?";
}
DisplayState classifyTemperature(float t, DisplayState prev) {
  if (t >= 32.0f) return STATE_HOT;
  if (t <= 18.0f) return STATE_COLD;
  if (prev == STATE_HOT && t >= 31.0f) return STATE_HOT;
  if (prev == STATE_COLD && t <= 19.0f) return STATE_COLD;
  return STATE_NORMAL;
}
void showFace(DisplayState desired) {
  if (faceState != desired) {
    Serial.printf("FACE %s -> %s\n", nameOf(faceState), nameOf(desired));
    faceState = desired;
  }
  updateMochiFace(desired);
}
void setSleeping(bool value) {
  if (sleeping == value) return;
  sleeping = value;
  refreshMochiFace(); // same face must be reapplied if eye-closure policy changed
}
void changePhase(SleepPhase next, uint32_t now) {
  if (phase == next) return;
  phase = next;
  phaseSince = now;
  Serial.printf("[%lu ms] PHASE -> %s\n", (unsigned long)now, phaseName());
}
void readDht() {
  auto data = dht.getTempAndHumidity();
  if (isnan(data.temperature) || isnan(data.humidity)) {
    dhtOk = false;
    Serial.println("DHT read failed; preserving previous temperature state");
    return;
  }
  temperatureC = data.temperature;
  humidityPct = data.humidity;
  dhtOk = true;
  DisplayState next = classifyTemperature(data.temperature, temperatureState);
  if (next != temperatureState) {
    Serial.printf("TEMP %s -> %s (%.1f C)\n", nameOf(temperatureState), nameOf(next), data.temperature);
    temperatureState = next;
    setMochiTemperatureEffect(next == STATE_HOT ? TEMP_EFFECT_HOT :
      (next == STATE_COLD ? TEMP_EFFECT_COLD : TEMP_EFFECT_NONE));
  }
}
void readLight(uint32_t now) {
  lightRaw = analogRead(LDR_PIN);
  brightness = constrain((ADC_DARK - lightRaw) * 100.0f / (ADC_DARK - ADC_BRIGHT), 0.0f, 100.0f);
  bool next = dark;
  if (!dark && brightness <= 20.0f) next = true;
  else if (dark && brightness > 25.0f) next = false;
  if (next == dark) return;
  dark = next;
  Serial.printf("[%lu ms] LIGHT -> %s | adc=%d | brightness=%.1f%%\n", (unsigned long)now, dark ? "DARK" : "BRIGHT", lightRaw, brightness);
  if (mode == OperatingMode::MONITOR) return;
  if (!dark) {
    setSleeping(false);
    changePhase(SleepPhase::AWAKE, now);
  } else if (phase == SleepPhase::AWAKE) {
    // First entry into darkness: yawn immediately (no initial 10-second wait).
    changePhase(SleepPhase::YAWNING, now);
  }
}
void startReaction(DisplayState state, uint32_t duration, uint32_t now) {
  setSleeping(false);
  reactionFace = state;
  reactionDuration = duration;
  changePhase(SleepPhase::REACTION, now);
  // A new reaction replaces the previous timer; touch taps are handled separately.
  phaseSince = now;
  Serial.printf("[%lu ms] EVENT %s\n", (unsigned long)now, nameOf(state));
}
void onTouchPressed(uint32_t now) {
  // Each debounced press edge counts once; holding the button does not repeat.
  // Ignore further touches while ANGER is displayed, so its timer stays fixed.
  if (angerLocked && phase == SleepPhase::REACTION && reactionFace == STATE_ANGER) return;
  if (tapCount == 0 || now - tapWindowStartedAt > MULTITAP_WINDOW_MS) {
    tapWindowStartedAt = now;
    tapCount = 0;
  }
  ++tapCount;
  Serial.printf("[%lu ms] TAP %u/%u (window=%lu ms)\n",
                (unsigned long)now, tapCount, ANGER_TAP_COUNT,
                (unsigned long)(now - tapWindowStartedAt));
  if (tapCount >= ANGER_TAP_COUNT) {
    tapCount = 0;
    angerLocked = true;
    startReaction(STATE_ANGER, ANGER_REACTION_MS, now);
  } else {
    // Keep the first HAPPY timer; don't extend it on the second tap.
    if (!(phase == SleepPhase::REACTION && reactionFace == STATE_TOUCH)) {
      startReaction(STATE_TOUCH, TOUCH_REACTION_MS, now);
    }
  }
}
void readInputs(uint32_t now) {
  bool rawPressed = (digitalRead(TOUCH_PIN) == LOW);
  if (rawPressed != touchRawPrevious) {
    touchRawPrevious = rawPressed;
    touchChangedAt = now;
  }
  if ((now - touchChangedAt >= TOUCH_DEBOUNCE_MS) && rawPressed != touchStable) {
    touchStable = rawPressed;
    if (mode == OperatingMode::PET) {
      if (touchStable) onTouchPressed(now);
    } else {
      if (!touchStable) monitorTouchReady = true;
      else if (monitorTouchReady) {
        monitorPage ^= 1;
        monitorDirty = true;
        Serial.printf("MONITOR PAGE %u\n", monitorPage + 1);
      }
    }
  }
  soundRaw = analogRead(SOUND_PIN);
  // A potentiometer is not a microphone. Trigger only on rising threshold crossing.
  if (soundRaw < SOUND_TRIGGER_ADC - 300) soundArmed = true;
  if (soundArmed && soundRaw >= SOUND_TRIGGER_ADC &&
      (lastSoundEventAt == 0 || now - lastSoundEventAt >= SOUND_COOLDOWN_MS)) {
    soundArmed = false;
    lastSoundEventAt = now;
    if (mode == OperatingMode::PET && !angerLocked) startReaction(STATE_SURPRISE, SOUND_REACTION_MS, now);
  }
}
void progressSleep(uint32_t now) {
  switch (phase) {
    case SleepPhase::AWAKE:
      if (dark) changePhase(SleepPhase::YAWNING, now);
      break;
    case SleepPhase::REACTION:
      if (now - phaseSince >= reactionDuration) {
        if (reactionFace == STATE_ANGER) angerLocked = false;
        if (dark) changePhase(SleepPhase::WAIT_DARK, now);
        else changePhase(SleepPhase::AWAKE, now);
      }
      break;
    case SleepPhase::WAIT_DARK:
      if (!dark) changePhase(SleepPhase::AWAKE, now);
      else if (now - phaseSince >= DARK_REENTRY_MS) changePhase(SleepPhase::YAWNING, now);
      break;
    case SleepPhase::YAWNING:
      if (!dark) changePhase(SleepPhase::AWAKE, now);
      else if (now - phaseSince >= YAWN_MS) {
        setSleeping(true);
        changePhase(SleepPhase::ASLEEP, now);
      }
      break;
    case SleepPhase::ASLEEP:
      if (!dark) {
        setSleeping(false);
        changePhase(SleepPhase::AWAKE, now);
      }
      break;
  }
}
DisplayState selectFace() {
  // During sleep, temperature only changes the independent overlay.
  if (phase == SleepPhase::ASLEEP) return STATE_SLEEP;
  // Temperature is now an independent visual overlay, never an eye mood.
  if (phase == SleepPhase::YAWNING) return STATE_YAWN;
  if (phase == SleepPhase::REACTION) return reactionFace;
  return STATE_NORMAL;
}
void switchMode(OperatingMode next, uint32_t now) {
  if (next == mode) return;
  mode = next;
  tapCount = 0;
  angerLocked = false;
  soundArmed = soundRaw < SOUND_TRIGGER_ADC - 300;
  // Re-sync the debouncer with the physical touch state to prevent phantom presses.
  touchRawPrevious = (digitalRead(TOUCH_PIN) == LOW);
  touchStable = touchRawPrevious;
  touchChangedAt = now;
  monitorTouchReady = false;
  if (mode == OperatingMode::MONITOR) {
    monitorPage = 0;
    monitorDirty = true;
    lastMonitorAt = now;
    Serial.println("MODE -> MONITOR");
  } else {
    // Reset transient reactions, then re-enter the established light/sleep policy.
    setSleeping(false);
    phase = SleepPhase::AWAKE;
    phaseSince = now;
    if (dark) changePhase(SleepPhase::YAWNING, now);
    refreshMochiFace();
    Serial.println("MODE -> PET");
  }
}
void pollMode(uint32_t now) {
  bool rawHigh = digitalRead(MODE_PIN) == HIGH;
  if (rawHigh != modeRawHigh) {
    modeRawHigh = rawHigh;
    modeChangedAt = now;
  }
  if (now - modeChangedAt >= MODE_DEBOUNCE_MS)
    switchMode(rawHigh ? OperatingMode::PET : OperatingMode::MONITOR, now);
}
void renderMonitor(uint32_t now) {
  if (!monitorDirty && now - lastMonitorAt < MONITOR_REFRESH_MS) return;
  lastMonitorAt = now;
  monitorDirty = false;
  drawMochiMonitor(monitorPage, temperatureC, humidityPct, dhtOk, brightness,
                   dark, soundRaw, temperatureState == STATE_HOT ? TEMP_EFFECT_HOT :
                   (temperatureState == STATE_COLD ? TEMP_EFFECT_COLD : TEMP_EFFECT_NONE));
}

// STEP 6B-04: MQTT telemetry only. No incoming commands.
// Browser WSS: mqtt-dashboard.com:8884 (SSL enabled).
// ESP32 MQTT TCP: broker.hivemq.com:1883 (same public HiveMQ broker service).
constexpr char MQTT_HOST[] = "broker.hivemq.com";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_TOPIC[] = "mochi/v1/test84721/telemetry";
constexpr uint32_t MQTT_PUBLISH_MS = 2000;
constexpr uint32_t MQTT_RETRY_MS = 5000;
constexpr uint32_t WIFI_RETRY_MS = 10000;
WiFiClient mqttNetwork;
PubSubClient mqtt(mqttNetwork);
uint32_t lastMqttAt = 0, lastMqttRetryAt = 0, lastWifiRetryAt = 0;

void setupNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("Wokwi-GUEST", "");
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setSocketTimeout(1);
  Serial.println("MQTT WiFi connecting in background...");
}

void pollNetwork(uint32_t now) {
  if (WiFi.status() != WL_CONNECTED) {
    if (mqtt.connected()) mqtt.disconnect();
    if (now - lastWifiRetryAt >= WIFI_RETRY_MS) {
      lastWifiRetryAt = now;
      WiFi.disconnect();
      WiFi.begin("Wokwi-GUEST", "");
      Serial.println("WiFi retry");
    }
    return;
  }
  if (!mqtt.connected()) {
    if (now - lastMqttRetryAt < MQTT_RETRY_MS) return;
    lastMqttRetryAt = now;
    String clientId = "mochi-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqtt.connect(clientId.c_str())) Serial.println("MQTT CONNECTED");
    else Serial.printf("MQTT connect failed rc=%d\n", mqtt.state());
    return;
  }
  mqtt.loop();
  if (now - lastMqttAt < MQTT_PUBLISH_MS) return;
  lastMqttAt = now;
  // Numeric DHT values are null when sensor read is invalid.
  char temp[20], hum[20];
  if (dhtOk && isfinite(temperatureC) && isfinite(humidityPct)) {
    snprintf(temp, sizeof(temp), "%.1f", temperatureC);
    snprintf(hum, sizeof(hum), "%.1f", humidityPct);
  } else {
    strcpy(temp, "null");
    strcpy(hum, "null");
  }
  char payload[360];
  snprintf(payload, sizeof(payload),
    "{\"device_id\":\"mochi_esp32_01\",\"mode\":\"%s\","
    "\"temperature_c\":%s,\"humidity_pct\":%s,\"dht_ok\":%s,"
    "\"light_pct\":%.1f,\"light_adc\":%d,\"sound_adc\":%d,"
    "\"dark\":%s,\"temperature_status\":\"%s\",\"sleeping\":%s}",
    mode == OperatingMode::PET ? "PET" : "MONITOR", temp, hum,
    dhtOk ? "true" : "false", brightness, lightRaw, soundRaw,
    dark ? "true" : "false", nameOf(temperatureState), sleeping ? "true" : "false");
  if (mqtt.publish(MQTT_TOPIC, payload, false)) {
    Serial.print("MQTT PUB "); Serial.println(payload);
  } else Serial.println("MQTT publish failed");
}

void setup() {
  Serial.begin(115200);
  Serial.println("MOCHI V1 | STEP 5B | PET / MONITOR");
  pinMode(TOUCH_PIN, INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT); // switch ends wired to 3V3 and GND
  modeRawHigh = digitalRead(MODE_PIN) == HIGH;
  mode = modeRawHigh ? OperatingMode::PET : OperatingMode::MONITOR;
  analogReadResolution(12);
  dht.setup(DHT_PIN, DHTesp::DHT22);
  beginMochiDisplay();
  setupNetwork();
  readDht();
  uint32_t now = millis();
  lastDhtAt = lastLightAt = lastInputAt = lastStatusAt = now;
  readLight(now);
  if (mode == OperatingMode::PET) {
    if (dark) changePhase(SleepPhase::YAWNING, now);
    showFace(selectFace());
  } else {
    monitorTouchReady = !touchStable && digitalRead(TOUCH_PIN) == HIGH;
    renderMonitor(now);
  }
}
void loop() {
  uint32_t now = millis();
  pollMode(now);
  pollNetwork(now);
  if (now - lastDhtAt >= SENSOR_MS) { lastDhtAt = now; readDht(); }
  if (now - lastLightAt >= LIGHT_MS) { lastLightAt = now; readLight(now); }
  if (now - lastInputAt >= INPUT_MS) { lastInputAt = now; readInputs(now); }
  if (mode == OperatingMode::PET) {
    progressSleep(now);
    showFace(selectFace());
    updateMochiDisplay();
  } else {
    renderMonitor(now);
  }
  if (now - lastStatusAt >= STATUS_MS) {
    lastStatusAt = now;
    Serial.printf("STATUS mode=%s temp=%s dht=%s lightADC=%d brightness=%.1f%% dark=%d soundADC=%d touch=%d modeGPIO32=%s phase=%s face=%s sleeping=%d\n",
                  (mode == OperatingMode::PET ? "PET" : "MONITOR"), nameOf(temperatureState),
                  (dhtOk ? "OK" : "ERR"), lightRaw, brightness, dark, soundRaw, touchStable,
                  digitalRead(MODE_PIN) ? "HIGH" : "LOW", phaseName(), nameOf(faceState), sleeping);
  }
}
