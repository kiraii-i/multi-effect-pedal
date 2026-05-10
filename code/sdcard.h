#pragma once
#include "config.h"
#include <SD.h>
#include <ArduinoJson.h>

// ─────────────────────────────────────────────
//  sdInit()
// ─────────────────────────────────────────────
void sdInit() {
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("[SD] Not found — presets disabled");
    state.sdReady = false;
    return;
  }
  state.sdReady = true;
  Serial.printf("[SD] Ready — %llu MB\n", SD.totalBytes() / (1024*1024));

  // Create folders if missing
  if (!SD.exists("/presets"))  SD.mkdir("/presets");
  if (!SD.exists("/sessions")) SD.mkdir("/sessions");
  if (!SD.exists("/loops"))    SD.mkdir("/loops");

  // Create session filename with millis timestamp
  snprintf(state.sessionFile, sizeof(state.sessionFile),
           "/sessions/s%lu.txt", millis());
}

// ─────────────────────────────────────────────
// sdSavePreset(slot) — saves the current state
// ─────────────────────────────────────────────
void sdSavePreset(int slot) {
  if (!state.sdReady) return;
  char path[32];
  snprintf(path, sizeof(path), "/presets/p%d.json", slot);
  File f = SD.open(path, FILE_WRITE);
  if (!f) { Serial.printf("[SD] Cannot open %s\n", path); return; }

  StaticJsonDocument<800> doc;
  // Chain
  JsonArray ch = doc.createNestedArray("chain");
  for (int i = 0; i < CHAIN_LEN; i++) ch.add(state.chain[i]);
  // Params
  doc["bypass"]         = state.params.bypass;
  doc["masterVolume"]   = state.params.masterVolume;
  doc["chorusRate"]     = state.params.chorusRate;
  doc["chorusDepth"]    = state.params.chorusDepth;
  doc["chorusMix"]      = state.params.chorusMix;
  doc["phaserRate"]     = state.params.phaserRate;
  doc["phaserDepth"]    = state.params.phaserDepth;
  doc["phaserFeedback"] = state.params.phaserFeedback;
  doc["tremoloRate"]    = state.params.tremoloRate;
  doc["tremoloDepth"]   = state.params.tremoloDepth;
  doc["tremoloSquare"]  = state.params.tremoloSquare;
  doc["vibratoRate"]    = state.params.vibratoRate;
  doc["vibratoDepth"]   = state.params.vibratoDepth;
  doc["delayTime"]      = state.params.delayTime;
  doc["delayFeedback"]  = state.params.delayFeedback;
  doc["delayMix"]       = state.params.delayMix;
  doc["reverbDecay"]    = state.params.reverbDecay;
  doc["reverbMix"]      = state.params.reverbMix;
  doc["eqBass"]         = state.params.eqBass;
  doc["eqMid"]          = state.params.eqMid;
  doc["eqTreble"]       = state.params.eqTreble;
  doc["compThreshold"]  = state.params.compThreshold;
  doc["compRatio"]      = state.params.compRatio;
  doc["gateThreshold"]  = state.params.gateThreshold;
  doc["octaveDry"]      = state.params.octaveDry;
  doc["octaveWet"]      = state.params.octaveWet;
  doc["pitchSemitones"] = state.params.pitchSemitones;
  doc["volume"]         = state.params.volume;
  doc["bpm"]            = state.bpm;
  doc["autoTune"]       = state.autoTune;
  doc["exprTarget"]     = (int)state.exprTarget;

  serializeJson(doc, f);
  f.close();
  Serial.printf("[SD] Preset %d saved → %s\n", slot, path);
}

// ─────────────────────────────────────────────
//  sdLoadPreset(slot)
// ─────────────────────────────────────────────
void sdLoadPreset(int slot) {
  if (!state.sdReady) return;
  char path[32];
  snprintf(path, sizeof(path), "/presets/p%d.json", slot);
  if (!SD.exists(path)) {
    Serial.printf("[SD] Preset %d not found\n", slot);
    return;
  }
  File f = SD.open(path, FILE_READ);
  if (!f) return;

  StaticJsonDocument<800> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) { Serial.printf("[SD] JSON error: %s\n", err.c_str()); return; }

  // Chain
  JsonArray ch = doc["chain"];
  for (int i = 0; i < CHAIN_LEN && i < (int)ch.size(); i++)
    state.chain[i] = ch[i];

  // Params
  if (doc.containsKey("bypass"))         state.params.bypass         = doc["bypass"];
  if (doc.containsKey("masterVolume"))   state.params.masterVolume   = doc["masterVolume"];
  if (doc.containsKey("chorusRate"))     state.params.chorusRate     = doc["chorusRate"];
  if (doc.containsKey("chorusDepth"))    state.params.chorusDepth    = doc["chorusDepth"];
  if (doc.containsKey("chorusMix"))      state.params.chorusMix      = doc["chorusMix"];
  if (doc.containsKey("phaserRate"))     state.params.phaserRate     = doc["phaserRate"];
  if (doc.containsKey("phaserDepth"))    state.params.phaserDepth    = doc["phaserDepth"];
  if (doc.containsKey("phaserFeedback")) state.params.phaserFeedback = doc["phaserFeedback"];
  if (doc.containsKey("tremoloRate"))    state.params.tremoloRate    = doc["tremoloRate"];
  if (doc.containsKey("tremoloDepth"))   state.params.tremoloDepth   = doc["tremoloDepth"];
  if (doc.containsKey("tremoloSquare"))  state.params.tremoloSquare  = doc["tremoloSquare"];
  if (doc.containsKey("vibratoRate"))    state.params.vibratoRate    = doc["vibratoRate"];
  if (doc.containsKey("vibratoDepth"))   state.params.vibratoDepth   = doc["vibratoDepth"];
  if (doc.containsKey("delayTime"))      state.params.delayTime      = doc["delayTime"];
  if (doc.containsKey("delayFeedback"))  state.params.delayFeedback  = doc["delayFeedback"];
  if (doc.containsKey("delayMix"))       state.params.delayMix       = doc["delayMix"];
  if (doc.containsKey("reverbDecay"))    state.params.reverbDecay    = doc["reverbDecay"];
  if (doc.containsKey("reverbMix"))      state.params.reverbMix      = doc["reverbMix"];
  if (doc.containsKey("eqBass"))         state.params.eqBass         = doc["eqBass"];
  if (doc.containsKey("eqMid"))          state.params.eqMid          = doc["eqMid"];
  if (doc.containsKey("eqTreble"))       state.params.eqTreble       = doc["eqTreble"];
  if (doc.containsKey("compThreshold"))  state.params.compThreshold  = doc["compThreshold"];
  if (doc.containsKey("compRatio"))      state.params.compRatio      = doc["compRatio"];
  if (doc.containsKey("gateThreshold"))  state.params.gateThreshold  = doc["gateThreshold"];
  if (doc.containsKey("octaveDry"))      state.params.octaveDry      = doc["octaveDry"];
  if (doc.containsKey("octaveWet"))      state.params.octaveWet      = doc["octaveWet"];
  if (doc.containsKey("pitchSemitones")) state.params.pitchSemitones = doc["pitchSemitones"];
  if (doc.containsKey("volume"))         state.params.volume         = doc["volume"];
  if (doc.containsKey("bpm"))            state.bpm                   = doc["bpm"];
  if (doc.containsKey("autoTune"))       state.autoTune              = doc["autoTune"];
  if (doc.containsKey("exprTarget"))     state.exprTarget            = (ExprTarget)(int)doc["exprTarget"];

  state.presetSlot = slot;
  Serial.printf("[SD] Preset %d loaded\n", slot);
}

// ─────────────────────────────────────────────
// sdSessionLog() — logs the state every 10 seconds
// ─────────────────────────────────────────────
void sdSessionLog() {
  if (!state.sdReady) return;
  static uint32_t last = 0;
  if (millis() - last < 10000) return;
  last = millis();

  File f = SD.open(state.sessionFile, FILE_APPEND);
  if (!f) return;

  char chainDesc[64];
  chainDescribe(chainDesc, sizeof(chainDesc));

  f.printf("[%lu] Chain:%s BPM:%d Vol:%.2f Bypass:%d AutoTune:%d Freq:%.1f\n",
    millis(),
    chainDesc,
    state.bpm,
    state.params.masterVolume,
    state.params.bypass,
    state.autoTune,
    state.tunerFreq
  );
  f.close();
}
