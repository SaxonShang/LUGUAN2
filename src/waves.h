#include <math.h>
#include <stdlib.h>
#include <pin.h>
#include "effect.h"  // Include audio effect utilities

#define SAMPLE_RATE 22000
#define AMPLITUDE 0.5
#define TABLE_SIZE 256
#define PI M_PI

// -------------------- Global Parameters --------------------
float dt = 1.0f / SAMPLE_RATE;
float _prevCutoff = 500;
float _rc = 1.0f / (2.0f * PI * 500);
float _alpha = dt / (_rc + dt);

float lfo_frequency = 5.0;
float lfo_depth = 0.01;
float LFOAcc = 0;
float _prevLfoFreq = 20;
float lfoPhaseStep = 20 * 2 * PI / SAMPLE_RATE;

float attack_time = 0.1;
float decay_time = 0.2;
float sustain_level = 0.6;
float release_time = 0.4;

float filter_cutoff = 0.5;
float filter_resonance = 0.2;
float filter_env_amount = 2.0;
float noise_level = 0.1;
float noise_filter_cutoff = 0.8;

// -------------------- Utility Functions --------------------
float convertPhaseToIndex(float* phaseAcc, float phaseIncr) {
    *phaseAcc += phaseIncr;
    if ((int)(*phaseAcc * TABLE_SIZE) >= TABLE_SIZE) *phaseAcc -= 1;
    return (int)(*phaseAcc * TABLE_SIZE) % TABLE_SIZE;
}

float bhaskaraSin(float x) {
    float numerator = 16 * x * (PI - x);
    float denominator = 5 * PI * PI - 4 * x * (PI - x);
    return numerator / denominator;
}

// -------------------- Core Generators --------------------
float generateSin(float sinPhase, float* phase) {
    float tmpPhase = *phase + sinPhase;
    if (tmpPhase >= PI) tmpPhase -= PI;
    *phase = tmpPhase;
    return sin(tmpPhase);
}

float getSample(float phaseIncr, float* phaseAcc, float table[]) {
    int index = convertPhaseToIndex(phaseAcc, phaseIncr);
    return table[index];
}

float generateLFO(int reduceVal, float lfoFreq) {
    if (lfoFreq != _prevLfoFreq) {
        lfoPhaseStep = lfoFreq * 2 * PI / SAMPLE_RATE;
        _prevLfoFreq = lfoFreq;
    }
    return getSample(lfoPhaseStep, &LFOAcc, sineTable) / reduceVal;
}

float calcSawtoothAmp(float* phaseAcc, int volume, int noteIndex) {
    float step = notePhases[noteIndex];
    *phaseAcc += step;
    if (*phaseAcc > 1) *phaseAcc -= 1;
    return *phaseAcc;
}

// -------------------- Envelope and Effects --------------------
int calcFade(int count, int sustainTime, int fadeSpeed) {
    return (count < sustainTime) ? 0 : (count - sustainTime) / fadeSpeed;
}

int adsrGeneral(int pressCount) {
    int atk = __atomic_load_n(&settings.adsr.attack, __ATOMIC_RELAXED);
    int dec = __atomic_load_n(&settings.adsr.decay, __ATOMIC_RELAXED) + atk;
    int sus = __atomic_load_n(&settings.adsr.sustain, __ATOMIC_RELAXED) + dec;
    if (pressCount < atk && pressCount > 0) return pressCount - atk / 2;
    else if (pressCount >= atk && pressCount <= dec) return pressCount - atk;
    else if (pressCount > dec && pressCount < sus) return dec - atk;
    else {
        int fade = __atomic_load_n(&settings.adsr.sustain, __ATOMIC_RELAXED) + dec;
        return (dec - atk) + (pressCount - sus) / fade;
    }
}

int adsrHorn(int pressCount) {
    static int lowCounter = 0, highCounter = 0;
    const int lowTime = 3, lowVal = 2, highVal = -2, loopTime = 20;
    if ((pressCount % loopTime) <= lowTime) {
        highCounter = 0;
        if (lowCounter < lowVal) lowCounter++;
        return lowCounter;
    } else {
        lowCounter = 0;
        if (highCounter > highVal) highCounter--;
        return highCounter;
    }
}

float lowPassFilter(float current, float previous, float cutoffFreq) {
    if (_prevCutoff != cutoffFreq) {
        _rc = 1.0f / (2.0f * PI * cutoffFreq);
        _alpha = dt / (_rc + dt);
        _prevCutoff = cutoffFreq;
    }
    return previous + _alpha * (current - previous);
}

// -------------------- Output Mapping --------------------
u_int32_t calcVout(float amp, int volume, int vshift) {
    uint32_t output = static_cast<uint32_t>(amp * 255) - 128;
    int shift = 8 - volume + vshift;
    return (shift >= 0) ? ((output + 128) >> shift) : ((output + 128) << -shift);
}

u_int32_t calcHornVout(float amp, int volume, int idx) {
    uint32_t vout = static_cast<uint32_t>(amp * 127) - 128;
    int shift = adsrHorn(notes.notes[idx].pressedCount);
    int volShift = 8 - volume + shift;
    return (volShift >= 0) ? ((vout + 128) >> volShift) : ((vout + 128) << -volShift);
}

// -------------------- Audio Effects Chain --------------------
u_int32_t addEffects(float amp, int volume, int idx) {
    int shiftVal = 0;
    bool fadeEnabled = __atomic_load_n(&settings.fade.on, __ATOMIC_RELAXED);
    bool adsrEnabled = __atomic_load_n(&settings.adsr.on, __ATOMIC_RELAXED);

    if (fadeEnabled)
        shiftVal = calcFade(notes.notes[idx].pressedCount, settings.fade.sustainTime, settings.fade.fadeSpeed);
    else if (adsrEnabled)
        shiftVal = adsrGeneral(notes.notes[idx].pressedCount);

    return calcVout(amp, volume, shiftVal);
}

u_int32_t addLFO(float amp, int volume) {
    if (__atomic_load_n(&settings.lfo.on, __ATOMIC_RELAXED)) {
        int lfoVol = __atomic_load_n(&settings.lfo.reduceLFOVolume, __ATOMIC_RELAXED);
        int lfoFreq = __atomic_load_n(&settings.lfo.freq, __ATOMIC_RELAXED);
        amp = generateLFO(lfoVol, lfoFreq);
        return calcVout(amp, volume, 0);
    }
    return 0;
}

float addLPF(float amp, float* prevAmp) {
    if (__atomic_load_n(&settings.lowpass.on, __ATOMIC_RELAXED)) {
        int cutoff = __atomic_load_n(&settings.lowpass.freq, __ATOMIC_RELAXED);
        amp = lowPassFilter(amp, *prevAmp, cutoff);
        *prevAmp = amp;
    }
    return amp;
}

// -------------------- Time Counter --------------------
void presssedTimeCount() {
    for (int i = 0; i < 96; i++) {
        bool active = __atomic_load_n(&notes.notes[i].active, __ATOMIC_RELAXED);
        notes.notes[i].pressedCount = active ? notes.notes[i].pressedCount + 1 : 0;
    }
}