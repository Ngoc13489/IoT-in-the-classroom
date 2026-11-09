#pragma once
#include <Arduino.h>
#include "MochiTypes.h"
void updateMochiFace(DisplayState state);
void drawMochiExtras(DisplayState state);
void beginMochiDisplay();
void updateMochiDisplay();
void setMochiSquint(bool enabled);
void setMochiSleepTransition(bool enabled, unsigned long elapsedMs);
void refreshMochiFace();
enum TemperatureEffect : uint8_t { TEMP_EFFECT_NONE, TEMP_EFFECT_HOT, TEMP_EFFECT_COLD };
void setMochiTemperatureEffect(TemperatureEffect effect);

// Step 5B: MONITOR owns the OLED only when PET rendering is paused.
void drawMochiMonitor(uint8_t page, float tempC, float humidityPct, bool dhtOk,
                      float lightPct, bool dark, int soundAdc, TemperatureEffect warning);
