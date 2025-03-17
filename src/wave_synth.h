#include <math.h>
#include <stdlib.h>
#include <pin_definitions.h>
#include "effect.h"  // **Include effect.h to apply audio effects**

#define SAMPLE_RATE 22000  // **Sampling rate for audio synthesis**
#define AMPLITUDE 0.5      // **Base amplitude for audio signals**
#define TABLE_SIZE 256     // **Size of the lookup table for waveform generation**
#define PI M_PI            // **Define PI using math.h**

// ADSR envelope parameters (Attack, Decay, Sustain, Release)
float attack_time = 0.1;
float decay_time = 0.2;
float sustain_level = 0.6;
float release_time = 0.4;

// Filter parameters
float filter_cutoff = 0.5;        // **Cutoff frequency for the filter**
float filter_resonance = 0.2;     // **Resonance level of the filter**
float filter_env_amount = 2;      // **Amount of filter envelope applied**

// LFO (Low-Frequency Oscillator) parameters for pitch modulation
float lfo_frequency = 5.0;   // **LFO frequency in Hz**
float lfo_depth = 0.01;      // **Depth of LFO modulation**

// Noise parameters
float noise_level = 0.1;          // **Amplitude level of noise**
float noise_filter_cutoff = 0.8;  // **Filter cutoff for noise**

// Precompute filter parameters to optimize performance
float dt = 1.0f / SAMPLE_RATE;
float prevcutoffFreq = 500;
float rc = 1.0f / (2.0f * M_PI * 500);
float alpha = dt / (rc + dt);  // **Precomputed filter coefficient**

float lowPassFilter(float cur, float prev, float cutoffFreq) {
    if (prevcutoffFreq!=cutoffFreq){
        rc = 1.0f / (2.0f * M_PI * cutoffFreq);
        alpha = dt / (rc + dt);
    }

    prevcutoffFreq=cutoffFreq;
    // float 


    return  prev + alpha * (cur - prev);
}

// Bhaskara approximation for sine wave (less accurate but computationally efficient)
float bhaskaraSin(float x) {
    float numerator = 16 * x * (PI - x);
    float denominator = 5 * PI * PI - 4 * x * (PI - x);
    return numerator / denominator;// faster sin but actually slower
}
float generateSin( float sinPhase, float* phase){
    float testsinAcc=*phase;
    testsinAcc+=sinPhase;
    if (testsinAcc>=M_PI){
        testsinAcc-=M_PI;
    }
    *phase=testsinAcc;
    return sin(testsinAcc);
}


float getSample(float phaseIcre, float* phaseAcc, float table[]) {
    *phaseAcc+=phaseIcre;
    if (int(*phaseAcc*TABLE_SIZE)>TABLE_SIZE){
        *phaseAcc-=1;
    }
    int index = (int)(*phaseAcc * TABLE_SIZE);
    index = index % TABLE_SIZE;
    return table[index];
}
float LFOAcc=0;
float prevLFOfreq=20;
float lfoPhase=20*M_PI*2/SAMPLE_RATE;
float generateLFO(int reduceVal,float lfoFreq){
    // float lfoFreq=10.0;
    if(lfoFreq!=prevLFOfreq){
        lfoPhase=lfoFreq*M_PI*2/SAMPLE_RATE;
        
    }
    prevLFOfreq=lfoFreq;
    

    float amp=getSample(lfoPhase,&LFOAcc,sineTable)/reduceVal;
    



    return amp;
}

int calcFade(int pressedCount, int decaytime, int decayspeed){
    // int press=3;
    // int decay=4;
    if (pressedCount<decaytime){
        return 0;
    }
    else{
        return int( (pressedCount-decaytime)/decayspeed);
    }

}
float calcSawtoothAmp(float *phaseAcc,int volume, int i){
    // uint32_t stepSize=__atomic_load_n(&stepSizes[i%12],__ATOMIC_RELAXED); 
    float stepsize=notePhases[i];


    *phaseAcc+=stepsize;
    if (*phaseAcc>1){
        *phaseAcc-=1;
    }
    float amp=*phaseAcc;
    // uint32_t Vout = (*phaseAcc >> 24) - 128;
    // Vout = (Vout+128) >> (8 - volume)) ;

    return amp;
}




u_int32_t calcVout(float Amp,int volume, int vshift){
    uint32_t Vout = static_cast<uint32_t>(Amp *255) - 128;
    int volshift=8 - volume+vshift;
    if (volshift>=0){

    Vout = ((Vout+128) >> (volshift)) ;}
    else {Vout = ((Vout+128) << -(volshift)) ;}

    return Vout;
}



int adsrGeneral(int pressedCount){
    int attack=__atomic_load_n(&settings.adsr.attack, __ATOMIC_RELAXED);
    int decay=__atomic_load_n(&settings.adsr.decay, __ATOMIC_RELAXED)+attack;
    int sustain=__atomic_load_n(&settings.adsr.sustain, __ATOMIC_RELAXED)+decay;
    if (pressedCount<attack && pressedCount>0){
        return pressedCount-int(attack/2);
    }
    else if (pressedCount>= attack && decay>=pressedCount){
        return int( (pressedCount-attack));
    }
    else if(pressedCount>decay && pressedCount<sustain){
        return int( (decay-attack));
    }

    else{
        int fadeSpeed=__atomic_load_n(&settings.adsr.sustain, __ATOMIC_RELAXED)+decay;
        return (decay-attack)+(pressedCount-sustain)/fadeSpeed;
    }
}

// ADSR envelope for horn-like sounds
int adsrHorn(int pressedCount) {
    int lowtime = 3;
    int lowval = 2;
    int highval = -2;
    int loopduration = 20;
    static int countlow = 0;
    static int counthigh = 0;
    if ((pressedCount % loopduration) <= lowtime) {
        counthigh = 0;
        if (countlow < lowval) {
            countlow += 1;
        }
        return countlow;
    } else {
        countlow = 0;
        if (counthigh > highval) {
            counthigh -= 1;
        }
        return counthigh;
    }
}

// u_int32_t calcPianoVout(float Amp,int volume, int i){
//     // Amp+=generateLFO(2);
//     uint32_t Vout = static_cast<uint32_t>(Amp *127) - 128;
//     int v=adsrGeneral(notes.notes[i].pressedCount);
//     // int v=adsrHorn(notes.notes[i].pressedCount);
//     int volshift=8 - volume+v;
//     if (volshift>=0){

//     Vout = ((Vout+128) >> (volshift)) ;}
//     else {Vout = ((Vout+128) << -(volshift)) ;}

//     return Vout;
// }
u_int32_t calcHornVout(float Amp,int volume, int i){
    // Amp+=generateLFO(2);
    uint32_t Vout = static_cast<uint32_t>(Amp *127) - 128;
    int v=adsrHorn(notes.notes[i].pressedCount);
    // int v=adsrHorn(notes.notes[i].pressedCount);
    int volshift=8 - volume+v;
    if (volshift>=0){

    Vout = ((Vout+128) >> (volshift)) ;}
    else {Vout = ((Vout+128) << -(volshift)) ;}

    return Vout;
}


void presssedTimeCount(){
    for (int i=0;i<96;i++){
        bool isactive=__atomic_load_n(&notes.notes[i].active,__ATOMIC_RELAXED);
        if (isactive){
            notes.notes[i].pressedCount+=1;
        }
        else{
            notes.notes[i].pressedCount=0;
        }
    }
}

// Apply effects (fade, ADSR) to sound output
u_int32_t addEffects(float amp, int volume, int i) {
    int vshift = 0;
    bool fadeon = __atomic_load_n(&settings.fade.on, __ATOMIC_RELAXED);
    bool adsron = __atomic_load_n(&settings.adsr.on, __ATOMIC_RELAXED);
    if (fadeon) {
        vshift = calcFade(notes.notes[i].pressedCount, settings.fade.sustainTime, settings.fade.fadeSpeed);
    } else if (adsron) {
        vshift = adsrGeneral(notes.notes[i].pressedCount);
    }
    return calcVout(amp, volume, vshift);
}
u_int32_t addLFO(float amp,int volume){
    bool lfoon=__atomic_load_n(&settings.lfo.on, __ATOMIC_RELAXED);
    if (lfoon){
        int lfovol=__atomic_load_n(&settings.lfo.reduceLFOVolume, __ATOMIC_RELAXED);
        int lfofreq=__atomic_load_n(&settings.lfo.freq, __ATOMIC_RELAXED);
        amp=generateLFO(lfovol,lfofreq);
        
        u_int32_t Vout=calcVout(amp, volume,0);
        return Vout;
    }
    else{
        return 0;
    }


}
float addLPF(float amp,float *prevamp){
    bool lpon=__atomic_load_n(&settings.lowpass.on, __ATOMIC_RELAXED);
    if (lpon){
        int freq=__atomic_load_n(&settings.lowpass.freq, __ATOMIC_RELAXED);
        amp=lowPassFilter(amp, *prevamp,freq);
        *prevamp=amp;
       
    }
    return amp;

}
