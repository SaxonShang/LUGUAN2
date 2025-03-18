#ifndef MENU_H
#define MENU_H

#include "pin.h"
#include "effect.h"

// ---------- Menu Constants ----------
const int PAGE_HEIGHT = 48;
const int ITEM_SPACING = 8;
int menu_offset = 0;
int selected_option = 0;
const int total_menu_items = 7;

std::string convertBoolToStr(bool val) {
    return val ? "On" : "Off";
}

// ---------- Page Render Functions ----------
void emptyPage() {
    u8g2.drawStr(10, 14, "Empty Page");
}

void metPage(Metronome m) {
    sysState.currentMenu = "Met";
    u8g2.drawStr(10, 7, "Metronome");
    u8g2.drawStr(10, 14, "State: ");
    u8g2.drawStr(50, 14, convertBoolToStr(m.on).c_str());
    u8g2.drawStr(10, 21, "Speed: ");
    u8g2.drawStr(50, 21, std::to_string(m.speed).c_str());
}

void fadePage(Fade f) {
    sysState.currentMenu = "Fade";
    u8g2.drawStr(10, 7, "Fade");
    u8g2.drawStr(10, 14, "State: ");
    u8g2.drawStr(50, 14, convertBoolToStr(f.on).c_str());
    u8g2.drawStr(10, 21, "Sustain: ");
    u8g2.drawStr(70, 21, std::to_string(f.sustainTime).c_str());
    u8g2.drawStr(10, 28, "FadeSpd: ");
    u8g2.drawStr(70, 28, std::to_string(f.fadeSpeed).c_str());
}

void lfoPage(LFO l) {
    sysState.currentMenu = "LFO";
    u8g2.drawStr(10, 7, "LFO Settings");
    u8g2.drawStr(10, 14, "State: ");
    u8g2.drawStr(50, 14, convertBoolToStr(l.on).c_str());
    u8g2.drawStr(10, 21, "Freq: ");
    u8g2.drawStr(50, 21, std::to_string(l.freq).c_str());
    u8g2.drawStr(10, 28, "ReduceVol: ");
    u8g2.drawStr(80, 28, std::to_string(l.reduceLFOVolume).c_str());
}

void adsrPage(ADSR a) {
    sysState.currentMenu = "ADSR";
    u8g2.drawStr(10, 7, "ADSR Envelope");
    u8g2.drawStr(10, 14, "On");
    u8g2.drawStr(40, 14, "A");
    u8g2.drawStr(70, 14, "D");
    u8g2.drawStr(100, 14, "S");


    u8g2.setCursor(10, 21); u8g2.print(convertBoolToStr(a.on).c_str());
    u8g2.setCursor(40, 21); u8g2.print(std::to_string(a.attack).c_str());
    u8g2.setCursor(70, 21); u8g2.print(std::to_string(a.decay).c_str());
    u8g2.setCursor(100, 21); u8g2.print(std::to_string(a.sustain).c_str());
}

void lpfPage(Lowpass lpf) {
    sysState.currentMenu = "LPF";
    u8g2.drawStr(10, 7, "LowPass Filter");
    u8g2.drawStr(10, 14, "State: ");
    u8g2.drawStr(50, 14, convertBoolToStr(lpf.on).c_str());
    u8g2.drawStr(10, 21, "Freq: ");
    u8g2.drawStr(50, 21, std::to_string(lpf.freq).c_str());
}

void reverbPage() {
    sysState.currentMenu = "Reverb";
    u8g2.drawStr(10, 7, "Reverb Effect");
    u8g2.drawStr(30, 7, "State: ");
    u8g2.drawStr(70, 7, convertBoolToStr(settings.reverb_on).c_str());
    u8g2.drawStr(10, 14, "Strength: ");
    u8g2.drawStr(70, 14, std::to_string(settings.reverb_strength).c_str());
}

void distortionPage() {
    sysState.currentMenu = "Distortion";
    u8g2.drawStr(10, 7, "Distortion Effect");
    u8g2.drawStr(30, 7, "State: ");
    u8g2.drawStr(70, 7, convertBoolToStr(settings.distortion_on).c_str());
    u8g2.drawStr(10, 14, "Strength: ");
    u8g2.drawStr(70, 14, std::to_string(settings.distortion_strength).c_str());
}

void chorusPage() {
    sysState.currentMenu = "Chorus";
    u8g2.drawStr(10, 7, "Chorus Effect");
    u8g2.drawStr(30, 7, "State: ");
    u8g2.drawStr(70, 7, convertBoolToStr(settings.chorus_on).c_str());
    u8g2.drawStr(10, 14, "Strength: ");
    u8g2.drawStr(70, 14, std::to_string(settings.chorus_strength).c_str());
}

void effectPage() {
    static bool prevClick = false;
    sysState.currentMenu = "Effect";
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_tr);

    settings.effectType += sysState.knobValues[1].lastIncrement;
    settings.effectType = constrain(settings.effectType, 0, 2);

    if (sysState.knobValues[1].clickState && !prevClick) {
        if (settings.effectType == 0) settings.reverb_on = !settings.reverb_on;
        else if (settings.effectType == 1) settings.distortion_on = !settings.distortion_on;
        else if (settings.effectType == 2) settings.chorus_on = !settings.chorus_on;
    }
    prevClick = sysState.knobValues[1].clickState;

    int step = 5;
    if (settings.effectType == 0) {
        settings.reverb_strength += sysState.knobValues[2].lastIncrement * step;
        settings.reverb_strength = constrain(settings.reverb_strength, 0, 255);
    } else if (settings.effectType == 1) {
        settings.distortion_strength += sysState.knobValues[2].lastIncrement * step;
        settings.distortion_strength = constrain(settings.distortion_strength, 0, 255);
    } else if (settings.effectType == 2) {
        settings.chorus_strength += sysState.knobValues[2].lastIncrement * step;
        settings.chorus_strength = constrain(settings.chorus_strength, 1, 255);
    }

    u8g2.setCursor(10, 7);  u8g2.print("Effect Menu");
    u8g2.setCursor(10, 17); u8g2.print("> ");
    if (settings.effectType == 0) u8g2.print("Reverb");
    else if (settings.effectType == 1) u8g2.print("Distortion");
    else u8g2.print("Chorus");

    u8g2.setCursor(10, 27); u8g2.print("State: ");
    if (settings.effectType == 0) u8g2.print(convertBoolToStr(settings.reverb_on).c_str());
    else if (settings.effectType == 1) u8g2.print(convertBoolToStr(settings.distortion_on).c_str());
    else u8g2.print(convertBoolToStr(settings.chorus_on).c_str());

    u8g2.setCursor(60, 27); u8g2.print("Strength: ");
    if (settings.effectType == 0) u8g2.print(std::to_string(settings.reverb_strength).c_str());
    else if (settings.effectType == 1) u8g2.print(std::to_string(settings.distortion_strength).c_str());
    else u8g2.print(std::to_string(settings.chorus_strength).c_str());

    u8g2.sendBuffer();
}


// ---------- Main Menu Rendering Dispatcher ----------
void menu(int option, setting s) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_tr);
    switch(option) {
        case 0: metPage(s.metronome); break;
        case 1: fadePage(s.fade); break;
        case 2: lfoPage(s.lfo); break;
        case 3: adsrPage(s.adsr); break;
        case 4: lpfPage(s.lowpass); break;
        case 5: effectPage(); break;
        case 6: emptyPage(); break;
        default: emptyPage(); break;
    }
}

// ---------- Menu Setting Update Dispatcher ----------
void update_menu_settings(std::string option, int& currentTune) {
    if (option == "Main") {
        settings.volume += sysState.knobValues[3].lastIncrement;
        settings.tune += sysState.knobValues[2].lastIncrement;
        settings.waveIndex += sysState.knobValues[1].lastIncrement;
        settings.volume = constrain(settings.volume, 0, 8);
        settings.tune = constrain(settings.tune, 0, 8);
        settings.waveIndex = constrain(settings.waveIndex, 0, 8);
        currentTune = settings.tune;
    } else if (option == "Effect") {
        settings.effectType += sysState.knobValues[1].lastIncrement;
        settings.effectType = constrain(settings.effectType, 0, 2);
    } else if (option == "Reverb") {
        settings.reverb_on = sysState.knobValues[1].clickState;
        settings.reverb_strength += sysState.knobValues[2].lastIncrement;
        settings.reverb_strength = constrain(settings.reverb_strength, 0, 255);
    } else if (option == "Distortion") {
        settings.distortion_on = sysState.knobValues[1].clickState;
        settings.distortion_strength += sysState.knobValues[2].lastIncrement;
        settings.distortion_strength = constrain(settings.distortion_strength, 0, 255);
    } else if (option == "Chorus") {
        settings.chorus_on = sysState.knobValues[1].clickState;
        settings.chorus_strength += sysState.knobValues[2].lastIncrement;
        settings.chorus_strength = constrain(settings.chorus_strength, 1, 255);
    } else if (option == "Met") {
        settings.metronome.on = sysState.knobValues[1].clickState;
        settings.metronome.speed += sysState.knobValues[2].lastIncrement;
        settings.metronome.speed = constrain(settings.metronome.speed, 1, 8);
    } else if (option == "Fade") {
        settings.fade.on = sysState.knobValues[1].clickState;
        settings.fade.sustainTime += sysState.knobValues[2].lastIncrement;
        settings.fade.fadeSpeed += sysState.knobValues[3].lastIncrement;
        settings.fade.sustainTime = constrain(settings.fade.sustainTime, 1, 30);
        settings.fade.fadeSpeed = constrain(settings.fade.fadeSpeed, 1, 10);
    } else if (option == "LFO") {
        settings.lfo.on = sysState.knobValues[1].clickState;
        settings.lfo.freq += sysState.knobValues[2].lastIncrement;
        settings.lfo.reduceLFOVolume += sysState.knobValues[3].lastIncrement;
        settings.lfo.freq = constrain(settings.lfo.freq, 1, 30);
        settings.lfo.reduceLFOVolume = constrain(settings.lfo.reduceLFOVolume, 1, 8);
    } else if (option == "ADSR") {
        settings.adsr.on = sysState.knobValues[1].clickState;
        settings.adsr.attack += sysState.knobValues[1].lastIncrement;
        settings.adsr.decay += sysState.knobValues[2].lastIncrement;
        settings.adsr.sustain += sysState.knobValues[3].lastIncrement;
        settings.adsr.attack = constrain(settings.adsr.attack, 0, 50);
        settings.adsr.decay = constrain(settings.adsr.decay, 0, 50);
        settings.adsr.sustain = constrain(settings.adsr.sustain, 0, 50);
    } else if (option == "LPF") {
        settings.lowpass.on = sysState.knobValues[1].clickState;
        settings.lowpass.freq += sysState.knobValues[2].lastIncrement * 100;
        settings.lowpass.freq = constrain(settings.lowpass.freq, 500, 2000);
    }
}

#endif
