// Generated from pinned AudioKit public source. MIT notices in ../notices/SynthOne-MIT.txt.
#pragma once
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <stdint.h>
#include <stddef.h>
extern "C" {
#include "soundpipe.h"
#include "soundpipeextension.h"
}
#define nil nullptr
#define S1_NUM_WAVEFORMS 4
#define S1_NUM_BANDLIMITED_FTABLES 13
static inline float pow2(float x){return x*x;}
template<typename T>static inline T clamp(T x,T a,T b){return x<a?a:x>b?b:x;}
//
//  S1Parameter.h
//  AudioKit
//
//  Created by AudioKit Contributors on 12/24/17.
//  Copyright © 2017 AudioKit. All rights reserved.
//



#pragma once

enum S1Parameter {
    index1 = 0,
    index2 = 1,
    morphBalance = 2,
    morph1SemitoneOffset = 3,
    morph2SemitoneOffset = 4,
    morph1Volume = 5,
    morph2Volume = 6,
    subVolume = 7,
    subOctaveDown = 8,
    subIsSquare = 9,
    fmVolume = 10,
    fmAmount = 11,
    noiseVolume = 12,
    lfo1Index = 13,
    lfo1Amplitude = 14,
    lfo1Rate = 15, // hz
    cutoff = 16,
    resonance = 17,
    filterMix = 18, // unused...hard coded to 1
    filterADSRMix = 19,
    isMono = 20,
    glide = 21,
    filterAttackDuration = 22,
    filterDecayDuration = 23,
    filterSustainLevel = 24,
    filterReleaseDuration = 25,
    attackDuration = 26,
    decayDuration = 27,
    sustainLevel = 28,
    releaseDuration = 29,
    morph2Detuning = 30,
    detuningMultiplier = 31,
    masterVolume = 32,
    bitCrushDepth = 33,//bitCrushDepth is unused
    bitCrushSampleRate = 34,
    autoPanAmount = 35,
    autoPanFrequency = 36,
    reverbOn = 37,
    reverbFeedback = 38,
    reverbHighPass = 39,
    reverbMix = 40,
    delayOn = 41,
    delayFeedback = 42,
    delayTime = 43,
    delayMix = 44,
    lfo2Index = 45,
    lfo2Amplitude = 46,
    lfo2Rate = 47, // hz
    cutoffLFO = 48,
    resonanceLFO = 49,
    oscMixLFO = 50,
    reverbMixLFO = 51,
    decayLFO = 52,
    noiseLFO = 53,
    fmLFO = 54,
    detuneLFO = 55,
    filterEnvLFO = 56,
    pitchLFO = 57,
    bitcrushLFO = 58,
    tremoloLFO = 59,
    arpDirection = 60,
    arpInterval = 61,
    arpIsOn = 62,
    arpOctave = 63,
    arpRate = 64, // rate in bpm, not hz
    arpIsSequencer = 65,
    arpTotalSteps = 66,
    sequencerPattern00 = 67,
    sequencerPattern01 = 68,
    sequencerPattern02 = 69,
    sequencerPattern03 = 70,
    sequencerPattern04 = 71,
    sequencerPattern05 = 72,
    sequencerPattern06 = 73,
    sequencerPattern07 = 74,
    sequencerPattern08 = 75,
    sequencerPattern09 = 76,
    sequencerPattern10 = 77,
    sequencerPattern11 = 78,
    sequencerPattern12 = 79,
    sequencerPattern13 = 80,
    sequencerPattern14 = 81,
    sequencerPattern15 = 82,
    sequencerOctBoost00 = 83,
    sequencerOctBoost01 = 84,
    sequencerOctBoost02 = 85,
    sequencerOctBoost03 = 86,
    sequencerOctBoost04 = 87,
    sequencerOctBoost05 = 88,
    sequencerOctBoost06 = 89,
    sequencerOctBoost07 = 90,
    sequencerOctBoost08 = 91,
    sequencerOctBoost09 = 92,
    sequencerOctBoost10 = 93,
    sequencerOctBoost11 = 94,
    sequencerOctBoost12 = 95,
    sequencerOctBoost13 = 96,
    sequencerOctBoost14 = 97,
    sequencerOctBoost15 = 98,
    sequencerNoteOn00 = 99,
    sequencerNoteOn01 = 100,
    sequencerNoteOn02 = 101,
    sequencerNoteOn03 = 102,
    sequencerNoteOn04 = 103,
    sequencerNoteOn05 = 104,
    sequencerNoteOn06 = 105,
    sequencerNoteOn07 = 106,
    sequencerNoteOn08 = 107,
    sequencerNoteOn09 = 108,
    sequencerNoteOn10 = 109,
    sequencerNoteOn11 = 110,
    sequencerNoteOn12 = 111,
    sequencerNoteOn13 = 112,
    sequencerNoteOn14 = 113,
    sequencerNoteOn15 = 114,
    filterType = 115,
    phaserMix = 116,
    phaserRate = 117,
    phaserFeedback = 118,
    phaserNotchWidth = 119,
    monoIsLegato = 120, // range [0 = legato, 1 = portamento (default)]
    widen = 121,
    compressorMasterRatio = 122,
    compressorReverbInputRatio = 123,
    compressorReverbWetRatio = 124,
    compressorMasterThreshold = 125,
    compressorReverbInputThreshold = 126,
    compressorReverbWetThreshold = 127,
    compressorMasterAttack = 128,
    compressorReverbInputAttack = 129,
    compressorReverbWetAttack = 130,
    compressorMasterRelease = 131,
    compressorReverbInputRelease = 132,
    compressorReverbWetRelease = 133,
    compressorMasterMakeupGain = 134,
    compressorReverbInputMakeupGain = 135,
    compressorReverbWetMakeupGain = 136,
    delayInputCutoffTrackingRatio = 137,
    delayInputResonance = 138,
    tempoSyncToArpRate = 139,
    pitchbend = 140,
    pitchbendMinSemitones = 141,
    pitchbendMaxSemitones = 142,
    frequencyA4 = 143,
    portamentoHalfTime = 144,

    oscBandlimitIndexOverride = 145, // deprecated
    oscBandlimitEnable = 146,

    arpSeqTempoMultiplier = 147,

    transpose = 148,

    adsrPitchTracking = 149,

    S1ParameterCount = 150
};


static const float minimums[150]={0.0f,0.0f,0.0f,-12.0f,-12.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.000520833333f,64.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0005f,0.005f,0.0f,0.0f,0.0005f,0.005f,0.0f,0.004f,-4.0f,1.0f,0.0f,1.0f,2048.0f,0.0f,0.000520833333f,0.0f,0.0f,80.0f,0.0f,0.0f,0.0f,0.000362811791f,0.0f,0.0f,0.0f,0.000520833333f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,1.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,-12.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,100.0f,0.0f,0.0f,1.0f,1.0f,1.0f,-60.0f,-60.0f,-60.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.5f,0.5f,0.5f,0.5f,0.0f,0.0f,0.0f,-24.0f,0.0f,410.0f,1e-06f,-1.0f,0.0f,0.0104166667f,-24.0f,0.0f};
static const float defaults[150]={1.0f,1.0f,0.5f,0.0f,0.0f,0.8f,0.8f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.25f,20000.0f,0.1f,1.0f,0.0f,0.0f,0.0f,0.05f,0.05f,1.0f,0.5f,0.05f,0.005f,0.8f,0.05f,0.0f,1.0f,0.5f,24.0f,48000.0f,0.0f,0.25f,1.0f,0.5f,700.0f,0.0f,0.0f,0.1f,0.25f,0.125f,0.0f,0.0f,0.25f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,12.0f,0.0f,1.0f,120.0f,0.0f,4.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,12.0f,0.0f,800.0f,0.0f,0.0f,20.0f,13.0f,13.0f,-9.0f,-8.5f,-8.0f,0.001f,0.001f,0.001f,0.15f,0.225f,0.15f,2.0f,1.88f,1.88f,0.75f,0.0f,1.0f,8192.0f,-12.0f,12.0f,440.0f,0.1f,-1.0f,0.0f,0.25f,0.0f,0.0f};
static const float maximums[150]={1.0f,1.0f,1.0f,24.0f,24.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,15.0f,0.25f,3.0f,1.0f,80.0f,22050.0f,0.98f,1.0f,1.2f,1.0f,0.2f,2.0f,2.0f,1.0f,2.0f,2.0f,2.0f,1.0f,2.0f,4.0f,2.0f,2.0f,24.0f,48000.0f,1.0f,10.0f,1.0f,1.0f,900.0f,1.0f,1.0f,0.9f,2.5f,1.0f,3.0f,1.0f,80.0f,3.0f,3.0f,3.0f,3.0f,3.0f,3.0f,3.0f,3.0f,3.0f,3.0f,3.0f,3.0f,2.0f,12.0f,1.0f,3.0f,200.0f,1.0f,16.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,12.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,2.0f,1.0f,300.0f,0.8f,1000.0f,1.0f,1.0f,20.0f,20.0f,20.0f,0.0f,0.0f,0.0f,0.01f,0.01f,0.01f,0.5f,0.5f,0.5f,4.0f,4.0f,4.0f,1.0f,0.98f,1.0f,16383.0f,0.0f,24.0f,470.0f,0.99f,12.0f,1.0f,8.0f,24.0f,1.0f};
class S1DSPKernel {
public:
 float parameters[150];sp_data*sp;sp_ftbl*ft_array[52];float ft_frequencyBand[13];sp_ftbl*sine;
 float lfo1_0_1=0,lfo1_1_0=1,lfo2_0_1=0,lfo2_1_0=1,lfo3_0_1=0,lfo3_1_0=1,monoFrequencySmooth=261.6255653006f;
 sp_phasor*lfo1Phasor,*lfo2Phasor;sp_port*lfo1Port,*lfo2Port;float lfo1Smooth=0,lfo2Smooth=0;
 sp_data*spp(){return sp;}int sampleRate()const{return sp->sr;}
 float clampedValue(S1Parameter p,float x){return clamp(x,minimums[p],maximums[p]);}
 void processLfo();
};
struct S1NoteState {
    
    S1DSPKernel* kernel;
    
    enum NoteStateStage { stageOff, stageOn, stageRelease };
    NoteStateStage stage = stageOff;
    
    float internalGate = 0;

    // velocity stores the original velocity...not used in "process"
    int velocity;

    // amp is a transform of velocity and is used in "process"
    float amp = 0;

    float filter = 0;

    // -1 denotes an invalid note number
    int rootNoteNumber = 0;

    // used for frequency look up and UI
    int transpose = 0;
    
    //Amplitude ADSR
    sp_adsr *adsr;
    
    //Filter Cutoff Frequency ADSR
    sp_adsr *fadsr;
    
    //Morphing Oscillator 1 & 2
    sp_oscmorph2d *oscmorph1;
    sp_oscmorph2d *oscmorph2;

    sp_crossfade *morphCrossFade;
    
    //Subwoofer OSC
    sp_osc *subOsc;
    
    //FM OSC
    sp_fosc *fmOsc;
    
    //NOISE OSC
    sp_noise *noise;
    
    //FILTERS
    sp_moogladder *loPass;
    sp_buthp *hiPass;
    sp_butbp *bandPass;
    sp_crossfade *filterCrossFade;
    
    inline float getParam(S1Parameter param);
    inline int sampleRate() const;

    void init();

    void destroy();
    
    void clear();

    void startNoteHelper(int noteNumber, int velocity, float frequency);

    void run(int frameIndex, float *outL, float *outR);
};
