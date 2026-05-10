#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────
//  WiFi
// ─────────────────────────────────────────────
#define WIFI_SSID   "GuitarPedal"
#define WIFI_PASS   "guitar123"

// ─────────────────────────────────────────────
//  Pins
// ─────────────────────────────────────────────
// I2S Mic (INMP441)
#define PIN_MIC_SCK   14
#define PIN_MIC_WS    15
#define PIN_MIC_SD    32

// DAC output
#define PIN_DAC       25    // GPIO25 = DAC_CHANNEL_1

// OLED I2C (SSD1306)
#define PIN_SDA       21
#define PIN_SCL       22
#define OLED_W        128
#define OLED_H        64
#define OLED_ADDR     0x3C

// SD Card SPI
#define PIN_SD_CS     5
#define PIN_SD_SCK    18
#define PIN_SD_MISO   19
#define PIN_SD_MOSI   23

// Controls
#define PIN_STOMP     33    // STOMP button (INPUT_PULLUP)
#define PIN_EXPR      34    // Expression pedal (ADC)

// ─────────────────────────────────────────────
//  Audio
// ─────────────────────────────────────────────
#define SAMPLE_RATE     20000
#define BLOCK_SIZE      128
#define OUT_BUF_SIZE    512
#define CHAIN_LEN       4       // Maximum number of effects in the chain
#define MAX_DELAY_SAMP  (SAMPLE_RATE * 1)
#define CHORUS_SIZE     1024

// ─────────────────────────────────────────────
//  Expression targets
// ─────────────────────────────────────────────
enum ExprTarget {
  EXPR_VOLUME    = 0,
  EXPR_WAH       = 1,
  EXPR_DELAY_MIX = 2,
  EXPR_REVERB_MIX= 3,
};

// ─────────────────────────────────────────────
//  OLED menu pages
// ─────────────────────────────────────────────
enum OledPage {
  PAGE_MAIN   = 0,   // Effect + level + BPM
  PAGE_TUNER  = 1,   // Tuner display
  PAGE_CHAIN  = 2,   // Chain editor
  PAGE_PRESET = 3,   // Preset selector
  PAGE_EXPR   = 4,   // Expression config
};

// ─────────────────────────────────────────────
//  Effect Parameters
// ─────────────────────────────────────────────
struct Params {
  bool  bypass         = false;
  float masterVolume   = 0.8f;

  float chorusRate     = 0.5f,  chorusDepth  = 0.3f,  chorusMix    = 0.5f;
  float phaserRate     = 0.8f,  phaserDepth  = 0.7f,  phaserFeedback=0.5f;
  float tremoloRate    = 5.0f,  tremoloDepth = 0.8f;
  bool  tremoloSquare  = false;
  float vibratoRate    = 5.0f,  vibratoDepth = 0.3f;
  float delayTime      = 0.3f,  delayFeedback= 0.4f,  delayMix     = 0.5f;
  float reverbDecay    = 0.6f,  reverbMix    = 0.4f;
  float eqBass         = 1.0f,  eqMid        = 1.0f,  eqTreble     = 1.0f;
  float compThreshold  = 0.5f,  compRatio    = 4.0f;
  float gateThreshold  = 0.02f;
  float octaveDry      = 0.7f,  octaveWet    = 0.5f;
  float pitchSemitones = 0.0f;
  float volume         = 1.0f;
};

// ─────────────────────────────────────────────
//  Global App State
// ─────────────────────────────────────────────
struct AppState {
  Params   params;
  int      chain[CHAIN_LEN] = {0, 0, 0, 0};  // effect IDs in order
  int      bpm         = 120;
  bool     autoTune    = false;
  float    tunerFreq   = 0.0f;
  float    tunerCents  = 0.0f;
  char     tunerNote[8]= "—";
  float    signalLevel = 0.0f;
  float    exprValue   = 0.0f;
  float    exprWahFreq = 800.0f;
  ExprTarget exprTarget= EXPR_VOLUME;
  OledPage oledPage    = PAGE_MAIN;
  int      presetSlot  = 0;
  bool     sdReady     = false;
  char     sessionFile[32] = "/sessions/boot.txt";
};

// ─────────────────────────────────────────────
//  Extern declarations
// ─────────────────────────────────────────────
extern AppState            state;
extern Adafruit_SSD1306    display;
extern WebServer           server;
extern volatile uint8_t    oBuf[];
extern volatile int        oR, oW;
extern int32_t             rxBuf[];
