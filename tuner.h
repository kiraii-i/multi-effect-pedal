#pragma once
#include "config.h"
#include <math.h>

// ─────────────────────────────────────────────
//  Tuner buffers
// ─────────────────────────────────────────────
static float _tBuf[2048] = {};
static int   _tIdx = 0;

// Note names
static const char* _noteNames[] = {
  "A","A#","B","C","C#","D","D#","E","F","F#","G","G#"
};

// ─────────────────────────────────────────────
//  tunerProcess(sample)
// Called for each sample — calculates the frequency every 512 samples
// ─────────────────────────────────────────────
void tunerProcess(float s) {
  _tBuf[_tIdx % 2048] = s;
  _tIdx++;
  if (_tIdx % 512 != 0) return;

  // Autocorrelation
  int   N    = 1024;
  int   minL = (int)(SAMPLE_RATE / 1200.f);  // 1200 Hz max
  int   maxL = (int)(SAMPLE_RATE / 50.f);    // 50 Hz min
  float mx   = 0;
  int   best = 0;

  for (int lag = minL; lag < maxL && lag < N; lag++) {
    float corr = 0;
    for (int i = 0; i < 256; i++) {
      corr += _tBuf[(_tIdx - N + i) % 2048] *
              _tBuf[(_tIdx - N + i + lag) % 2048];
    }
    if (corr > mx) { mx = corr; best = lag; }
  }

  if (best > 0 && mx > 0.001f) {
    float freq = (float)SAMPLE_RATE / (float)best;
    state.tunerFreq = freq;

    // Convert to note + cents
    float  A4       = 440.0f;
    float  semitones= 12.0f * log2f(freq / A4);
    int    rounded  = (int)roundf(semitones);
    int    noteIdx  = ((rounded % 12) + 12) % 12;
    int    octave   = 4 + (int)floorf((rounded + 9) / 12.0f);
    float  targetF  = A4 * powf(2.0f, (float)rounded / 12.0f);
    float  cents    = 1200.0f * log2f(freq / targetF);

    state.tunerCents = cents;
    snprintf(state.tunerNote, sizeof(state.tunerNote), "%s%d",
             _noteNames[noteIdx], octave);

    // Auto-Tune: shift pitch to nearest note automatically
    if (state.autoTune && fabsf(cents) > 3.0f) {
      // Apply correction via pitch shift
      state.params.pitchSemitones = -cents / 100.0f;
      // Clamp to ±2 semitones for stability
      if (state.params.pitchSemitones >  2.0f) state.params.pitchSemitones =  2.0f;
      if (state.params.pitchSemitones < -2.0f) state.params.pitchSemitones = -2.0f;
    } else if (!state.autoTune) {
      // Reset auto-tune correction if mode off
      if (fabsf(state.params.pitchSemitones) < 2.0f &&
          state.autoTune == false) {
        state.params.pitchSemitones = 0.0f;
      }
    }
  } else {
    state.tunerFreq  = 0.0f;
    state.tunerCents = 0.0f;
    snprintf(state.tunerNote, sizeof(state.tunerNote), "—");
  }
}

// ─────────────────────────────────────────────
// tunerInTune() → true if within ±5 cents
// ─────────────────────────────────────────────
bool tunerInTune() {
  return state.tunerFreq > 0 && fabsf(state.tunerCents) < 5.0f;
}
