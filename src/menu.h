#include "pin_definitions.h"
#include "effect.h"
#ifndef MENU_H
#define MENU_H

std::string convert_bool_2_string(bool value) {
    return value ? "On" : "Off";
}
const int PAGE_HEIGHT = 48;  // **屏幕可显示的最大高度**
const int ITEM_SPACING = 8;  // **每个菜单项的高度**
int menu_offset = 0;         // **当前滚动的偏移量**
int selected_option = 0;     // **当前选中的菜单项**
const int total_menu_items = 7;  // **总菜单项**

// std::string menu_items[total_menu_items] = {
//     "Metronome", "Fade", "LFO", "ADSR",
//     "LPF", "Effect", "Empty"
// };
// **Reverb（混响）**
void reverb_page() {
    sysState.currentMenu = "Reverb";
    u8g2.drawStr(10, 7, "Reverb Effect");
    u8g2.drawStr(30, 7, "State: ");
    u8g2.drawStr(70, 7, convert_bool_2_string(settings.reverb_on).c_str());
    u8g2.drawStr(10, 14, "Strength: ");
    u8g2.drawStr(70, 14, std::to_string(settings.reverb_strength).c_str());
}

// **Distortion（失真）**
void distortion_page() {
    sysState.currentMenu = "Distortion";
    u8g2.drawStr(10, 7, "Distortion Effect");
    u8g2.drawStr(30, 7, "State: ");
    u8g2.drawStr(70, 7, convert_bool_2_string(settings.distortion_on).c_str());
    u8g2.drawStr(10, 14, "Strength: ");
    u8g2.drawStr(70, 14, std::to_string(settings.distortion_strength).c_str());
}

// **Chorus（合唱）**
void chorus_page() {
    sysState.currentMenu = "Chorus";
    u8g2.drawStr(10, 7, "Chorus Effect");
    u8g2.drawStr(30, 7, "State: ");
    u8g2.drawStr(70, 7, convert_bool_2_string(settings.chorus_on).c_str());
    u8g2.drawStr(10, 14, "Strength: ");
    u8g2.drawStr(70, 14, std::to_string(settings.chorus_strength).c_str());
}


// **Effect 主菜单**
void effect_page() {
    static bool prev_knob1_clickState = false;  // 存储上一次的点击状态
    sysState.currentMenu = "Effect";
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_tr);

    // 旋钮1选择效果类型
    settings.effectType += sysState.knobValues[1].lastIncrement;
    settings.effectType = constrain(settings.effectType, 0, 2);

    // 旋钮点击翻转当前效果开关（仅检测上升沿）
    if (sysState.knobValues[1].clickState && !prev_knob1_clickState) {
        switch (settings.effectType) {
            case 0: settings.reverb_on = !settings.reverb_on; break;
            case 1: settings.distortion_on = !settings.distortion_on; break;
            case 2: settings.chorus_on = !settings.chorus_on; break;
        }
    }
    prev_knob1_clickState = sysState.knobValues[1].clickState;  // 更新状态

    // 旋钮2调整效果强度（更灵敏）
    int strength_step = 5;
    switch (settings.effectType) {
        case 0:
            settings.reverb_strength += sysState.knobValues[2].lastIncrement * strength_step;
            settings.reverb_strength = constrain(settings.reverb_strength, 0, 255);
            break;
        case 1:
            settings.distortion_strength += sysState.knobValues[2].lastIncrement * strength_step;
            settings.distortion_strength = constrain(settings.distortion_strength, 0, 255);
            break;
        case 2:
            settings.chorus_strength += sysState.knobValues[2].lastIncrement * strength_step;
            settings.chorus_strength = constrain(settings.chorus_strength, 1, 255);
            break;
    }

// **标题**
    u8g2.setCursor(10, 7);
    u8g2.print("Effect Menu");

    // **显示当前选中的效果**
    u8g2.setCursor(10, 17);
    u8g2.print("> ");
    switch (settings.effectType) {
        case 0: u8g2.print("Reverb"); break;
        case 1: u8g2.print("Distortion"); break;
        case 2: u8g2.print("Chorus"); break;
    }

    // **显示当前效果的开关状态**
    u8g2.setCursor(10, 27);
    u8g2.print("State: ");
    switch (settings.effectType) {
        case 0: u8g2.print(convert_bool_2_string(settings.reverb_on).c_str()); break;
        case 1: u8g2.print(convert_bool_2_string(settings.distortion_on).c_str()); break;
        case 2: u8g2.print(convert_bool_2_string(settings.chorus_on).c_str()); break;
    }

    // **显示当前效果的强度**
    u8g2.setCursor(60, 27);
    u8g2.print("Strength: ");
    switch (settings.effectType) {
        case 0: u8g2.print(std::to_string(settings.reverb_strength).c_str()); break;
        case 1: u8g2.print(std::to_string(settings.distortion_strength).c_str()); break;
        case 2: u8g2.print(std::to_string(settings.chorus_strength).c_str()); break;
    }

    u8g2.sendBuffer();
}


void met_page(Metronome metronome_inside_page){
    sysState.currentMenu = "Met";
    u8g2.drawStr(10, 7, "Metronome");
    u8g2.drawStr(10, 14, "State: ");
    u8g2.drawStr(50, 14, convert_bool_2_string(metronome_inside_page.on).c_str());
    u8g2.drawStr(10, 21, "Speed: ");
    u8g2.drawStr(50, 21, std::to_string(metronome_inside_page.speed).c_str());
}

void fade_page(Fade fade_inside_page) {
    sysState.currentMenu = "Fade";
    u8g2.drawStr(10, 7, "Fade");
    u8g2.drawStr(10, 14, "State: ");
    u8g2.drawStr(50, 14, convert_bool_2_string(fade_inside_page.on).c_str());
    u8g2.drawStr(10, 21, "sustainTime: ");
    u8g2.drawStr(70, 21, std::to_string(fade_inside_page.sustainTime).c_str());
    u8g2.drawStr(10, 28, "fadeSpeed: ");
    u8g2.drawStr(60, 28, std::to_string(fade_inside_page.fadeSpeed).c_str());

}

void lfo_page(LFO lfo_inside_page) {
    sysState.currentMenu = "LFO";
    u8g2.drawStr(10, 7, "Low Fre Oscillator");
    u8g2.drawStr(10, 14, "State: ");
    u8g2.drawStr(50, 14, convert_bool_2_string(lfo_inside_page.on).c_str());
    u8g2.drawStr(10, 21, "freq:");
    u8g2.drawStr(70, 21, std::to_string(lfo_inside_page.freq).c_str());
    u8g2.drawStr(10, 28, "reduceLFOVolume:");
    u8g2.drawStr(90, 28, std::to_string(lfo_inside_page.reduceLFOVolume).c_str());

}

void adsr_page(ADSR adsr_inside_page) {
    sysState.currentMenu = "ADSR";
    u8g2.drawStr(10, 7, "ADSR Envelope Gen");
    u8g2.drawStr(10, 14, "on? attack dec sustain");
    u8g2.drawStr(10, 21, convert_bool_2_string(adsr_inside_page.on).c_str());
    u8g2.drawStr(40, 21, std::to_string(adsr_inside_page.attack).c_str());
    u8g2.drawStr(70, 21, std::to_string(adsr_inside_page.decay).c_str());
    u8g2.drawStr(100, 21, std::to_string(adsr_inside_page.sustain).c_str());
}

void lpf_page(Lowpass lowpass_inside_page) {
    sysState.currentMenu = "LPF";
    u8g2.drawStr(10, 7, "Low Pass Filter");
    u8g2.drawStr(10, 14, "State: ");
    u8g2.drawStr(50, 14, convert_bool_2_string(lowpass_inside_page.on).c_str());
    u8g2.drawStr(10, 21, "Fre: ");
    u8g2.drawStr(50, 21, std::to_string(lowpass_inside_page.freq).c_str());
}

void empty_page() {
    u8g2.drawStr(10, 10, "Empty");
}


void menu(int option, setting setting_inside_menu) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_tr);
    // **计算起始Y轴位置**
    int start_y = 10 - menu_offset;

    // **循环显示菜单选项**
    // for (int i = 0; i < total_menu_items; i++) {
    //     int y_position = start_y + (i * ITEM_SPACING);

    //     // **如果菜单项在可见区域内，才绘制**
    //     if (y_position >= 10 && y_position < PAGE_HEIGHT + 10) {
    //         u8g2.setCursor(10, y_position);
    //         if (i == selected_option) {
    //             u8g2.print("> ");  // **高亮选项**
    //         } else {
    //             u8g2.print("  ");
    //         }
    //         // u8g2.print(menu_items[i].c_str());
    //     }
    // }

    // u8g2.sendBuffer();


    switch(option) {
        case 0: met_page(setting_inside_menu.metronome); break;
        case 1: fade_page(setting_inside_menu.fade); break;
        case 2: lfo_page(setting_inside_menu.lfo); break;
        case 3: adsr_page(setting_inside_menu.adsr); break;
        case 4: lpf_page(setting_inside_menu.lowpass); break;
        case 5: effect_page(); break;  // **Effect 菜单**
        case 6: empty_page(); break;
        default: empty_page(); break;
    }
}
// void menu(int selected_option, setting setting_inside_menu) {
//     u8g2.clearBuffer();
//     u8g2.setFont(u8g2_font_5x8_tr);

//     // **计算起始Y轴位置**
//     int start_y = 10 - menu_offset;

//     // **循环显示菜单选项**
//     for (int i = 0; i < total_menu_items; i++) {
//         int y_position = start_y + (i * ITEM_SPACING);

//         // **如果菜单项在可见区域内，才绘制**
//         if (y_position >= 10 && y_position < PAGE_HEIGHT + 10) {
//             u8g2.setCursor(10, y_position);
//             if (i == selected_option) {
//                 u8g2.print("> ");  // **高亮选项**
//             } else {
//                 u8g2.print("  ");
//             }
//             u8g2.print(menu_items[i].c_str());
//         }
//     }

//     u8g2.sendBuffer();

//     // **点击时跳转**
//     if (sysState.knobValues[1].clickState) {
//         switch (selected_option) {
//             case 0: met_page(setting_inside_menu.metronome); break;
//             case 1: fade_page(setting_inside_menu.fade); break;
//             case 2: lfo_page(setting_inside_menu.lfo); break;
//             case 3: adsr_page(setting_inside_menu.adsr); break;
//             case 4: lpf_page(setting_inside_menu.lowpass); break;
//             case 5: effect_page(); break;  // **进入 Effect 菜单**
//             case 6: empty_page(); break;
//             default: empty_page(); break;
//         }
//     }
// }

void update_menu_settings(std::string option, int& currentTune){
    if (option == "Main"){
        settings.volume += sysState.knobValues[3].lastIncrement;
        settings.tune += sysState.knobValues[2].lastIncrement;
        settings.waveIndex += sysState.knobValues[1].lastIncrement;
        settings.volume  = constrain(settings.volume, 0, 8);
        settings.tune = constrain(settings.tune, 0, 8);
        settings.waveIndex = constrain(settings.waveIndex, 0, 8);
        currentTune = settings.tune;
    }

  // **Effect 菜单**
    else if (option == "Effect") {
        settings.effectType += sysState.knobValues[1].lastIncrement;
        settings.effectType = constrain(settings.effectType, 0, 2); // 0: Reverb, 1: Distortion, 2: Chorus
    }

    // **Effect 详细设置**
    else if (option == "Reverb") {
        settings.reverb_on = sysState.knobValues[1].clickState;  // 旋钮 1 控制 On/Off
        settings.reverb_strength += sysState.knobValues[2].lastIncrement;  // 旋钮 2 控制强度
        settings.reverb_strength = constrain(settings.reverb_strength, 0, 255);
    }

    else if (option == "Distortion") {
        settings.distortion_on = sysState.knobValues[1].clickState;  // 旋钮 1 控制 On/Off
        settings.distortion_strength += sysState.knobValues[2].lastIncrement;
        settings.distortion_strength = constrain(settings.distortion_strength, 0, 255);
    }

    else if (option == "Chorus") {
        settings.chorus_on = sysState.knobValues[1].clickState;  // 旋钮 1 控制 On/Off
        settings.chorus_strength += sysState.knobValues[2].lastIncrement;
        settings.chorus_strength = constrain(settings.chorus_strength, 1, 255);
    }

    else if (option == "Met"){
      settings.metronome.on = sysState.knobValues[1].clickState;
      settings.metronome.speed += sysState.knobValues[2].lastIncrement;
      settings.metronome.speed = constrain(settings.metronome.speed, 1, 8);
    }

    else if (option == "Fade"){
      settings.fade.on = sysState.knobValues[1].clickState;
      settings.fade.fadeSpeed += sysState.knobValues[3].lastIncrement;
      settings.fade.sustainTime += sysState.knobValues[2].lastIncrement;
      settings.fade.fadeSpeed = constrain(settings.fade.fadeSpeed, 1, 10);
      settings.fade.sustainTime = constrain(settings.fade.sustainTime, 1, 30);
    }

    else if (option == "LFO"){
      settings.lfo.on = sysState.knobValues[1].clickState;
      settings.lfo.freq += sysState.knobValues[2].lastIncrement;
      settings.lfo.reduceLFOVolume += sysState.knobValues[3].lastIncrement;
      settings.lfo.freq = constrain(settings.lfo.freq, 1, 30);
      settings.lfo.reduceLFOVolume = constrain(settings.lfo.reduceLFOVolume, 1, 8);
    }

    else if (option == "ADSR"){
      settings.adsr.on = sysState.knobValues[1].clickState;
      settings.adsr.attack += sysState.knobValues[1].lastIncrement;
      settings.adsr.decay += sysState.knobValues[2].lastIncrement;
      settings.adsr.sustain += sysState.knobValues[3].lastIncrement;
      settings.adsr.attack = constrain(settings.adsr.attack, 0, 50);
      settings.adsr.decay = constrain(settings.adsr.decay, 0, 50);
      settings.adsr.sustain = constrain(settings.adsr.sustain, 0, 50);
    }

    else if (option == "LPF"){
      settings.lowpass.on = sysState.knobValues[1].clickState;
      settings.lowpass.freq += 100*sysState.knobValues[2].lastIncrement;
      settings.lowpass.freq = constrain(settings.lowpass.freq, 500,2000 );
    }
}
#endif