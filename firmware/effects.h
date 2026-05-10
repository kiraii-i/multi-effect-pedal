#pragma once
#include "config.h"
#include <math.h>

// ─────────────────────────────────────────────
//  Shared DSP buffers
// ─────────────────────────────────────────────
static float  _delayBuf[MAX_DELAY_SAMP]  = {};
static float  _chorusBuf[CHORUS_SIZE]    = {};
static float  _flangerBuf[1024]          = {};
static int    _wDelay=0, _wChorus=0, _wFlanger=0;

// LFO phases (per effect)
static float  _phCh=0,_phPh=0,_phTr=0,_phVi=0;
static float  _phWa=0,_phRi=0,_phFl=0;

// Phaser allpass
static float  _apX[4]={},_apY[4]={};

// EQ biquad
static float  _bBx1=0,_bBx2=0,_bBy1=0,_bBy2=0;
static float  _bMx1=0,_bMx2=0,_bMy1=0,_bMy2=0;
static float  _bTx1=0,_bTx2=0,_bTy1=0,_bTy2=0;

// Dynamics
static float  _compG=1,_compE=0,_gateE=0;

// Octave
static float  _octP=0,_octF=1,_octS=0;

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
inline float _clip(float x,float t=1.f){return x>t?t:x<-t?-t:x;}

inline float _soft(float x){
  x=_clip(x); return x-(x*x*x)/3.f;
}

float _interp(float* b,int sz,int wp,float ds){
  float rf=(float)wp-ds;
  while(rf<0) rf+=sz;
  int r0=(int)rf%sz, r1=(r0+1)%sz;
  float fr=rf-(int)rf;
  return b[r0]*(1-fr)+b[r1]*fr;
}

float _biquad(float x,float &x1,float &x2,float &y1,float &y2,
              float fc,float gain,float Q){
  if(fabsf(gain-1.f)<0.02f) return x;
  float A=sqrtf(fabsf(gain));
  float w=2.f*M_PI*fc/SAMPLE_RATE;
  float cw=cosf(w),sw=sinf(w),al=sw/(2.f*Q);
  float b0=1+al*A,b1=-2*cw,b2=1-al*A;
  float a0=1+al/A,a1=-2*cw,a2=1-al/A;
  float y=(b0/a0)*x+(b1/a0)*x1+(b2/a0)*x2-(a1/a0)*y1-(a2/a0)*y2;
  x2=x1;x1=x;y2=y1;y1=y; return y;
}

float _apf(float x,float c,float &xp,float &yp){
  float y=-c*x+xp+c*yp; xp=x;yp=y; return y;
}

// ─────────────────────────────────────────────
//  Effect name lookup
// ─────────────────────────────────────────────
const char* effectName(int e){
  const char* n[]={
    "Bypass","Chorus","Phaser","Tremolo","Vibrato",
    "Delay","Reverb","EQ","Compress","Gate",
    "Octave","Pitch","Wah","Fuzz","Overdrive",
    "Distort","Flanger","RingMod","Volume"
  };
  return (e>=0&&e<19)?n[e]:"?";
}

// ─────────────────────────────────────────────
//  Individual Effects
// ─────────────────────────────────────────────

float fx_bypass(float x){ return x; }

float fx_chorus(float x){
  _phCh+=2*M_PI*state.params.chorusRate/SAMPLE_RATE;
  if(_phCh>2*M_PI)_phCh-=2*M_PI;
  float d=(0.02f+state.params.chorusDepth*0.02f*sinf(_phCh))*SAMPLE_RATE;
  _chorusBuf[_wChorus%CHORUS_SIZE]=x;
  float w=_interp(_chorusBuf,CHORUS_SIZE,_wChorus,d);
  _wChorus=(_wChorus+1)%CHORUS_SIZE;
  return x*(1-state.params.chorusMix)+w*state.params.chorusMix;
}

float fx_phaser(float x){
  _phPh+=2*M_PI*state.params.phaserRate/SAMPLE_RATE;
  if(_phPh>2*M_PI)_phPh-=2*M_PI;
  float lfo=sinf(_phPh)*0.5f+0.5f;
  float fc=200.f+lfo*3000.f*state.params.phaserDepth;
  float t=tanf(M_PI*fc/SAMPLE_RATE);
  float c=(t-1)/(t+1);
  float y=x;
  for(int i=0;i<4;i++) y=_apf(y,c,_apX[i],_apY[i]);
  return x*(1-state.params.phaserFeedback)+y*state.params.phaserFeedback;
}

float fx_tremolo(float x){
  _phTr+=2*M_PI*state.params.tremoloRate/SAMPLE_RATE;
  if(_phTr>2*M_PI)_phTr-=2*M_PI;
  float lfo=state.params.tremoloSquare?
    (sinf(_phTr)>0?1.f:0.f):0.5f+0.5f*sinf(_phTr);
  return x*(1-state.params.tremoloDepth+state.params.tremoloDepth*lfo);
}

float fx_vibrato(float x){
  _phVi+=2*M_PI*state.params.vibratoRate/SAMPLE_RATE;
  if(_phVi>2*M_PI)_phVi-=2*M_PI;
  float d=(0.005f+state.params.vibratoDepth*0.005f*sinf(_phVi))*SAMPLE_RATE;
  _chorusBuf[_wChorus%CHORUS_SIZE]=x;
  float w=_interp(_chorusBuf,CHORUS_SIZE,_wChorus,d);
  _wChorus=(_wChorus+1)%CHORUS_SIZE;
  return w;
}

float fx_delay(float x){
  int ds=(int)(state.params.delayTime*SAMPLE_RATE);
  if(ds>=MAX_DELAY_SAMP)ds=MAX_DELAY_SAMP-1;
  int rp=(_wDelay-ds+MAX_DELAY_SAMP)%MAX_DELAY_SAMP;
  float echo=_delayBuf[rp];
  _delayBuf[_wDelay]=x+echo*state.params.delayFeedback;
  _wDelay=(_wDelay+1)%MAX_DELAY_SAMP;
  return x*(1-state.params.delayMix)+echo*state.params.delayMix;
}

float fx_reverb(float x){
  static float c1[1800]={},c2[2700]={},c3[3600]={};
  static int p1=0,p2=0,p3=0;
  float d=state.params.reverbDecay;
  float w=c1[p1]*0.33f+c2[p2]*0.33f+c3[p3]*0.33f;
  c1[p1]=x+c1[p1]*d; p1=(p1+1)%1800;
  c2[p2]=x+c2[p2]*d; p2=(p2+1)%2700;
  c3[p3]=x+c3[p3]*d; p3=(p3+1)%3600;
  static float ap[419]={}; static int app=0;
  float ao=ap[app]+w*(-0.7f);
  ap[app]=w+ao*0.7f; app=(app+1)%419;
  return x*(1-state.params.reverbMix)+ao*state.params.reverbMix;
}

float fx_eq(float x){
  float y=x;
  y=_biquad(y,_bBx1,_bBx2,_bBy1,_bBy2, 100.f,state.params.eqBass,  0.7f);
  y=_biquad(y,_bMx1,_bMx2,_bMy1,_bMy2,1000.f,state.params.eqMid,   0.7f);
  y=_biquad(y,_bTx1,_bTx2,_bTy1,_bTy2,6000.f,state.params.eqTreble,0.7f);
  return y;
}

float fx_comp(float x){
  float ea=expf(-1.f/(0.003f*SAMPLE_RATE));
  float er=expf(-1.f/(0.1f *SAMPLE_RATE));
  float ax=fabsf(x);
  _compE=ax>_compE?ea*_compE+(1-ea)*ax:er*_compE+(1-er)*ax;
  float tg=(_compE>state.params.compThreshold&&_compE>0.0001f)?
    state.params.compThreshold/_compE*
    powf(_compE/state.params.compThreshold,1.f/state.params.compRatio):1.f;
  float ga=expf(-1.f/(0.001f*SAMPLE_RATE));
  float gr=expf(-1.f/(0.05f *SAMPLE_RATE));
  _compG=tg<_compG?ga*_compG+(1-ga)*tg:gr*_compG+(1-gr)*tg;
  return x*_compG*(1+(state.params.compRatio-1)*0.3f);
}

float fx_gate(float x){
  float ea=expf(-1.f/(0.001f*SAMPLE_RATE));
  float er=expf(-1.f/(0.05f *SAMPLE_RATE));
  float ax=fabsf(x);
  _gateE=ax>_gateE?ea*_gateE+(1-ea)*ax:er*_gateE+(1-er)*ax;
  if(_gateE<state.params.gateThreshold){
    float r=_gateE/state.params.gateThreshold;
    return x*r*r;
  }
  return x;
}

float fx_octave(float x){
  if((x>0)!=(_octP>0)) _octF=-_octF;
  _octP=x;
  _octS=_octS*0.99f+_octF*fabsf(x)*0.01f;
  return x*state.params.octaveDry+_octS*state.params.octaveWet;
}

float fx_pitch(float x){
  static float pb[2048]={}; static float rp=0; static int wp=0;
  float ratio=powf(2.f,state.params.pitchSemitones/12.f);
  pb[wp%2048]=x; rp+=ratio;
  if(rp>=2048.f)rp-=2048.f;
  if(rp<0.f)rp+=2048.f;
  int r0=(int)rp,r1=(r0+1)%2048; float fr=rp-r0;
  float o=pb[r0]*(1-fr)+pb[r1]*fr;
  wp=(wp+1)%2048;
  return o*0.7f+x*0.3f;
}

float fx_wah(float x){
  // Expression pedal controls wah frequency
  float fc=state.exprWahFreq;
  _phWa+=2*M_PI*1.5f/SAMPLE_RATE;
  if(_phWa>2*M_PI)_phWa-=2*M_PI;
  static float svL=0,svB=0;
  float F=2*sinf(M_PI*fc/SAMPLE_RATE);
  float n=x-0.3f*svB;
  svL+=F*svB; svB=F*(n-svL)+svB;
  return svB*1.8f;
}

float fx_fuzz(float x){
  float y=_clip(x*8.f+0.1f,0.9f);
  y=_clip(y*6.f);
  static float fp=0,fd=0;
  fd=0.995f*fd+y-fp; fp=y; return fd*0.4f;
}

float fx_overdrive(float x){
  float y=_soft(x*3.f);
  y=y*0.5f+y*y*0.25f+y*y*y*0.25f;
  static float op=0,od=0;
  od=0.995f*od+y-op; op=y;
  return _clip(od)*0.6f;
}

float fx_dist(float x){
  float y=fabsf(x*5.f)*2.f-1.f;
  y=_clip(y*2.f);
  static float dp=0,dd=0;
  dd=0.995f*dd+y-dp; dp=y; return dd*0.35f;
}

float fx_flanger(float x){
  _phFl+=2*M_PI*0.25f/SAMPLE_RATE;
  if(_phFl>2*M_PI)_phFl-=2*M_PI;
  float d=(0.001f+0.004f*(sinf(_phFl)*0.5f+0.5f))*SAMPLE_RATE;
  _flangerBuf[_wFlanger%1024]=x;
  float w=_interp(_flangerBuf,1024,_wFlanger,d);
  _wFlanger=(_wFlanger+1)%1024;
  return (x+w)*0.5f;
}

float fx_ring(float x){
  _phRi+=2*M_PI*440.f/SAMPLE_RATE;
  if(_phRi>2*M_PI)_phRi-=2*M_PI;
  return x*sinf(_phRi);
}

float fx_volume(float x){ return x*state.params.volume; }

// ─────────────────────────────────────────────
//  Dispatch
// ─────────────────────────────────────────────
float applyEffect(float x, int id){
  switch(id){
    case 0:  return fx_bypass(x);
    case 1:  return fx_chorus(x);
    case 2:  return fx_phaser(x);
    case 3:  return fx_tremolo(x);
    case 4:  return fx_vibrato(x);
    case 5:  return fx_delay(x);
    case 6:  return fx_reverb(x);
    case 7:  return fx_eq(x);
    case 8:  return fx_comp(x);
    case 9:  return fx_gate(x);
    case 10: return fx_octave(x);
    case 11: return fx_pitch(x);
    case 12: return fx_wah(x);
    case 13: return fx_fuzz(x);
    case 14: return fx_overdrive(x);
    case 15: return fx_dist(x);
    case 16: return fx_flanger(x);
    case 17: return fx_ring(x);
    case 18: return fx_volume(x);
    default: return x;
  }
}
