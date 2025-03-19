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
    // 如果混响关闭，直接返回原始输入
    if (!settings.reverb_on) return inputSample;

    // 计算混响衰减系数（归一化 reverb_strength）
    float decayFactor = 0.2f + (settings.reverb_strength * 0.06f);  // 取值范围 0.2 - 0.8

    // 获取延迟样本
    float delayedSample = reverbBuffer[reverbIndex];

    // 计算混响后的输出
    float outputSample = inputSample + delayedSample * decayFactor;

    // 线性插值平滑混响，避免过度衰减
    reverbBuffer[reverbIndex] = (reverbBuffer[reverbIndex] * 0.8f) + (outputSample * 0.2f);

    // 更新环形缓冲区索引（优化 % 运算）
    reverbIndex++;
    if (reverbIndex >= REVERB_BUFFER_SIZE) reverbIndex = 0;

    // 限制范围，避免过载
    return (outputSample < -1.0f) ? -1.0f : (outputSample > 1.0f ? 1.0f : outputSample);
}

//------------------------------------------------------------------------------
// Distortion Effect
//------------------------------------------------------------------------------
float applyDistortion(float inputSample) {
    // 如果失真关闭，直接返回原始输入
    if (!settings.distortion_on) return inputSample;

    // 计算失真增益（取值范围 3.0 - 10.0）
    float gain = 3.0f + (settings.distortion_strength * 0.7f);

    // 计算失真效果，防止极端输入导致 NaN
    float outputSample = tanh(fmin(fmax(inputSample * gain, -10.0f), 10.0f));

    // 由于 tanh 本身限制范围 [-1,1]，不需要额外 constrain()
    return outputSample;
}


//------------------------------------------------------------------------------
// Chorus Effect
//------------------------------------------------------------------------------
float applyChorus(float inputSample) {
    // 如果合唱关闭，直接返回原始输入
    if (!settings.chorus_on) return inputSample;

    // 平滑 LFO 振荡（降低计算量）
    static float chorusPhase = 0.0f;
    float lfo = sinf(chorusPhase);
    chorusPhase += 0.01f; // 控制 LFO 频率
    if (chorusPhase > 2 * PI) chorusPhase -= 2 * PI;

    // 计算延迟（范围 5~25）
    float strengthFactor = settings.chorus_strength / 10.0f;  // 归一化到 [0, 1]
    int delaySamples = 5 + strengthFactor * 20;  // 计算基础延迟
    delaySamples += int(lfo * 3.0f);  // 平滑波动（±3）

    // 计算延迟索引（循环缓冲区）
    int delayedIndex = (chorusIndex - delaySamples + CHORUS_BUFFER_SIZE) % CHORUS_BUFFER_SIZE;
    float delayedSample = chorusBuffer[delayedIndex];

    // 存储当前样本
    chorusBuffer[chorusIndex] = inputSample;
    chorusIndex = (chorusIndex + 1) % CHORUS_BUFFER_SIZE;

    // 70% 干声，30% 湿声
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
