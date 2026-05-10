#pragma once
#include "config.h"
#include "chain.h"
#include "oled.h"
#include "sdcard.h"
#include <ArduinoJson.h>

// ─────────────────────────────────────────────
//  CORS headers
// ─────────────────────────────────────────────
void _cors() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void _ok()  { _cors(); server.send(200, "application/json", "{\"ok\":true}"); }
void _opt() { _cors(); server.send(200, "text/plain", ""); }

// ─────────────────────────────────────────────
//  GET /status
// Returns the full state — called by the website every 500ms
// ─────────────────────────────────────────────
void _handleStatus() {
  _cors();
  StaticJsonDocument<900> d;
  d["connected"]      = true;
  // Chain
  JsonArray ch = d.createNestedArray("chain");
  for (int i = 0; i < CHAIN_LEN; i++) ch.add(state.chain[i]);
  char chainDesc[64]; chainDescribe(chainDesc, sizeof(chainDesc));
  d["chainDesc"]      = chainDesc;
  // Compat with old site (effect = first active slot)
  for (int i = 0; i < CHAIN_LEN; i++) {
    if (state.chain[i] != 0) { d["effect"] = state.chain[i]; break; }
  }
  d["effectName"]     = effectName(state.chain[0]);
  // State
  d["bypass"]         = state.params.bypass;
  d["masterVolume"]   = state.params.masterVolume;
  d["bpm"]            = state.bpm;
  d["autoTune"]       = state.autoTune;
  d["tunerFreq"]      = state.tunerFreq;
  d["tunerNote"]      = state.tunerNote;
  d["tunerCents"]     = state.tunerCents;
  d["signalLevel"]    = state.signalLevel;
  d["exprValue"]      = state.exprValue;
  d["exprTarget"]     = (int)state.exprTarget;
  d["presetSlot"]     = state.presetSlot;
  d["sdReady"]        = state.sdReady;
  d["oledPage"]       = (int)state.oledPage;
  // All params
  d["chorusRate"]     = state.params.chorusRate;
  d["chorusDepth"]    = state.params.chorusDepth;
  d["chorusMix"]      = state.params.chorusMix;
  d["phaserRate"]     = state.params.phaserRate;
  d["phaserDepth"]    = state.params.phaserDepth;
  d["phaserFeedback"] = state.params.phaserFeedback;
  d["tremoloRate"]    = state.params.tremoloRate;
  d["tremoloDepth"]   = state.params.tremoloDepth;
  d["tremoloSquare"]  = state.params.tremoloSquare;
  d["vibratoRate"]    = state.params.vibratoRate;
  d["vibratoDepth"]   = state.params.vibratoDepth;
  d["delayTime"]      = state.params.delayTime;
  d["delayFeedback"]  = state.params.delayFeedback;
  d["delayMix"]       = state.params.delayMix;
  d["reverbDecay"]    = state.params.reverbDecay;
  d["reverbMix"]      = state.params.reverbMix;
  d["eqBass"]         = state.params.eqBass;
  d["eqMid"]          = state.params.eqMid;
  d["eqTreble"]       = state.params.eqTreble;
  d["compThreshold"]  = state.params.compThreshold;
  d["compRatio"]      = state.params.compRatio;
  d["gateThreshold"]  = state.params.gateThreshold;
  d["octaveDry"]      = state.params.octaveDry;
  d["octaveWet"]      = state.params.octaveWet;
  d["pitchSemitones"] = state.params.pitchSemitones;
  d["volume"]         = state.params.volume;

  String j; serializeJson(d, j);
  server.send(200, "application/json", j);
}

// ─────────────────────────────────────────────
//  GET /setParam?key=val&key2=val2...
// Same old API + new API
// ─────────────────────────────────────────────
void _handleSetParam() {
  _cors();
  for (int i = 0; i < server.args(); i++) {
    String k = server.argName(i);
    float  f = server.arg(i).toFloat();
    int    v = server.arg(i).toInt();
    // Legacy single-effect compat
    if      (k=="effect")         chainSetSlot(0, v);
    else if (k=="bypass")         state.params.bypass         = v!=0;
    else if (k=="masterVolume")   state.params.masterVolume   = f;
    else if (k=="chorusRate")     state.params.chorusRate     = f;
    else if (k=="chorusDepth")    state.params.chorusDepth    = f;
    else if (k=="chorusMix")      state.params.chorusMix      = f;
    else if (k=="phaserRate")     state.params.phaserRate     = f;
    else if (k=="phaserDepth")    state.params.phaserDepth    = f;
    else if (k=="phaserFeedback") state.params.phaserFeedback = f;
    else if (k=="tremoloRate")    state.params.tremoloRate    = f;
    else if (k=="tremoloDepth")   state.params.tremoloDepth   = f;
    else if (k=="tremoloSquare")  state.params.tremoloSquare  = v!=0;
    else if (k=="vibratoRate")    state.params.vibratoRate    = f;
    else if (k=="vibratoDepth")   state.params.vibratoDepth   = f;
    else if (k=="delayTime")      state.params.delayTime      = f;
    else if (k=="delayFeedback")  state.params.delayFeedback  = f;
    else if (k=="delayMix")       state.params.delayMix       = f;
    else if (k=="reverbDecay")    state.params.reverbDecay    = f;
    else if (k=="reverbMix")      state.params.reverbMix      = f;
    else if (k=="eqBass")         state.params.eqBass         = f;
    else if (k=="eqMid")          state.params.eqMid          = f;
    else if (k=="eqTreble")       state.params.eqTreble       = f;
    else if (k=="compThreshold")  state.params.compThreshold  = f;
    else if (k=="compRatio")      state.params.compRatio      = f;
    else if (k=="gateThreshold")  state.params.gateThreshold  = f;
    else if (k=="octaveDry")      state.params.octaveDry      = f;
    else if (k=="octaveWet")      state.params.octaveWet      = f;
    else if (k=="pitchSemitones") state.params.pitchSemitones = f;
    else if (k=="volume")         state.params.volume         = f;
    // New params
    else if (k=="bpm")            state.bpm                   = v;
    else if (k=="autoTune")       state.autoTune              = v!=0;
    else if (k=="exprTarget")     state.exprTarget            = (ExprTarget)v;
  }
  _ok();
}

// ─────────────────────────────────────────────
//  GET /setChain?s=0&e=5
//  s = slot (0-3), e = effect id (0-18)
// ─────────────────────────────────────────────
void _handleSetChain() {
  _cors();
  int slot = server.hasArg("s") ? server.arg("s").toInt() : 0;
  int eff  = server.hasArg("e") ? server.arg("e").toInt() : 0;
  chainSetSlot(slot, eff);
  Serial.printf("[CHAIN] Slot %d = %s\n", slot, effectName(eff));
  _ok();
}

// ─────────────────────────────────────────────
//  GET /swapChain?a=0&b=1
// ─────────────────────────────────────────────
void _handleSwapChain() {
  _cors();
  int a = server.hasArg("a") ? server.arg("a").toInt() : 0;
  int b = server.hasArg("b") ? server.arg("b").toInt() : 1;
  chainSwapSlots(a, b);
  _ok();
}

// ─────────────────────────────────────────────
//  GET /bypass?en=1
// ─────────────────────────────────────────────
void _handleBypass() {
  _cors();
  if (server.hasArg("en"))
    state.params.bypass = server.arg("en").toInt() != 0;
  _ok();
}

// ─────────────────────────────────────────────
//  GET /savePreset?n=0
//  GET /loadPreset?n=0
// ─────────────────────────────────────────────
void _handleSavePreset() {
  _cors();
  int n = server.hasArg("n") ? server.arg("n").toInt() : 0;
  sdSavePreset(n);
  _ok();
}

void _handleLoadPreset() {
  _cors();
  int n = server.hasArg("n") ? server.arg("n").toInt() : 0;
  sdLoadPreset(n);
  _ok();
}

// ─────────────────────────────────────────────
//  GET /oledPage?p=1
//  GET /oledNext
// ─────────────────────────────────────────────
void _handleOledPage() {
  _cors();
  if (server.hasArg("p"))
    state.oledPage = (OledPage)constrain(server.arg("p").toInt(), 0, 4);
  _ok();
}

void _handleOledNext() {
  _cors();
  oledNextPage();
  _ok();
}

// ─────────────────────────────────────────────
//  GET /tapTempo
// Triggers a tap from the API (same as the STOMP button)
// ─────────────────────────────────────────────
void _handleTapTempo() {
  _cors();
  static uint32_t lastTap = 0;
  static uint32_t taps[4] = {0,0,0,0};
  static int tapIdx = 0;
  uint32_t now = millis();
  uint32_t gap = now - lastTap;
  if (gap > 200 && gap < 3000) {
    taps[tapIdx % 4] = gap;
    tapIdx++;
    uint32_t avg = 0;
    int cnt = min(tapIdx, 4);
    for (int i = 0; i < cnt; i++) avg += taps[i];
    avg /= cnt;
    state.bpm = constrain(60000 / avg, 20, 300);
    state.params.delayTime = 60.0f / (float)state.bpm;
    if (state.params.delayTime > 1.0f) state.params.delayTime = 1.0f;
  }
  lastTap = now;
  _ok();
}

// ─────────────────────────────────────────────
//  Looper stubs
// ─────────────────────────────────────────────
void _handleLooper() { _cors(); _ok(); }

// ─────────────────────────────────────────────
//  webApiInit()
// ─────────────────────────────────────────────
void webApiInit() {
  server.on("/status",      HTTP_GET,     _handleStatus);
  server.on("/setParam",    HTTP_GET,     _handleSetParam);
  server.on("/setChain",    HTTP_GET,     _handleSetChain);
  server.on("/swapChain",   HTTP_GET,     _handleSwapChain);
  server.on("/bypass",      HTTP_GET,     _handleBypass);
  server.on("/savePreset",  HTTP_GET,     _handleSavePreset);
  server.on("/loadPreset",  HTTP_GET,     _handleLoadPreset);
  server.on("/oledPage",    HTTP_GET,     _handleOledPage);
  server.on("/oledNext",    HTTP_GET,     _handleOledNext);
  server.on("/tapTempo",    HTTP_GET,     _handleTapTempo);
  server.on("/loopRecord",  HTTP_GET,     _handleLooper);
  server.on("/loopPlay",    HTTP_GET,     _handleLooper);
  server.on("/loopClear",   HTTP_GET,     _handleLooper);

  // OPTIONS preflight
  server.on("/status",    HTTP_OPTIONS, _opt);
  server.on("/setParam",  HTTP_OPTIONS, _opt);
  server.on("/setChain",  HTTP_OPTIONS, _opt);

  server.onNotFound([](){
    _cors();
    server.send(404, "application/json", "{\"error\":\"not found\"}");
  });

  server.begin();
  Serial.println("[HTTP] Server ready");
}
