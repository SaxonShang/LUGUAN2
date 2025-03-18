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
    settings.reverb_strength = 50;
    settings.distortion_strength = 150;
    settings.chorus_strength = 150;

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
    // If reverb is disabled, return the original sample
    if (!settings.reverb_on) return inputSample;

    // Map reverb strength to a decay factor (more reasonable decay mapping)
    float decayFactor = map(settings.reverb_strength, 0, 255, 20, 80) / 100.0f;
    float delayedSample = reverbBuffer[reverbIndex];

    // Combine the input with the delayed sample
    float outputSample = inputSample + delayedSample * decayFactor;
    // Store the processed sample to prevent excessive accumulation
    reverbBuffer[reverbIndex] = outputSample * decayFactor;
    reverbIndex = (reverbIndex + 1) % REVERB_BUFFER_SIZE;

    // Ensure the output remains within valid bounds
    return constrain(outputSample, -1.0f, 1.0f);
}

//------------------------------------------------------------------------------
// Distortion Effect
//------------------------------------------------------------------------------
float applyDistortion(float inputSample) {
    // If distortion is disabled, return the original sample
    if (!settings.distortion_on) return inputSample;

    // Map distortion strength to a gain value (appropriate gain)
    float gain = map(settings.distortion_strength, 0, 255, 30, 100) / 10.0f;
    float outputSample = tanh(inputSample * gain);

    // Ensure the output remains within valid bounds
    return constrain(outputSample, -1.0f, 1.0f);
}

//------------------------------------------------------------------------------
// Chorus Effect
//------------------------------------------------------------------------------
float applyChorus(float inputSample) {
    // If chorus is disabled, return the original sample
    if (!settings.chorus_on) return inputSample;

    // Calculate a smoother Low Frequency Oscillator (LFO)
    float lfo = sin(2 * PI * millis() * 0.002);
    // Map chorus strength to a base delay (in samples)
    int delaySamples = map(settings.chorus_strength, 0, 255, 5, 25);
    // Adjust delay dynamically with LFO for smoother variation
    delaySamples += int(lfo * 4);

    int delayedIndex = (chorusIndex - delaySamples + CHORUS_BUFFER_SIZE) % CHORUS_BUFFER_SIZE;
    float delayedSample = chorusBuffer[delayedIndex];

    // Store the current sample in the chorus buffer
    chorusBuffer[chorusIndex] = inputSample;
    chorusIndex = (chorusIndex + 1) % CHORUS_BUFFER_SIZE;

    // Mix the original and delayed signals (70% dry, 30% wet)
    return constrain(inputSample * 0.7f + delayedSample * 0.3f, -1.0f, 1.0f);
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
    float output = (inputSample * 0.7f) + (wetSignal * 0.3f);

    return output;
}

#endif
