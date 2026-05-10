#pragma once
#include "config.h"
#include "effects.h"

/*
 * chainProcess()
 * Processes the sample through 4 effects in order
 * state.chain[0..3] = effect IDs
 * If ID = 0 (Bypass), the signal passes through without processing
 */
float chainProcess(float x) {
  float y = x;
  for (int i = 0; i < CHAIN_LEN; i++) {
    int id = state.chain[i];
    if (id > 0) {          // 0 = bypass slot (skip)
      y = applyEffect(y, id);
     // Hard clip after each effect to keep the signal within range
      if (y >  1.0f) y =  1.0f;
      if (y < -1.0f) y = -1.0f;
    }
  }
  return y;
}

/*
 * chainSetSlot(slot, effectId)
 * Changes the effect in a specific slot (0-3)
 */
void chainSetSlot(int slot, int effectId) {
  if (slot < 0 || slot >= CHAIN_LEN) return;
  if (effectId < 0 || effectId > 18)  return;
  state.chain[slot] = effectId;
}

/*
 * chainSwapSlots(a, b)
 * Swaps the order of two slots
 */
void chainSwapSlots(int a, int b) {
  if (a < 0 || a >= CHAIN_LEN) return;
  if (b < 0 || b >= CHAIN_LEN) return;
  int tmp       = state.chain[a];
  state.chain[a]= state.chain[b];
  state.chain[b]= tmp;
}

/*
 * chainDescribe(buf, size)
 * Writes the chain description as a string
 * Example: "Chorus→Delay→Reverb→—"
 */
void chainDescribe(char* buf, int size) {
  buf[0] = '\0';
  for (int i = 0; i < CHAIN_LEN; i++) {
    strncat(buf, effectName(state.chain[i]), size - strlen(buf) - 1);
    if (i < CHAIN_LEN - 1)
      strncat(buf, "->", size - strlen(buf) - 1);
  }
}
