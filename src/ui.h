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
    u8g2.drawStr(30, 14, "State: ");
    u8g2.drawStr(70, 14, convertBoolToStr(settings.reverb_on).c_str());
    u8g2.drawStr(10, 21, "Strength: ");
    u8g2.drawStr(70, 21, std::to_string(settings.reverb_strength).c_str());
}

void distortionPage() {
    sysState.currentMenu = "Distortion";
    u8g2.drawStr(10, 7, "Distortion Effect");
    u8g2.drawStr(30, 14, "State: ");
    u8g2.drawStr(70, 14, convertBoolToStr(settings.distortion_on).c_str());
    u8g2.drawStr(10, 21, "Strength: ");
    u8g2.drawStr(70, 21, std::to_string(settings.distortion_strength).c_str());
}

void chorusPage() {
    sysState.currentMenu = "Chorus";
    u8g2.drawStr(10, 7, "Chorus Effect");
    u8g2.drawStr(30, 14, "State: ");
    u8g2.drawStr(70, 14, convertBoolToStr(settings.chorus_on).c_str());
    u8g2.drawStr(10, 21, "Strength: ");
    u8g2.drawStr(70, 21, std::to_string(settings.chorus_strength).c_str());
}

// ---------- Main Menu Rendering Dispatcher ----------
void menu(int option, setting s) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_tr);
    switch(option) {
        case 0: metPage(s.metronome); break;
        case 1: adsrPage(s.adsr); break;
        case 2: lpfPage(s.lowpass); break;
        case 3: distortionPage(); break;
        case 4: chorusPage(); break;
        case 5: reverbPage(); break;
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
        settings.reverb_strength = constrain(settings.reverb_strength, 0, 10);
    } else if (option == "Distortion") {
        settings.distortion_on = sysState.knobValues[1].clickState;
        settings.distortion_strength += sysState.knobValues[2].lastIncrement;
        settings.distortion_strength = constrain(settings.distortion_strength, 0, 10);
    } else if (option == "Chorus") {
        settings.chorus_on = sysState.knobValues[1].clickState;
        settings.chorus_strength += sysState.knobValues[2].lastIncrement;
        settings.chorus_strength = constrain(settings.chorus_strength, 0, 10);
    } else if (option == "Met") {
        settings.metronome.on = sysState.knobValues[1].clickState;
        settings.metronome.speed += sysState.knobValues[2].lastIncrement;
        settings.metronome.speed = constrain(settings.metronome.speed, 1, 8);
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
