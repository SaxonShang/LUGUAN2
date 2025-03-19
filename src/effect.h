#ifndef EFFECT_H
#define EFFECT_H

#include "pin.h"
#include <Arduino.h>

//------------------------------------------------------------------------------
// External Settings Declaration
//------------------------------------------------------------------------------
// Externally defined settings instance
extern setting settings;

//------------------------------------------------------------------------------
// Buffer Definitions for Audio Effects
//------------------------------------------------------------------------------

// Reverb buffer
const int REVERB_BUFFER_SIZE = 256;
float reverbBuffer[REVERB_BUFFER_SIZE] = {0};
int reverbIndex = 0;

// Chorus buffer
const int CHORUS_BUFFER_SIZE = 512;
float chorusBuffer[CHORUS_BUFFER_SIZE] = {0};
int chorusIndex = 0;

//------------------------------------------------------------------------------
// Initialization of Audio Effects Module
//------------------------------------------------------------------------------
void initEffects() {
    // Disable all effects initially
    settings.reverb_on = false;
    settings.distortion_on = false;
    settings.chorus_on = false;

    // Set default strength values
    settings.reverb_strength = 5;
    settings.distortion_strength = 5;
    settings.chorus_strength = 5;

    // Clear the effect buffers
    memset(reverbBuffer, 0, sizeof(reverbBuffer));
    memset(chorusBuffer, 0, sizeof(chorusBuffer));
    reverbIndex = 0;
    chorusIndex = 0;
}

//------------------------------------------------------------------------------
// Reverb Effect
//------------------------------------------------------------------------------
float applyReverb(float inputSample) {

    if (!settings.reverb_on) return inputSample;

    float decayFactor = 0.2f + (settings.reverb_strength * 0.06f);

    float delayedSample = reverbBuffer[reverbIndex];

    float outputSample = inputSample + delayedSample * decayFactor;

    reverbBuffer[reverbIndex] = (reverbBuffer[reverbIndex] * 0.8f) + (outputSample * 0.2f);

    reverbIndex++;
    if (reverbIndex >= REVERB_BUFFER_SIZE) reverbIndex = 0;

    return (outputSample < -1.0f) ? -1.0f : (outputSample > 1.0f ? 1.0f : outputSample);
}

//------------------------------------------------------------------------------
// Distortion Effect
//------------------------------------------------------------------------------
float applyDistortion(float inputSample) {
    if (!settings.distortion_on) return inputSample;

    float gain = 3.0f + (settings.distortion_strength * 0.7f);

    float outputSample = tanh(fmin(fmax(inputSample * gain, -10.0f), 10.0f));

    return outputSample;
}


//------------------------------------------------------------------------------
// Chorus Effect
//------------------------------------------------------------------------------
float applyChorus(float inputSample) {

    if (!settings.chorus_on) return inputSample;

    static float chorusPhase = 0.0f;
    float lfo = sinf(chorusPhase);
    chorusPhase += 0.01f;
    if (chorusPhase > 2 * PI) chorusPhase -= 2 * PI;
    float strengthFactor = settings.chorus_strength / 10.0f;
    int delaySamples = 5 + strengthFactor * 20;
    delaySamples += int(lfo * 3.0f);

    int delayedIndex = (chorusIndex - delaySamples + CHORUS_BUFFER_SIZE) % CHORUS_BUFFER_SIZE;
    float delayedSample = chorusBuffer[delayedIndex];

    chorusBuffer[chorusIndex] = inputSample;
    chorusIndex = (chorusIndex + 1) % CHORUS_BUFFER_SIZE;

    return inputSample * 0.7f + delayedSample * 0.3f;
}


//------------------------------------------------------------------------------
// Apply All Audio Effects with Dry/Wet Mixing
//------------------------------------------------------------------------------
float applyEffects(float inputSample) {
    float wetSignal = 0.0f;

    if (settings.reverb_on)
        wetSignal += applyReverb(inputSample) * (settings.reverb_strength / 255.0f) * 0.5f;

    if (settings.distortion_on)
        wetSignal += applyDistortion(inputSample) * (settings.distortion_strength / 255.0f) * 0.3f;

    if (settings.chorus_on)
        wetSignal += applyChorus(inputSample) * (settings.chorus_strength / 255.0f) * 0.3f;

    // Combine dry and wet signals with a mix ratio of 70% dry and 30% wet
    float output = (inputSample * 0.3f) + (wetSignal * 0.7f);

    return output;
}

#endif
