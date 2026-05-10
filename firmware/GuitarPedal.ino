/*
 * ═══════════════════════════════════════════════════════════
 *  GuitarPedal — ESP32 Multi-Effect Firmware
 *
 *  Features:
 *   - 4-Effect Chain (run 4 effects simultaneously)
 *   - Multi-Effect Chain
 *   - BPM Tap Tempo (STOMP button)
 *   - Auto-Tune Mode
 *   - OLED 0.96" SSD1306 (full menu)
 *   - Expression Pedal (photometer GPIO34)
 *   - STOMP Button (fast bypass GPIO33)
 *   - SD Card (presets + looper + session log)
 *   - WiFi AP + HTTP API for website
 *
 *  Hardware:
 *   - ESP32 CH9102
 *   - INMP441  → GPIO 14(SCK), 15(WS), 32(SD)
 *   - DAC out  → GPIO 25 → 10µF cap → 3.5mm jack
 *   - OLED     → GPIO 21(SDA), 22(SCL)  [I2C]
 *   - SD Card  → GPIO 5(CS), 18(SCK), 19(MISO), 23(MOSI)
 *   - STOMP    → GPIO 33 → GND
 *   - EXPR POT → GPIO 34 (ADC)
 *
 *  Libraries (Manage Libraries):
 *   - ArduinoJson      (Benoit Blanchon)
 *   - Adafruit SSD1306 (Adafruit)
 *   - Adafruit GFX     (Adafruit)
 *   - SD               (built-in)
 * ═══════════════════════════════════════════════════════════
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <driver/dac.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>

// ── includes ──────────────────────────────────────
#include "config.h"
#include "effects.h"
#include "chain.h"
#include "tuner.h"
#include "oled.h"
#include "sdcard.h"
#include "webapi.h"

// ─────────────────────────────────────────────
//  Global objects (defined here, extern)
// ─────────────────────────────────────────────
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
WebServer         server(80);

// State
AppState          state;

// Audio ring buffer (ISR ↔ audio task)
volatile uint8_t  oBuf[OUT_BUF_SIZE];
volatile int      oR = 0, oW = 0;

// I2S read buffer
int32_t           rxBuf[BLOCK_SIZE];

// Timer
hw_timer_t*       dacTimer = NULL;
portMUX_TYPE      timerMux = portMUX_INITIALIZER_UNLOCKED;

// ─────────────────────────────────────────────
//  Timer ISR — DAC output at SAMPLE_RATE
// ─────────────────────────────────────────────
void IRAM_ATTR onDacTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  if (oR != oW) {
    dac_output_voltage(DAC_CHANNEL_1, oBuf[oR]);
    oR = (oR + 1) % OUT_BUF_SIZE;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

// ─────────────────────────────────────────────
//  Init I2S Mic
// ─────────────────────────────────────────────
void initMic() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = BLOCK_SIZE,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t pins = {
    .bck_io_num   = PIN_MIC_SCK,
    .ws_io_num    = PIN_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = PIN_MIC_SD
  };
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  Serial.println("[MIC] I2S ready");
}

// ─────────────────────────────────────────────
//  Init DAC + Timer
// ─────────────────────────────────────────────
void initDAC() {
  dac_output_enable(DAC_CHANNEL_1);
  dac_output_voltage(DAC_CHANNEL_1, 128);
  dacTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(dacTimer, &onDacTimer, true);
  timerAlarmWrite(dacTimer, 1000000 / SAMPLE_RATE, true);
  timerAlarmEnable(dacTimer);
  Serial.println("[DAC] Timer ready");
}

// ─────────────────────────────────────────────
//  Init GPIO
// ─────────────────────────────────────────────
void initGPIO() {
  pinMode(PIN_STOMP, INPUT_PULLUP);
  pinMode(PIN_EXPR,  INPUT);
  Serial.println("[GPIO] Ready");
}

// ─────────────────────────────────────────────
//  STOMP Button handler (tap tempo + bypass)
// ─────────────────────────────────────────────
void handleStomp() {
  static bool     lastState    = HIGH;
  static uint32_t lastPress    = 0;
  static uint32_t pressStart   = 0;

  bool cur = digitalRead(PIN_STOMP);
  if (cur == LOW && lastState == HIGH) {
    pressStart = millis();
    // Tap tempo
    uint32_t now = millis();
    uint32_t gap = now - lastPress;
    if (gap > 200 && gap < 3000) {
      // Rolling average of last 4 taps
      static uint32_t taps[4] = {0,0,0,0};
      static int tapIdx = 0;
      taps[tapIdx % 4] = gap;
      tapIdx++;
      uint32_t avg = 0;
      int cnt = min(tapIdx, 4);
      for (int i = 0; i < cnt; i++) avg += taps[i];
      avg /= cnt;
      state.bpm = constrain(60000 / avg, 20, 300);
      // Sync delay time to BPM
      state.params.delayTime = 60.0f / (float)state.bpm;
      if (state.params.delayTime > 1.0f) state.params.delayTime = 1.0f;
      Serial.printf("[TAP] BPM: %d  Delay: %.2fs\n", state.bpm, state.params.delayTime);
    }
    lastPress = now;
  }
  // Long press (>500ms) = bypass toggle
  if (cur == HIGH && lastState == LOW) {
    if (millis() - pressStart > 500) {
      state.params.bypass = !state.params.bypass;
      Serial.printf("[STOMP] Bypass: %s\n", state.params.bypass ? "ON" : "OFF");
    }
  }
  lastState = cur;
}

// ─────────────────────────────────────────────
//  Expression Pedal
// ─────────────────────────────────────────────
void handleExpression() {
  static uint32_t last = 0;
  if (millis() - last < 20) return;
  last = millis();
  int raw = analogRead(PIN_EXPR);              // 0–4095
  float val = (float)raw / 4095.0f;
  state.exprValue = val;
  // Apply to mapped target
  switch (state.exprTarget) {
    case EXPR_VOLUME:    state.params.masterVolume = val;         break;
    case EXPR_WAH:       state.exprWahFreq = 300 + val * 2500;   break;
    case EXPR_DELAY_MIX: state.params.delayMix = val;            break;
    case EXPR_REVERB_MIX:state.params.reverbMix = val;           break;
    default: break;
  }
}

// ─────────────────────────────────────────────
//  Audio Task — Core 0
// ─────────────────────────────────────────────
void audioTask(void* pv) {
  size_t br;
  while (true) {
    i2s_read(I2S_NUM_0, rxBuf, BLOCK_SIZE * sizeof(int32_t), &br, portMAX_DELAY);
    int n = br / sizeof(int32_t);
    for (int i = 0; i < n; i++) {
      float x = (float)(rxBuf[i] >> 8) / (float)0x7FFFFF;
      if (x >  1.0f) x =  1.0f;
      if (x < -1.0f) x = -1.0f;

      // Update signal level + tuner
      state.signalLevel = state.signalLevel * 0.999f + fabsf(x) * 0.001f;
      tunerProcess(x);

      float y = x;
      if (!state.params.bypass) {
        y = chainProcess(x);          // 4-effect chain
      }
      y *= state.params.masterVolume;
      if (y >  1.0f) y =  1.0f;
      if (y < -1.0f) y = -1.0f;

      uint8_t dac = (uint8_t)constrain((int)((y * 0.5f + 0.5f) * 255.0f), 0, 255);
      int next = (oW + 1) % OUT_BUF_SIZE;
      if (next != oR) { oBuf[oW] = dac; oW = next; }
    }
  }
}

// ─────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Multi-Effect Pedal Boot ===");

  // State defaults
  state = AppState();
  state.chain[0] = 1;  // Chorus
  state.chain[1] = 5;  // Delay
  state.chain[2] = 6;  // Reverb
  state.chain[3] = 0;  // Bypass (free slot)

  // Peripherals
  initGPIO();
  initMic();
  initDAC();
  oledInit();
  sdInit();

  // WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] AP: %s | IP: %s\n", WIFI_SSID, WiFi.softAPIP().toString().c_str());

  // HTTP
  webApiInit();

  // Load last preset from SD
  sdLoadPreset(0);

  // Audio task runs on Core 0
  xTaskCreatePinnedToCore(audioTask, "Audio", 10240, NULL, 24, NULL, 0);

  oledSplash();
  Serial.println("=== Ready ===");
}

// ─────────────────────────────────────────────
//  Loop — Core 1  (WiFi + UI + sensors)
// ─────────────────────────────────────────────
void loop() {
  server.handleClient();
  handleStomp();
  handleExpression();
  oledUpdate();
  sdSessionLog();    // every 10sec record a state in SD

  static uint32_t dbg = 0;
  if (millis() - dbg > 5000) {
    dbg = millis();
    Serial.printf("[Chain] %s → %s → %s → %s | BPM:%d | Expr:%.2f\n",
      effectName(state.chain[0]), effectName(state.chain[1]),
      effectName(state.chain[2]), effectName(state.chain[3]),
      state.bpm, state.exprValue);
  }
}
