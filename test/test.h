// -------------------- Include Headers --------------------
#include <STM32FreeRTOS.h>
#include <ES_CAN.h>
#include <cmath>

#include "input_read.h"
#include "pin.h"
#include "waves.h"
#include "ui.h"
#include "test.h"
#include "effect.h"

// -------------------- Module: Sample Buffer Writer --------------------
// Write output sample into active sample buffer
void writeToSampleBuffer(uint32_t Vout, uint32_t writeCtr) {
    if (writeBuffer1) {
        sampleBuffer1[writeCtr] = Vout;
    } else {
        sampleBuffer0[writeCtr] = Vout;
    }
}

void send_handshake_signal(int stateW, int stateE) {
    setRow(5);
    delayMicroseconds(3);
    digitalWrite(OUT_PIN, stateW);
    delayMicroseconds(3);
    setRow(6);
    delayMicroseconds(3);
    digitalWrite(OUT_PIN, stateE);
    delayMicroseconds(3);
}

// -------------------- Module: Audio Sample Interrupt --------------------
void sampleISR() {
    static uint32_t readCtr = 0;
    static uint32_t metronomeCounter = 0;

    int metronomeSpeed = __atomic_load_n(&settings.metronome.speed, __ATOMIC_RELAXED);
    bool metronomeOn = __atomic_load_n(&settings.metronome.on, __ATOMIC_RELAXED);
    int posId = __atomic_load_n(&sysState.posId, __ATOMIC_RELAXED);

    if (posId == 0) {
        if (readCtr == SAMPLE_BUFFER_SIZE / 2) {
            readCtr = 0;
            writeBuffer1 = !writeBuffer1;
            presssedTimeCount();
            xSemaphoreGiveFromISR(sampleBufferSemaphore, NULL);
        }

        if (metronomeSpeed != 8 && metronomeCounter >= metronomeTime[7 - metronomeSpeed] && metronomeOn) {
            analogWrite(OUTR_PIN, 255);
            metronomeCounter = 0;
        } else {
            analogWrite(OUTR_PIN, writeBuffer1 ? sampleBuffer0[readCtr++] : sampleBuffer1[readCtr++]);
            metronomeCounter++;
        }
    } else {
        if (readCtr == SAMPLE_BUFFER_SIZE / 2) {
            readCtr = 0;
            xSemaphoreGiveFromISR(sampleBufferSemaphore, NULL);
        } else {
            readCtr++;
        }
    }
}

// -------------------- CAN Interrupt Handlers --------------------
void CAN_RX_ISR(void) {
    uint8_t RX_Message_ISR[8];
    CAN_RX(ID, RX_Message_ISR);
    xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
}

void CAN_TX_ISR(void) {
    xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}

// -------------------- Auto-Detect Board Position --------------------
int auto_detect_init() {
    uint8_t detext_RX_Message[8] = {0};   // Message received via CAN
    uint8_t detext_TX_Message[8] = {0};   // Message to send via CAN
    uint32_t detect_CAN_ID = 0x123;       // Common CAN ID for detection

    std::bitset<28> inputs;
    std::bitset<1> WestDetect;
    std::bitset<1> EastDetect;

    // Display detection screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(20, 20);
    u8g2.print("Auto Detecting...");
    u8g2.sendBuffer();

    // Initial handshake pulse
    for (int i = 0; i < 100; i++) {
        send_handshake_signal(1, 1);
        delay(30);
    }

    // Read handshake detection pins
    inputs = readInputs();
    WestDetect = extractBits<28, 1>(inputs, 23, 1);
    EastDetect = extractBits<28, 1>(inputs, 27, 1);
    delay(2000);  // Allow time for signal propagation

    // If no board on the west → not the main board
    if (!WestDetect[0]) {
        Serial.println("West board detected (not main)");

        // Wait for confirmation message from main board
        do {
            Serial.println("Awaiting main board confirmation...");
            delay(100);
            while (CAN_CheckRXLevel()) {
                CAN_RX(detect_CAN_ID, detext_RX_Message);
            }
        } while (detext_RX_Message[0] != 'C' || detext_RX_Message[1] != 0);

        Serial.println("Main board confirmed.");

        // Reset handshake lines to 0
        for (int i = 0; i < 10; i++) {
            send_handshake_signal(0, 0);
            delay(30);
        }

        // Wait for handshake signal to be updated again
        do {
            inputs = readInputs();
            WestDetect = extractBits<28, 1>(inputs, 23, 1);
            delay(10);
        } while (WestDetect[0]);

        Serial.println("Updated West Detect confirmed.");
        delay(200);

        // Wait for the message from west board indicating its position
        do {
            Serial.println("Waiting for board position info from west...");
            while (CAN_CheckRXLevel()) {
                CAN_RX(detect_CAN_ID, detext_RX_Message);
            }
        } while (detext_RX_Message[0] != 'M');

        // Send final handshake again
        for (int i = 0; i < 10; i++) {
            send_handshake_signal(1, 1);
            delay(10);
        }

        Serial.println("East handshake updated.");

        // Send own position (received_position + 1)
        detext_TX_Message[0] = 'M';
        detext_TX_Message[1] = detext_RX_Message[1] + 1;

        if (detext_RX_Message[1] != 2) {
            CAN_TX(detect_CAN_ID, detext_TX_Message);
            Serial.println("Sent board pos to east.");
        } else {
            Serial.println("This is the east-most board.");
        }

        return detext_TX_Message[1];
    }
    // If west is detected → this is the main board
    else {
        delay(500);
        Serial.println("Main board detected (no west neighbor)");

        // Notify others that main board is determined
        detext_TX_Message[0] = 'C';
        detext_TX_Message[1] = 0;
        CAN_TX(detect_CAN_ID, detext_TX_Message);
        Serial.println("Sent main board confirmation");

        delay(100);
        send_handshake_signal(1, 1);
        delay(200);

        // Send position (M0)
        detext_TX_Message[0] = 'M';
        CAN_TX(detect_CAN_ID, detext_TX_Message);
        Serial.println("Broadcasted main board position");

        return 0;
    }
}

// -------------------- Task: Scan Keys --------------------
void scanKeysTask(void *pvParameters) {
    Serial.println("scanKeysTask started!");

    const TickType_t scanInterval = 20 / portTICK_PERIOD_MS;
    TickType_t lastWakeTime = xTaskGetTickCount();

    std::bitset<12> currentKeys;
    std::bitset<12> previousKeys("111111111111");

    std::bitset<8> knobStatesCurrent, knobStatesPrevious("00000000");
    std::bitset<4> knobClicksCurrent, knobClicksPrevious("0000");

    std::bitset<1> joystickStateCurrent("0"), joystickStatePrevious("0");
    std::bitset<1> westDetectPrevious, eastDetectPrevious;

    int tunePrevious = 0;
    int tuneCurrent = 0;

    while (1) {
        uint32_t startTime = micros();
        vTaskDelayUntil(&lastWakeTime, scanInterval);

        // Lock shared state
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        xSemaphoreTake(notes.mutex, portMAX_DELAY);
        xSemaphoreTake(settings.mutex, portMAX_DELAY);

        // Read all hardware inputs
        sysState.inputs = readInputs();

        currentKeys = extractBits<inputSize, 12>(sysState.inputs, 0, 12);
        sysState.WestDetect = extractBits<28, 1>(sysState.inputs, 23, 1);
        sysState.EastDetect = extractBits<28, 1>(sysState.inputs, 27, 1);

        knobStatesCurrent = extractBits<inputSize, 8>(sysState.inputs, 12, 8);
        knobClicksCurrent = extractBits<inputSize, 4>(sysState.inputs, 20, 2).to_ulong() << 2 |
                             extractBits<inputSize, 4>(sysState.inputs, 24, 2).to_ulong();

        joystickStateCurrent = extractBits<inputSize, 1>(sysState.inputs, 22, 1).to_ulong();

        // Toggle joystick flag if state changed and pressed
        if (joystickStateCurrent != joystickStatePrevious && joystickStateCurrent == 0) {
            sysState.joystickState = !sysState.joystickState;
        }

        // Update knob values and click states
        updateKnob(sysState.knobValues, knobStatesPrevious, knobStatesCurrent,
                   knobClicksPrevious, knobClicksCurrent);

        // Update menu configuration
        update_menu_settings(sysState.currentMenu, tuneCurrent);

        // Sync tune if changed and in main board
        if (tuneCurrent != tunePrevious && sysState.posId == 0 && !sysState.EastDetect[0]) {
            Serial.println("Tune updated by main board.");
            __atomic_store_n(&settings.tune, tuneCurrent, __ATOMIC_RELAXED);
            TX_Message[0] = 'T';
            TX_Message[1] = tuneCurrent;
            xQueueSend(msgOutQ, const_cast<uint8_t *>(TX_Message), portMAX_DELAY);
        }

        // Trigger position update if WestDetect changes
        if (eastDetectPrevious[0] && westDetectPrevious[0] && (!sysState.WestDetect[0])) {
            Serial.println("Triggering position update request...");
            delay(100);
            TX_Message[0] = 'N';
            TX_Message[1] = sysState.local_boardId;
            xQueueSend(msgOutQ, const_cast<uint8_t *>(TX_Message), portMAX_DELAY);
        }

        // Main board operation: local sound play
        if (sysState.WestDetect[0] && sysState.EastDetect[0]) {
            sysState.posId = 0;
            int tune = __atomic_load_n(&settings.tune, __ATOMIC_RELAXED);
            for (int i = 0; i < 12; ++i) {
                if (currentKeys.to_ulong() != 0xFFF) {
                    notes.notes[(tune - 1) * 12 + i].active = !currentKeys[i];
                } else {
                    notes.notes[(tune - 1) * 12 + i].active = false;
                }
            }
        }
        // Relay mode: forward note state changes via CAN
        else {
            for (int i = 0; i < 12; ++i) {
                if (currentKeys[i] != previousKeys[i]) {
                    TX_Message[0] = currentKeys[i] ? 'R' : 'P';
                    TX_Message[1] = i;
                    TX_Message[2] = settings.tune;
                    TX_Message[3] = sysState.posId;
                    xQueueSend(msgOutQ, const_cast<uint8_t *>(TX_Message), portMAX_DELAY);
                }
            }
        }

        // Store previous values for next loop
        tunePrevious = tuneCurrent;
        previousKeys = currentKeys;
        knobStatesPrevious = knobStatesCurrent;
        knobClicksPrevious = knobClicksCurrent;
        joystickStatePrevious = joystickStateCurrent;
        westDetectPrevious = sysState.WestDetect;
        eastDetectPrevious = sysState.EastDetect;

        // Release all mutexes
        xSemaphoreGive(sysState.mutex);
        xSemaphoreGive(notes.mutex);
        xSemaphoreGive(settings.mutex);
        Serial.println(micros() - startTime);
    }
}

// -------------------- Task: Joystick Scanning --------------------
void scanJoystickTask(void *pvParameters) {
    const TickType_t xFrequency = 100 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    const int origin = 490;    // Joystick center position
    const int deadZone = 150;  // Threshold to ignore small fluctuations
    int joystickX = 0, joystickY = 0;

    std::string previousMovement = "origin";
    std::string currentMovement;

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        joystickX = analogRead(JOYX_PIN);
        joystickY = analogRead(JOYY_PIN);

        // Determine joystick movement direction
        if (abs(joystickX - origin) <= deadZone && abs(joystickY - origin) <= deadZone) {
            currentMovement = "origin";
        } else if (joystickX - origin > deadZone) {
            currentMovement = "left";
        } else if (joystickX - origin < -deadZone) {
            currentMovement = "right";
        } else if (joystickY - origin > deadZone) {
            currentMovement = "down";
            // Scroll menu down
            if (selected_option < total_menu_items - 1) {
                selected_option++;
                if ((selected_option * ITEM_SPACING) >= (menu_offset + PAGE_HEIGHT)) {
                    menu_offset += ITEM_SPACING;
                }
            }
        } else if (joystickY - origin < -deadZone) {
            currentMovement = "up";
            // Scroll menu up
            if (selected_option > 0) {
                selected_option--;
                if ((selected_option * ITEM_SPACING) < menu_offset) {
                    menu_offset = std::max(0, menu_offset - ITEM_SPACING);
                }
            }
        }

        // Confirm movement only when joystick returns to center
        if (currentMovement != previousMovement && currentMovement == "origin") {
            movement = previousMovement;
        } else {
            movement = "origin";
        }
        previousMovement = currentMovement;
    }
}

// -------------------- Task: Display Update --------------------
void displayUpdateTask(void * pvParameters) {
    const TickType_t xFrequency2 = 100 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime2 = xTaskGetTickCount();
    
    static uint32_t count = 0;
    int posX = 0;
    int posY = 0;
    int index = 0;
    int stay_time = 2000;
    int counter = 0;
    int pressedKeyH = 20;
    std::bitset<2> previous_knob1("00");
    std::bitset<2> previous_knob2("00");
    std::bitset<2> previous_knob3("00");
    
    while (1) {
        vTaskDelayUntil(&xLastWakeTime2, xFrequency2);
        u8g2.clearBuffer();

        if (sysState.posId != 0) {
            u8g2.setCursor(30, 15);
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.print("Slave board");
            pressedKeyH = 25;
        } else {
            pressedKeyH = 20;
            if (counter != 0) {
                counter--;
            }
            xSemaphoreTake(sysState.mutex, portMAX_DELAY);
            xSemaphoreTake(settings.mutex, portMAX_DELAY);

            if (sysState.knobValues[0].clickState) {
                u8g2.setFont(u8g2_font_6x10_tr);
                if (movement == "down") {
                    index += 3;
                } else if (movement == "up") {
                    index -= 3;
                } else if (movement == "right") {
                    index += 1;
                } else if (movement == "left") {
                    index -= 1;
                }
                index = constrain(index, 0, 6);

                if ((index * ITEM_SPACING) >= (menu_offset + PAGE_HEIGHT)) {
                    menu_offset += ITEM_SPACING;
                } else if ((index * ITEM_SPACING) < menu_offset) {
                    menu_offset -= ITEM_SPACING;
                }

                // Draw menu items in rows
                for (int i = 0; i < 7; i++) {
                    int x_pos, y_pos;
                    if (i < 3) {            // First row
                        x_pos = 10 + 35 * i;
                        y_pos = 14;
                    } else if (i < 6) {     // Second row
                        x_pos = 10 + 35 * (i - 3);
                        y_pos = 25;
                    } else {                // Third row (single "Effect" option)
                        x_pos = 10 + 35;  // Center the "Effect" option
                        y_pos = 36;
                    }

                    // Slow slide effect for last item when selected
                    if (index == 6) {
                        if (menu_offset < ITEM_SPACING * 2) {
                            menu_offset += 2;
                        }
                    } else if (index != 6) {
                        menu_offset = 0;
                    }

                    int final_y_pos = y_pos - menu_offset;
                    // Draw only if within visible area
                    if (final_y_pos >= 10 && final_y_pos < PAGE_HEIGHT + 10) {
                        u8g2.setCursor(x_pos, final_y_pos);
                        u8g2.print(menu_first_level[i].c_str());
                        // Highlight selected menu item
                        if (index == i) {
                            u8g2.drawFrame(x_pos - 3, final_y_pos - 9, 32, 12);
                        }
                    }
                }
                if (sysState.joystickState) {
                    menu(index, settings);
                }
            } else {
                sysState.currentMenu = "Main";
                u8g2.setCursor(15, 10);
                u8g2.setFont(u8g2_font_ncenB08_tr);
                u8g2.print("LUGUAN Keyboard");
                u8g2.setFont(u8g2_font_5x8_tr);
                for (int i = 0; i < 4; i++) {
                    u8g2.drawFrame(8 + 30 * i, 20, 25, 20);
                    if (i != 0 && 
                        (extractBits<inputSize, 2>(sysState.inputs, 12, 2) != previous_knob3 ||
                         extractBits<inputSize, 2>(sysState.inputs, 14, 2) != previous_knob2 ||
                         extractBits<inputSize, 2>(sysState.inputs, 16, 2) != previous_knob1)) {
                        counter = stay_time / 100;
                    }
                    if (counter != 0 && i == 3) {
                        u8g2.drawStr(10 + 30 * i, 29, std::to_string(settings.volume).c_str());
                    } else if (counter != 0 && i == 2) {
                        u8g2.drawStr(10 + 30 * i, 29, std::to_string(settings.tune).c_str());
                    } else if (counter != 0 && i == 1) {
                        u8g2.drawStr(10 + 30 * i, 29, waveNames[settings.waveIndex].c_str());
                    } else {
                        u8g2.drawStr(10 + 30 * i, 29, bottomBar_menu[i].c_str());
                    }
                }
            }
            previous_knob1 = extractBits<inputSize, 2>(sysState.inputs, 16, 2);
            previous_knob2 = extractBits<inputSize, 2>(sysState.inputs, 14, 2);
            previous_knob3 = extractBits<inputSize, 2>(sysState.inputs, 12, 2);
            xSemaphoreGive(sysState.mutex);
            xSemaphoreGive(settings.mutex);
        }

        u8g2.setFont(u8g2_font_5x8_tr);
        // Display pressed keys
        std::vector<std::string> pressedKeys;
        for (int i = 0; i < 12; i++) {
            if (sysState.inputs[i] == 0) {
                pressedKeys.push_back(noteNames[i]);
            }
        }
        for (int i = 0; i < pressedKeys.size(); i++) {
            u8g2.setCursor(10 + 10 * i, pressedKeyH);
            u8g2.print(pressedKeys[i].c_str());
        }

        u8g2.sendBuffer();
        digitalToggle(LED_BUILTIN);
        uint32_t ENDTime = micros();
    }
}

// -------------------- Module: Background Audio Calculation --------------------
void backgroundCalcTask(void *pvParameters) {
    static float prevfloatAmp = 0;

    while (1) {
        uint32_t startTime = micros();
        // Wait for buffer availability
        xSemaphoreTake(sampleBufferSemaphore, portMAX_DELAY);
        uint32_t writeCtr = 0;

        while (writeCtr < SAMPLE_BUFFER_SIZE / 2) {
            int vol_knob_value = settings.volume;
            int version_knob_value = 8 - settings.waveIndex;

            bool hasActiveKey = false;
            int activeKeyCount = 0;
            float floatAmp = 0.0f;

            for (int i = 0; i < 96; ++i) {
                if (writeCtr >= SAMPLE_BUFFER_SIZE / 2) break;

                bool isActive = __atomic_load_n(&notes.notes[i].active, __ATOMIC_RELAXED);
                if (isActive) {
                    hasActiveKey = true;
                    activeKeyCount++;

                    float amp = 0.0f;

                    // Waveform generation based on knob selection
                    switch (version_knob_value) {
                        case 8:  // Sawtooth
                            amp = calcSawtoothAmp(&notes.notes[i].floatPhaseAcc, vol_knob_value, i);
                            break;
                        case 7:  // Sine
                            amp = getSample(notePhases[i], &notes.notes[i].floatPhaseAcc, sineTable);
                            break;
                        case 6:  // Square
                            amp = getSample(notePhases[i], &notes.notes[i].floatPhaseAcc, squareTable);
                            break;
                        case 5:  // Triangle
                            amp = getSample(notePhases[i], &notes.notes[i].floatPhaseAcc, triangleTable);
                            break;
                        case 4:  // Piano
                            amp = getSample(notePhases[i], &notes.notes[i].floatPhaseAcc, pianoTable);
                            break;
                        case 3:  // Saxophone
                            amp = getSample(notePhases[i], &notes.notes[i].floatPhaseAcc, saxophoneTable);
                            break;
                        case 2:  // Bell
                            amp = getSample(notePhases[i], &notes.notes[i].floatPhaseAcc, bellTable);
                            break;
                        case 1:  // Alarm
                            amp = getSample(notePhases[i], &notes.notes[i].floatPhaseAcc, squareTable);
                            floatAmp += calcHornVout(amp, vol_knob_value, i);
                            break;
                        default: // Random (dongTable)
                            amp = getSample(notePhases[i], &notes.notes[i].floatPhaseAcc, dongTable);
                            break;
                    }

                    // Effect Chain
                    floatAmp += addEffects(amp, vol_knob_value, i);
                    floatAmp = applyEffects(floatAmp);
                }

                // When reaching the last note, finalize this sample
                if (i == 95 && activeKeyCount > 0) {
                    floatAmp /= activeKeyCount;
                    floatAmp += addLFO(floatAmp, vol_knob_value);
                    floatAmp = addLPF(floatAmp, &prevfloatAmp);
                    uint32_t Vout = static_cast<uint32_t>(floatAmp);
                    writeToSampleBuffer(Vout, writeCtr++);
                }
            }

            // No keys active → write silence
            if (!hasActiveKey && writeCtr < SAMPLE_BUFFER_SIZE / 2) {
                writeToSampleBuffer(0, writeCtr++);
            }
        }
        Serial.println(micros() - startTime);
        vTaskDelay(1); // Yield to other tasks
    }
}

// -------------------- Task: Decode Received CAN Messages --------------------
void decodeTask(void * pvParameters) {
    Serial.println("decodeTask started!");
    while (1) {
        uint32_t startTime = micros();
        // Wait for incoming message
        xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);

        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        xSemaphoreTake(notes.mutex, portMAX_DELAY);
        xSemaphoreTake(settings.mutex, portMAX_DELAY);

        // Handle Note Press/Release for Main Board
        if (sysState.posId == 0) {
            if (RX_Message[0] == 'P') {
                sysState.inputs[RX_Message[1]] = 0;
                notes.notes[(RX_Message[2] - 1) * 12 + RX_Message[1]].active = true;
            } else if (RX_Message[0] == 'R') {
                sysState.inputs[RX_Message[1]] = 1;
                notes.notes[(RX_Message[2] - 1) * 12 + RX_Message[1]].active = false;
            }
        }
        // Forward Message from Board 0 to Board 1
        else if (RX_Message[3] == 0 && sysState.posId == 1 &&
                 (RX_Message[0] == 'P' || RX_Message[0] == 'R')) {
            RX_Message[3] = 1;
            xQueueSend(msgOutQ, const_cast<uint8_t*>(RX_Message), portMAX_DELAY);
        }

        // Update Tune
        if (RX_Message[0] == 'T') {
            Serial.println("Update tune!");
            settings.tune = RX_Message[1] + sysState.posId;
            settings.tune = constrain(settings.tune, 0, 8);
            Serial.println(settings.tune);
        }

        // New Board Join Request
        if (RX_Message[0] == 'N') {
            Serial.println("New board request received!");
            TX_Message[0] = 'U';
            TX_Message[1] = sysState.posId;
            TX_Message[2] = RX_Message[1];
            xQueueSend(msgOutQ, const_cast<uint8_t*>(TX_Message), portMAX_DELAY);
        }

        // Update Board ID Position
        if (RX_Message[0] == 'U' && RX_Message[2] == sysState.local_boardId) {
            if (RX_Message[1] >= sysState.posId) {
                sysState.posId = RX_Message[1] + 1;
                settings.tune = sysState.posId + 3;
                Serial.print("Update posId: ");
                Serial.println(sysState.posId);
            }
        }

        xSemaphoreGive(notes.mutex);
        xSemaphoreGive(sysState.mutex);
        xSemaphoreGive(settings.mutex);
        Serial.println(micros() - startTime);
    }
}

// -------------------- Task: CAN Message Transmit Handler --------------------
void CAN_TX_Task(void * pvParameters) {
    Serial.println("CAN_TX_Task started!");
    uint8_t msgOut[8];

    while (1) {
        // Wait for a message in TX queue
        xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
        xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);

        // Debug info
        Serial.print("TX: ");
        Serial.print((char)msgOut[0]);
        Serial.print(msgOut[1]);
        Serial.print(msgOut[2]);
        Serial.println();

        // Transmit CAN message
        CAN_TX(ID, msgOut);
    }
}

// -------------------- Utility: Fill Message Queue for Testing --------------------
void fillmsgQ() {
    uint8_t RX_Message[8] = {'P', 4, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 60; i++) {
        xQueueSend(msgOutQ, RX_Message, portMAX_DELAY);
    }
}

// -------------------- Task: Dummy CAN TX Timing Test --------------------
void time_CAN_TX_Task() {
    uint8_t msgOut[8];
    for (int i = 0; i < 60; i++) {
        xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
        // CAN_TX(ID, msgOut);  // Optional for testing
    }
}

// -------------------- Function: Measure Worst Case Background Calc Time --------------------
void backCalcTime() {
    setWorstcaseBackCalc();
    backgroundCalcTask(NULL);
}

// -------------------- Function: Measure Worst Case Display Update Time --------------------
void displayTime() {
    setWorstcaseDisplay();
    displayUpdateTask(NULL);
}

// -------------------- Function: Run Joystick Task Once --------------------
void runJoystickTaskOnce() {
    const int origin = 490;    // Joystick center position
    const int deadZone = 150;  // Ignore small fluctuations
    int joystickX = analogRead(JOYX_PIN);
    int joystickY = analogRead(JOYY_PIN);

    std::string currentMovement = "origin";

    // Determine joystick movement direction
    if (abs(joystickX - origin) <= deadZone && abs(joystickY - origin) <= deadZone) {
        currentMovement = "origin";
    } else if (joystickX - origin > deadZone) {
        currentMovement = "left";
    } else if (joystickX - origin < -deadZone) {
        currentMovement = "right";
    } else if (joystickY - origin > deadZone) {
        currentMovement = "down";
        // Scroll menu down
        if (selected_option < total_menu_items - 1) {
            selected_option++;
            if ((selected_option * ITEM_SPACING) >= (menu_offset + PAGE_HEIGHT)) {
                menu_offset += ITEM_SPACING;
            }
        }
    } else if (joystickY - origin < -deadZone) {
        currentMovement = "up";
        // Scroll menu up
        if (selected_option > 0) {
            selected_option--;
            if ((selected_option * ITEM_SPACING) < menu_offset) {
                menu_offset = std::max(0, menu_offset - ITEM_SPACING);
            }
        }
    }

    // Confirm movement only when joystick returns to center
    static std::string previousMovement = "origin";
    if (currentMovement != previousMovement && currentMovement == "origin") {
        movement = previousMovement;
    } else {
        movement = "origin";
    }
    previousMovement = currentMovement;
}

void joystickTime() {
    Serial.println("[Joystick Task] Start WCET test...");
    
    uint32_t startTime = micros();
    for (int i = 0; i < 32; i++) {
        runJoystickTaskOnce();
    }
    uint32_t endTime = micros();

    Serial.print("[Joystick Task] WCET for 32 iterations: ");
    Serial.println(endTime - startTime);
}

// -------------------- Function: Measure CAN TX Task Time --------------------
void canTXtime() {
    fillmsgQ();
    uint32_t startTime = micros();
    for (int i = 0; i < 32; i++) {
        time_CAN_TX_Task();
        fillmsgQ();
    }
    Serial.println(micros() - startTime);
}

// -------------------- Function: Measure Key Scan Task Time --------------------
void scankeyTime() {
    setWorstCaseScankey();
    for (int i = 0; i < 32; i++) {
        scanKeysTask(NULL);
    }
}

// -------------------- Function: Measure Decode Task Time --------------------
void decodeTime() {
    for (int i = 0; i < 32; i++) {
        decodeTask(NULL);
    }
}

// -------------------- Function: Test Setup Entry Point --------------------
void testSetup() {
    sysState.knobValues[2].current_knob_value = 4;
    sysState.knobValues[3].current_knob_value = 6;

    Serial.begin(9600);
    Serial.println("Serial port initialized");

    generatePhaseLUT();
    set_pin_directions();
    set_notes();
    init_settings();

    CAN_Init(true);
    setCANFilter(0x123, 0x7ff);
    CAN_RegisterRX_ISR(CAN_RX_ISR);
    CAN_RegisterTX_ISR(CAN_TX_ISR);
    CAN_Start();

    // Uncomment to test individually
    // joystickTime();
    // displayTime();
    // scankeyTime();
    backCalcTime();
    // canTXtime();
    // decodeTime();

    while (1) {}  // Keep running
}

void setup() {
    // Initialize Internal Knob Default State
    sysState.knobValues[2].current_knob_value = 4;
    sysState.knobValues[3].current_knob_value = 6;

    // Initialize Phase Lookup Table and Settings
    generatePhaseLUT();
    set_pin_directions();
    set_notes();
    init_settings();
    initEffects();

    // Initial Display Rendering
    initial_display();

    // Setup Semaphores
    CAN_TX_Semaphore = xSemaphoreCreateCounting(3, 3);
    sampleBufferSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(sampleBufferSemaphore); // Prime the buffer initially

    // Initialize CAN Communication
    CAN_Init(false);

    // Serial Communication Setup
    Serial.begin(9600);
    Serial.println("Serial port initialized");

    // Auto-Detect Board Position ID
    sysState.posId = auto_detect_init();
    delay(200);
    initial_display();

    Serial.print("posId: ");
    Serial.println(sysState.posId);
    sysState.knobValues[2].current_knob_value = sysState.posId + 3;

    Serial.print("UID: ");
    Serial.println(sysState.local_boardId);

    // Create System Tasks
    /*
    xTaskCreate(backgroundCalcTask, "BackCalc", 256, NULL, 4, &BackCalc_Handle);
    xTaskCreate(scanKeysTask, "scanKeys", 256, NULL, 6, &scanKeysHandle);
    xTaskCreate(displayUpdateTask, "displayUpdate", 256, NULL, 2, &displayUpdateHandle);
    xTaskCreate(decodeTask, "decode", 256, NULL, 5, &decodeTaskHandle);
    xTaskCreate(scanJoystickTask, "scanJoystick", 256, NULL, 1, &scanJoystick_Handle);
    xTaskCreate(CAN_TX_Task, "CAN_TX", 256, NULL, 3, &CAN_TX_Handle);
    */

    // Initialize Shared Resource Mutexes
    notes.mutex = xSemaphoreCreateMutex();
    sysState.mutex = xSemaphoreCreateMutex();
    settings.mutex = xSemaphoreCreateMutex();

    // Start RTOS Scheduler
    vTaskStartScheduler();
}

void loop() {
    testSetup();
}
