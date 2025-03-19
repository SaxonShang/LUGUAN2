#ifndef READ_INPUTS_H
#define READ_INPUTS_H

#include "pin.h"

// -------------------- Set Active Row for Matrix Scan --------------------
void setRow(uint8_t rowIdx) {
    digitalWrite(REN_PIN, LOW);
    digitalWrite(RA0_PIN, rowIdx & 0x01);
    digitalWrite(RA1_PIN, rowIdx & 0x02);
    digitalWrite(RA2_PIN, rowIdx & 0x04);
    digitalWrite(REN_PIN, HIGH);
}

// -------------------- Read Column States of a Row --------------------
std::bitset<4> readCols(int rowId) {
    setRow(rowId);
    delayMicroseconds(3);
    std::bitset<4> result;
    result[0] = digitalRead(C0_PIN);
    result[1] = digitalRead(C1_PIN);
    result[2] = digitalRead(C2_PIN);
    result[3] = digitalRead(C3_PIN);
    return result;
}

// -------------------- Read Full Input Matrix --------------------
std::bitset<inputSize> readInputs() {
    std::bitset<inputSize> inputs;
    for (int i = 0; i < inputSize / 4; i++) {
        std::bitset<4> cols = readCols(i);
        for (int j = 0; j < 4; j++) {
            inputs[i * 4 + j] = cols[j];
        }
    }
    return inputs;
}

// -------------------- Bit Extraction Utility --------------------
template <size_t F, size_t C>
std::bitset<C> extractBits(const std::bitset<F>& inputBits, int startPos, int length) {
    return std::bitset<C>((inputBits.to_ulong() >> startPos) & ((1 << length) - 1));
}

// -------------------- Process a Single Knob Encoder --------------------
void readOneKnob(knob& knob, std::bitset<2> previousState, std::bitset<2> currentState) {
    if ((previousState == 0b00 && currentState == 0b01) || (previousState == 0b11 && currentState == 0b10)) {
        int newVal = knob.current_knob_value + 1;
        knob.current_knob_value = constrain(newVal, 0, 8);
        knob.lastIncrement = 1;
    }
    else if ((previousState == 0b01 && currentState == 0b00) || (previousState == 0b10 && currentState == 0b11)) {
        int newVal = knob.current_knob_value - 1;
        knob.current_knob_value = constrain(newVal, 0, 8);
        knob.lastIncrement = -1;
    }
    else if (previousState[0] != currentState[0] && previousState[1] != currentState[1]) {
        int newVal = knob.current_knob_value + knob.lastIncrement;
        knob.current_knob_value = constrain(newVal, 0, 8);
    }
    else {
        knob.lastIncrement = 0;
    }
}

// -------------------- Update All Knobs and Click States --------------------
void updateKnob(std::array<knob, 4>& knobValues, std::bitset<8> previous_knobs, std::bitset<8> current_knobs,
                std::bitset<4> previous_clicks, std::bitset<4> current_clicks) {
    for (int i = 0; i < 4; i++) {
        std::bitset<2> prevState = extractBits<8, 2>(previous_knobs, 2 * i, 2);
        std::bitset<2> currState = extractBits<8, 2>(current_knobs, 2 * i, 2);
        readOneKnob(knobValues[3 - i], prevState, currState);
    }

    for (int i = 0; i < 4; i++) {
        if (current_clicks[i] != previous_clicks[i] && current_clicks[i] == 0) {
            knobValues[i].clickState = !knobValues[i].clickState;
        }
    }
}

#endif