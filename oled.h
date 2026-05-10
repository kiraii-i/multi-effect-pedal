#pragma once
#include "config.h"
#include "chain.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ─────────────────────────────────────────────
//  OLED refresh rate
// ─────────────────────────────────────────────
#define OLED_REFRESH_MS   80

static uint32_t _oledLast = 0;

// ─────────────────────────────────────────────
//  oledInit()
// ─────────────────────────────────────────────
void oledInit() {
  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] Not found — check wiring");
    return;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  Serial.println("[OLED] Ready");
}

// ─────────────────────────────────────────────
//  Splash screen
// ─────────────────────────────────────────────
void oledSplash() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 8);
  display.println("Multi-Effect Pedal");
  display.setCursor(10, 22);
  display.println("ESP32  Multi-FX");
  display.setCursor(10, 38);
  display.setTextSize(1);
  display.println("WiFi: GuitarPedal");
  display.setCursor(10, 50);
  display.println("IP:  192.168.4.1");
  display.display();
  delay(2500);
}

// ─────────────────────────────────────────────
//  Draw signal meter bar (8px tall)
// ─────────────────────────────────────────────
void _drawMeter(int x, int y, float level) {
  int w = (int)(level * 60.0f);
  w = constrain(w, 0, 60);
  display.drawRect(x, y, 62, 6, SSD1306_WHITE);
  display.fillRect(x+1, y+1, w, 4, SSD1306_WHITE);
}

// ─────────────────────────────────────────────
//  Draw BPM indicator
// ─────────────────────────────────────────────
void _drawBPM(int x, int y) {
  display.setTextSize(1);
  display.setCursor(x, y);
  display.printf("BPM:%3d", state.bpm);
}

// ─────────────────────────────────────────────
//  PAGE 0 — Main
//  Effect chain + level + BPM + bypass state
// ─────────────────────────────────────────────
void _drawPageMain() {
  display.setTextSize(1);

  // Title bar
  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(2, 1);
  if (state.params.bypass) {
    display.print("  *** BYPASS ***  ");
  } else {
    display.print(" MULTI-FX  CHAIN ");
  }
  display.setTextColor(SSD1306_WHITE);

  // Chain slots
  for (int i = 0; i < CHAIN_LEN; i++) {
    int sx = (i % 2) * 64;
    int sy = 12 + (i / 2) * 18;
    bool active = (state.chain[i] != 0);
    if (active) display.drawRect(sx, sy, 62, 14, SSD1306_WHITE);
    else        display.drawRect(sx, sy, 62, 14, SSD1306_WHITE);
    display.setCursor(sx + 3, sy + 3);
    display.setTextSize(1);
    if (active) {
      display.printf("[%d]%s", i+1, effectName(state.chain[i]));
    } else {
      display.printf("[%d]---", i+1);
    }
  }

  // Bottom bar: level + BPM
  _drawMeter(0, 50, state.signalLevel * 3.0f);
  display.setCursor(65, 50);
  display.printf("B%3d", state.bpm);
  display.setCursor(65, 58);
  if (state.autoTune) {
    display.print("AUTO");
  } else {
    display.printf("V%2d%%", (int)(state.params.masterVolume * 100));
  }
}

// ─────────────────────────────────────────────
//  PAGE 1 — Tuner
// ─────────────────────────────────────────────
void _drawPageTuner() {
  display.setTextSize(1);
  display.setCursor(30, 0);
  display.print("-- TUNER --");

  // Note name big
  display.setTextSize(3);
  int nx = (128 - strlen(state.tunerNote) * 18) / 2;
  display.setCursor(nx < 0 ? 0 : nx, 12);
  display.print(state.tunerNote);

  // Frequency
  display.setTextSize(1);
  display.setCursor(0, 42);
  if (state.tunerFreq > 0) {
    display.printf("%.1f Hz", state.tunerFreq);
  } else {
    display.print("Play a note...");
  }

  // Cents meter
  display.drawRect(14, 52, 100, 8, SSD1306_WHITE);
  int mid = 14 + 50;
  display.drawFastVLine(mid, 52, 8, SSD1306_WHITE);
  if (state.tunerFreq > 0) {
    int pos = constrain((int)(state.tunerCents), -50, 50);
    int px  = mid + pos;
    display.fillRect(px < mid ? px : mid, 53,
                     abs(pos) + 1, 6, SSD1306_WHITE);
  }

  // In-tune indicator
  display.setCursor(116, 52);
  if (tunerInTune()) {
    display.print("OK");
  }

  // Auto-tune mode indicator
  display.setCursor(0, 52);
  if (state.autoTune) display.print("AT");
}

// ─────────────────────────────────────────────
//  PAGE 2 — Chain Editor
// ─────────────────────────────────────────────
void _drawPageChain() {
  display.setTextSize(1);
  display.setCursor(20, 0);
  display.print("EFFECT CHAIN");
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  for (int i = 0; i < CHAIN_LEN; i++) {
    display.setCursor(0, 12 + i * 12);
    display.printf("Slot %d: [%2d] %s", i+1,
      state.chain[i], effectName(state.chain[i]));
  }

  display.setCursor(0, 57);
  display.print("API: /setChain?s=0&e=5");
}

// ─────────────────────────────────────────────
//  PAGE 3 — Preset Selector
// ─────────────────────────────────────────────
void _drawPagePreset() {
  display.setTextSize(1);
  display.setCursor(25, 0);
  display.print("PRESETS (SD)");
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  for (int i = 0; i < 5; i++) {
    display.setCursor(5, 12 + i * 10);
    if (i == state.presetSlot) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    display.printf("Preset %d", i + 1);
    if (i == state.presetSlot) display.print(" *");
  }
  display.setCursor(0, 57);
  display.print("API: /loadPreset?n=0");
}

// ─────────────────────────────────────────────
//  PAGE 4 — Expression Pedal
// ─────────────────────────────────────────────
void _drawPageExpr() {
  display.setTextSize(1);
  display.setCursor(15, 0);
  display.print("EXPRESSION PEDAL");
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  const char* targets[] = {"Volume","Wah Freq","Delay Mix","Reverb Mix"};
  display.setCursor(0, 12);
  display.printf("Target: %s", targets[(int)state.exprTarget]);

  display.setCursor(0, 24);
  display.printf("Value:  %.0f%%", state.exprValue * 100.0f);

  // Big bar
  display.drawRect(0, 36, 128, 12, SSD1306_WHITE);
  int bw = (int)(state.exprValue * 126.0f);
  display.fillRect(1, 37, bw, 10, SSD1306_WHITE);

  display.setCursor(0, 52);
  display.printf("GPIO34 ADC: %d", (int)(state.exprValue * 4095));
}

// ─────────────────────────────────────────────
//  oledUpdate() — called in loop()
// ─────────────────────────────────────────────
void oledUpdate() {
  if (millis() - _oledLast < OLED_REFRESH_MS) return;
  _oledLast = millis();

  display.clearDisplay();

  // Page indicator dots (top right)
  for (int i = 0; i < 5; i++) {
    int dx = 118 + (i % 3) * 4;
    int dy = (i / 3) * 4;
    if (i == (int)state.oledPage)
      display.fillCircle(dx, dy + 2, 1, SSD1306_WHITE);
    else
      display.drawCircle(dx, dy + 2, 1, SSD1306_WHITE);
  }

  switch (state.oledPage) {
    case PAGE_MAIN:   _drawPageMain();   break;
    case PAGE_TUNER:  _drawPageTuner();  break;
    case PAGE_CHAIN:  _drawPageChain();  break;
    case PAGE_PRESET: _drawPagePreset(); break;
    case PAGE_EXPR:   _drawPageExpr();   break;
  }

  display.display();
}

// ─────────────────────────────────────────────
//  oledNextPage() — called from the API or a button
// ─────────────────────────────────────────────
void oledNextPage() {
  state.oledPage = (OledPage)(((int)state.oledPage + 1) % 5);
}
