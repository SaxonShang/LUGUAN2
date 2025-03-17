// #ifndef EFFECT_H
// #define EFFECT_H

// #include "pin_definitions.h"
// #include <Arduino.h>

// extern setting settings; // 声明使用外部定义的settings


// // // **音频效果控制结构**
// // struct EffectSettings {
// //     bool reverb_on = false;
// //     int reverb_strength = 50;  // **混响强度**

// //     bool distortion_on = false;
// //     int distortion_strength = 150;  // **失真强度**

// //     bool chorus_on = false;
// //     int chorus_strength = 150;  // **合唱强度**
// // };



// // // **初始化音效模块**
// // void initEffects() {
// //     effects.reverb_on = false;
// //     effects.distortion_on = false;
// //     effects.chorus_on = false;

// //     effects.reverb_strength = 50;
// //     effects.distortion_strength = 150;
// //     effects.chorus_strength = 150;
// // }

// // **Reverb**
// // Reverb (混响)
// const int REVERB_BUFFER_SIZE = 256;
// float reverbBuffer[REVERB_BUFFER_SIZE] = {0};
// int reverbIndex = 0;
// float applyReverb(float inputSample) {
//     if (!settings.reverb_on) return inputSample;

//     float decayFactor = map(settings.reverb_strength, 0, 255, 10, 70) / 100.0f;
//     float delayedSample = reverbBuffer[reverbIndex];
//     float outputSample = inputSample + delayedSample * decayFactor;

//     reverbBuffer[reverbIndex] = outputSample * 0.7; // 防止无限累积
//     reverbIndex = (reverbIndex + 1) % REVERB_BUFFER_SIZE;

//     return constrain(outputSample, -1.0, 1.0);
// }




// // **Distortion (失真)**
// float applyDistortion(float inputSample) {
//     if (!settings.distortion_on) return inputSample;

//     float gain = (settings.distortion_strength / 255.0f) * 10.0f + 1.0f; // 适当增益
//     float clippedSample = tanh(inputSample * gain);
//     // Serial.print("Distortion output: ");
//     // Serial.println(clippedSample);

//     return constrain(clippedSample, -1.0f, 1.0f);
// }



// // Chorus (合唱)
// const int CHORUS_BUFFER_SIZE = 512;
// float chorusBuffer[CHORUS_BUFFER_SIZE] = {0};
// int chorusIndex = 0;
// float chorusLFOPhase = 0;

// float applyChorus(float inputSample) {
//     if (!settings.chorus_on) return inputSample;

//     float modulation = sin(2 * PI * millis() * 0.001);
//     int delaySamples = map(settings.chorus_strength, 0, 255, 5, 15);
//     delaySamples += int(modulation * 3); // 更温和的LFO调制

//     int delayedIndex = (chorusIndex - delaySamples + CHORUS_BUFFER_SIZE) % CHORUS_BUFFER_SIZE;
//     float delayedSample = chorusBuffer[delayedIndex];

//     chorusBuffer[chorusIndex] = inputSample;
//     chorusIndex = (chorusIndex + 1) % CHORUS_BUFFER_SIZE;

//     return inputSample * 0.75f + delayedSample * 0.25f;
// }




// void initEffects() {
//     settings.reverb_on = false;
//     settings.distortion_on = false;
//     settings.chorus_on = false;

//     settings.reverb_strength = 50;
//     settings.distortion_strength = 150;
//     settings.chorus_strength = 150;

//     memset(reverbBuffer, 0, sizeof(reverbBuffer));
//     memset(chorusBuffer, 0, sizeof(chorusBuffer));
//     reverbIndex = 0;
//     chorusIndex = 0;
// }


// // **应用所有音效**
// // float applyEffects(float inputSample) {
// //     inputSample = applyDistortion(inputSample);
// //     inputSample = applyChorus(inputSample);
// //     inputSample = applyReverb(inpeffect_utSample);
// //     return inputSample;
// // }

// float applyEffects(float inputSample) {
//     float wetSignal = inputSample;

//     if (settings.reverb_on) wetSignal = applyReverb(inputSample);
//     if (settings.distortion_on) wetSignal = applyDistortion(inputSample) ;
//     if (settings.chorus_on) wetSignal = applyChorus(inputSample);

//     // 干湿信号比例7:3
//     float output = inputSample * 0.7 + wetSignal * 0.3;

//     return output;
// }



// #endif

#ifndef EFFECT_H
#define EFFECT_H

#include "pin_definitions.h"
#include <Arduino.h>

extern setting settings; // 使用外部定义的settings

// 缓冲区定义
const int REVERB_BUFFER_SIZE = 256;
float reverbBuffer[REVERB_BUFFER_SIZE] = {0};
int reverbIndex = 0;

const int CHORUS_BUFFER_SIZE = 512;
float chorusBuffer[CHORUS_BUFFER_SIZE] = {0};
int chorusIndex = 0;

// 初始化音效模块
void initEffects() {
    settings.reverb_on = false;
    settings.distortion_on = false;
    settings.chorus_on = false;

    settings.reverb_strength = 50;
    settings.distortion_strength = 150;
    settings.chorus_strength = 150;

    memset(reverbBuffer, 0, sizeof(reverbBuffer));
    memset(chorusBuffer, 0, sizeof(chorusBuffer));
    reverbIndex = 0;
    chorusIndex = 0;
}

// **混响 (Reverb)**
// const int REVERB_BUFFER_SIZE = 256;
float applyReverb(float inputSample) {
    if (!settings.reverb_on) return inputSample;

    float decayFactor = map(settings.reverb_strength, 0, 255, 20, 80) / 100.0f; // 更合理的衰减映射
    float delayedSample = reverbBuffer[reverbIndex];

    float outputSample = inputSample + delayedSample * decayFactor;
    reverbBuffer[reverbIndex] = outputSample * decayFactor; // 防止过度叠加
    reverbIndex = (reverbIndex + 1) % REVERB_BUFFER_SIZE;

    return constrain(outputSample, -1.0f, 1.0f);
}

// 失真效果 (Distortion)
float applyDistortion(float inputSample) {
    if (!settings.distortion_on) return inputSample;

    float gain = map(settings.distortion_strength, 0, 255, 30, 100) / 10.0f; // 合理增益
    float outputSample = tanh(inputSample * gain);

    return constrain(outputSample, -1.0f, 1.0f);
}

// 合唱效果 (Chorus)
// const int CHORUS_BUFFER_SIZE = 512;
float applyChorus(float inputSample) {
    if (!settings.chorus_on) return inputSample;

    float lfo = sin(2 * PI * millis() * 0.002);  // 更平滑LFO
    int delaySamples = map(settings.chorus_strength, 0, 255, 5, 25);
    delaySamples += int(lfo * 4);  // 动态变化范围更平滑

    int delayedIndex = (chorusIndex - delaySamples + CHORUS_BUFFER_SIZE) % CHORUS_BUFFER_SIZE;
    float delayedSample = chorusBuffer[delayedIndex];

    chorusBuffer[chorusIndex] = inputSample;
    chorusIndex = (chorusIndex + 1) % CHORUS_BUFFER_SIZE;

    return constrain(inputSample * 0.7f + delayedSample * 0.3f, -1.0f, 1.0f);
}

// **应用所有音效，干湿混合处理**
float applyEffects(float inputSample) {
    float wetSignal = 0.0f;

    if (settings.reverb_on)
        wetSignal += applyReverb(inputSample) * (settings.reverb_strength / 255.0f) * 0.5f;

    if (settings.distortion_on)
        wetSignal += applyDistortion(inputSample) * (settings.distortion_strength / 255.0f) * 0.3f;

    if (settings.chorus_on)
        wetSignal += applyChorus(inputSample) * (settings.chorus_strength / 255.0f) * 0.3f;

    // 合理的干湿混合比例（70% 干，30% 湿）
    float output = (inputSample * 0.7f) + (wetSignal * 0.3f);

    return output;
}

#endif
