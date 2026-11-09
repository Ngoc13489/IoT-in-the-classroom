#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include "MochiTypes.h"
#include "MochiDisplay.h"

// Subclass intercepts the single display() call inside RoboEyes::drawEyes().
// RoboEyes is a template instantiated with this *derived* type, so its
// display->display() resolves to MochiSSD1306::display() (no virtual needed).
static TemperatureEffect temperatureEffect = TEMP_EFFECT_NONE;
static bool monitorRendering = false;
// COLD CUSTOM: user-selected design, based on Design Lab style 1.
// 2 chevrons per side; 8px tall; 10px vertical spacing; 6px from edges.
// 160ms motion period; ice crystals remain stationary to avoid visual noise.
static constexpr uint8_t COLD_MARKS_PER_SIDE = 2;
static constexpr uint8_t COLD_MARK_HEIGHT_PX = 8;
static constexpr uint8_t COLD_MARK_SPACING_PX = 10;
static constexpr uint8_t COLD_EDGE_OFFSET_PX = 6;
static constexpr uint32_t COLD_FRAME_INTERVAL_MS = 160;

// Exact monochrome pixel silhouette cropped from user-provided snow_pisxel.png.
// Original image: 32x32; non-black bounding box: x=11..17, y=7..17.
// Bitmap data uses Adafruit_GFX MSB-first horizontal packing.
static const uint8_t PROGMEM COLD_SNOWFLAKE_BITMAP[] = {
  0x10, 0x54, 0x38, 0xFE, 0x38, 0x54, 0x10
};
static constexpr int COLD_SNOWFLAKE_W = 7;
static constexpr int COLD_SNOWFLAKE_H = 7;
static void drawUserSnowflake(Adafruit_SSD1306 &canvas, int x, int y) {
  canvas.drawBitmap(x, y, COLD_SNOWFLAKE_BITMAP,
                    COLD_SNOWFLAKE_W, COLD_SNOWFLAKE_H, SSD1306_WHITE);
}

static void drawColdOverlay(Adafruit_SSD1306 &canvas, uint32_t now) {
  // Overlay is inserted before the existing RoboEyes display flush.
  const uint8_t phase = (now / COLD_FRAME_INTERVAL_MS) % 4U;
  const int8_t dx[4] = {0, 1, 0, -1};
  const int8_t dy[4] = {0, 1, 0, -1};
  const int half = COLD_MARK_HEIGHT_PX / 2;
  // Center the pair vertically; spacing is between the marks' top edges.
  const int firstY = (SCREEN_HEIGHT -
      ((COLD_MARKS_PER_SIDE - 1) * COLD_MARK_SPACING_PX + COLD_MARK_HEIGHT_PX)) / 2;
  for (int i = 0; i < COLD_MARKS_PER_SIDE; ++i) {
    const int y = firstY + i * COLD_MARK_SPACING_PX + dy[(phase + i) % 4];
    const int lx = COLD_EDGE_OFFSET_PX + dx[(phase + i) % 4];
    const int rx = SCREEN_WIDTH - 1 - COLD_EDGE_OFFSET_PX - dx[(phase + i) % 4];
    canvas.drawLine(lx, y, lx + half, y + half, SSD1306_WHITE);
    canvas.drawLine(lx + half, y + half, lx, y + COLD_MARK_HEIGHT_PX, SSD1306_WHITE);
    canvas.drawLine(rx, y, rx - half, y + half, SSD1306_WHITE);
    canvas.drawLine(rx - half, y + half, rx, y + COLD_MARK_HEIGHT_PX, SSD1306_WHITE);
  }
  // Ice crystals do not drift; only the chevrons animate.
  drawUserSnowflake(canvas, 10, 5);
  drawUserSnowflake(canvas, 111, 48);
}

class MochiSSD1306 : public Adafruit_SSD1306 {
public:
  MochiSSD1306(uint8_t w, uint8_t h, TwoWire *wire, int8_t rst)
      : Adafruit_SSD1306(w, h, wire, rst) {}
  void display() {
    // Called once after RoboEyes has drawn the eyes into the framebuffer.
    if (!monitorRendering && temperatureEffect == TEMP_EFFECT_COLD) drawColdOverlay(*this, millis());
    Adafruit_SSD1306::display();
  }
};
MochiSSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<MochiSSD1306> roboEyes(display);

static DisplayState currentDisplayState = STATE_NORMAL;
static DisplayState lastRenderedState = STATE_NORMAL;
static bool firstFaceUpdate = true;
static bool squintEyes = false;
static bool sleepTransition = false;
static unsigned long sleepTransitionElapsed = 0;
static unsigned long yawnStartTime = 0;
static constexpr unsigned long YAWN_DURATION_MS = 2000;
static constexpr uint8_t YAWN_START_HEIGHT = 30;
static constexpr uint8_t YAWN_END_HEIGHT = 1;

// Provided by main.cpp. Reserved for later sleep-flow integration.
extern bool sleeping;

struct FaceConfig {
  uint8_t eyeWidth, eyeHeight, eyeRadius, eyeGap, mood;
  bool sweat, horizontalFlicker, verticalFlicker, confused, eyesClosed;
};

// Must match DisplayState order in MochiTypes.h.
static const FaceConfig FACE_CONFIGS[] = {
  {30,30,8,12,ANGRY,   false,false,false,false,false}, // ERROR
  {30,30,8,12,TIRED,   true, false,false,false,false}, // HOT
  {30,30,8,12,TIRED,   false,true, true, false,false}, // COLD
  {30,10,5,12,DEFAULT, false,false,false,false,true }, // SLEEP
  {30,30,8,12,HAPPY,   false,false,false,false,false}, // TOUCH
  {36,36,14,10,DEFAULT,false,false,false,true, false}, // SURPRISE
  {30,30,8,12,ANGRY,   false,false,false,false,false}, // ANGER
  {30,20,5,12,TIRED,   false,false,false,false,false}, // YAWN
  {30,30,8,12,DEFAULT,false,false,false,false,false}, // MEAL
  {30,20,5,12,TIRED,   false,false,false,false,false}, // BEDTIME
  {30,30,8,12,HAPPY,   false,false,false,false,false}, // MUSIC (legacy)
  {30,20,5,12,TIRED,   false,false,false,false,false}, // DARK
  {30,30,8,12,DEFAULT, false,false,false,false,false}  // NORMAL
};
static constexpr size_t FACE_CONFIG_COUNT = sizeof(FACE_CONFIGS) / sizeof(FACE_CONFIGS[0]);

void refreshMochiFace() { firstFaceUpdate = true; }
void setMochiTemperatureEffect(TemperatureEffect effect) {
  if (temperatureEffect == effect) return;
  temperatureEffect = effect;
  // Built-in RoboEyes sweat is drawn in its own framebuffer/frame.
  // Cold marks are added by MochiSSD1306 just before that frame is sent.
  roboEyes.setSweat(effect == TEMP_EFFECT_HOT ? ON : OFF);
  Serial.printf("EFFECT -> %s\n", effect == TEMP_EFFECT_HOT ? "HOT_SWEAT" :
    effect == TEMP_EFFECT_COLD ? "COLD_SHIVER" : "NONE");
}
void setMochiSquint(bool enabled) {
  if (squintEyes != enabled) {
    squintEyes = enabled;
    firstFaceUpdate = true;
  }
}
void setMochiSleepTransition(bool enabled, unsigned long elapsedMs) {
  sleepTransition = enabled;
  sleepTransitionElapsed = elapsedMs;
  firstFaceUpdate = true;
}
void beginMochiDisplay() {
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED initialization failed at address 0x3C."));
    return;
  }
  Serial.println(F("OLED init OK (0x3C)"));
  display.clearDisplay();
  display.display();
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 30);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
}
void updateMochiDisplay() {
  monitorRendering = false;
  if (currentDisplayState == STATE_YAWN && !sleepTransition) {
    unsigned long elapsed = millis() - yawnStartTime;
    if (elapsed > YAWN_DURATION_MS) elapsed = YAWN_DURATION_MS;
    uint8_t height = YAWN_START_HEIGHT -
      ((YAWN_START_HEIGHT - YAWN_END_HEIGHT) * elapsed / YAWN_DURATION_MS);
    roboEyes.setHeight(height, height);
    if (elapsed >= YAWN_DURATION_MS) roboEyes.close();
  }
  if (sleepTransition) {
    const unsigned long t = sleepTransitionElapsed;
    uint8_t h = 30;
    if (t < 450) h = (t % 450 < 120) ? 5 : 30;
    else if (t < 1150) { unsigned long p = (t - 450) % 700; h = (p < 180) ? 5 : 30; }
    else if (t < 2100) { unsigned long p = (t - 1150) % 950; h = (p < 250) ? 4 : 30; }
    else if (t < 3000) { unsigned long p = (t - 2100) % 900; h = (p < 330) ? 3 : 30; }
    else h = 1;
    roboEyes.setHeight(h, h);
    if (t >= 3000) roboEyes.close();
  }
  roboEyes.update(); // exactly one OLED transfer per RoboEyes animation frame
}
void drawMochiExtras(DisplayState state) {
  if (state == STATE_ANGER) {
    const int x = 112, y = 9;
    display.drawLine(x-2,y-7,x+2,y-7,SSD1306_WHITE);
    display.drawLine(x+2,y-7,x+2,y-3,SSD1306_WHITE);
    display.drawLine(x+2,y-3,x+5,y-3,SSD1306_WHITE);
    display.drawLine(x+5,y-3,x+5,y+1,SSD1306_WHITE);
    display.drawLine(x+5,y+1,x+2,y+1,SSD1306_WHITE);
    display.drawLine(x+2,y+1,x+2,y+5,SSD1306_WHITE);
    display.drawLine(x+2,y+5,x-2,y+5,SSD1306_WHITE);
    display.drawLine(x-2,y+5,x-2,y+1,SSD1306_WHITE);
    display.drawLine(x-2,y+1,x-5,y+1,SSD1306_WHITE);
    display.drawLine(x-5,y+1,x-5,y-3,SSD1306_WHITE);
    if ((millis()/250)%2 == 0) display.fillRect(x-1,y-1,3,3,SSD1306_WHITE);
  }
  // No extra OLED flush: this legacy helper is not used in step 4D.
}
void updateMochiFace(DisplayState state) {
  if (state == STATE_HOT || state == STATE_COLD) state = STATE_NORMAL;
  int index = static_cast<int>(state);
  if (index < 0 || static_cast<size_t>(index) >= FACE_CONFIG_COUNT) return;
  if (!firstFaceUpdate && state == lastRenderedState) return;
  if (state != currentDisplayState) {
    currentDisplayState = state;
    if (state == STATE_YAWN) yawnStartTime = millis();
  }
  firstFaceUpdate = false;
  lastRenderedState = state;
  const FaceConfig& face = FACE_CONFIGS[index];
  roboEyes.setWidth(face.eyeWidth, face.eyeWidth);
  uint8_t eyeHeight = face.eyeHeight;
  if (squintEyes && !face.eyesClosed && state != STATE_SLEEP &&
      state != STATE_YAWN && !sleepTransition) eyeHeight = 12;
  roboEyes.setHeight(eyeHeight, eyeHeight);
  roboEyes.setBorderradius(face.eyeRadius, face.eyeRadius);
  roboEyes.setSpacebetween(face.eyeGap);
  roboEyes.setPosition(DEFAULT);
  roboEyes.setSweat(temperatureEffect == TEMP_EFFECT_HOT ? ON : OFF);
  roboEyes.setHFlicker(OFF, 0);
  roboEyes.setVFlicker(OFF, 0);
  roboEyes.setCuriosity(OFF);
  roboEyes.setCyclops(OFF);
  roboEyes.setMood(face.mood);
  // HOT uses built-in sweat, COLD uses side marks; neither changes eye mood.
  if (face.horizontalFlicker) roboEyes.setHFlicker(ON, 2);
  if (face.verticalFlicker) roboEyes.setVFlicker(ON, 1);
  if (face.confused) roboEyes.anim_confused();
  bool forceClosed = face.eyesClosed ||
    (sleeping && (state == STATE_ERROR || state == STATE_HOT ||
                  state == STATE_COLD || state == STATE_SLEEP));
  if (forceClosed) {
    roboEyes.setAutoblinker(OFF);
    roboEyes.setIdleMode(OFF);
    roboEyes.close();
  } else {
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
    if (!sleepTransition) roboEyes.open();
  }
}

// Monitor is rendered directly into the same framebuffer, then flushed once.
// No RoboEyes update occurs while MONITOR owns the OLED.
void drawMochiMonitor(uint8_t page, float tempC, float humidityPct, bool dhtOk,
                      float lightPct, bool darkNow, int soundAdc, TemperatureEffect warning) {
  monitorRendering = true;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(page == 0 ? F("MONITOR") : F("SENSORS"));
  display.setCursor(110, 0);
  display.print(page == 0 ? F("1/2") : F("2/2"));
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
  if (page == 0) {
    display.setCursor(0, 15); display.print(F("TEMP"));
    display.setCursor(67, 15);
    if (dhtOk) { display.print(tempC, 1); display.print(F(" C")); }
    else display.print(F("--"));
    display.setCursor(0, 27); display.print(F("HUM"));
    display.setCursor(67, 27);
    if (dhtOk) { display.print(humidityPct, 1); display.print(F(" %")); }
    else display.print(F("--"));
    display.setCursor(0, 39); display.print(F("LIGHT"));
    display.setCursor(67, 39); display.print((int)(lightPct + 0.5f)); display.print(F(" %"));
  } else {
    display.setCursor(0, 15); display.print(F("SOUND"));
    display.setCursor(67, 15); display.print(soundAdc);
    display.setCursor(0, 27); display.print(F("LIGHT"));
    display.setCursor(67, 27); display.print(darkNow ? F("DARK") : F("BRIGHT"));
    display.setCursor(0, 39); display.print(F("DHT"));
    display.setCursor(67, 39); display.print(dhtOk ? F("OK") : F("ERR"));
  }
  display.drawFastHLine(0, 51, 128, SSD1306_WHITE);
  display.setCursor(0, 55); display.print(F("STATUS:"));
  display.setCursor(50, 55);
  if (!dhtOk) display.print(F("ERR"));
  else if (warning == TEMP_EFFECT_HOT) display.print(F("! HOT"));
  else if (warning == TEMP_EFFECT_COLD) display.print(F("* COLD"));
  else display.print(F("OK"));
  display.display();
}
