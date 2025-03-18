#include <math.h>
#include <stdlib.h>
#include <pin.h>

// -------------------------------------------------------------
// Set worst-case scenario for background calculation task
// Simulate all 48 keys (4 keyboards x 12 keys) being pressed.
// Enable heavy features for computational load.
// -------------------------------------------------------------
void setWorstcaseBackCalc() {
    for (int i = 12; i < 24; i++) {
        notes.notes[i].active = true;
    }

    // Optional: enable heavy features for maximum load
    // settings.adsr.on = true;
    // settings.lfo.on = true;
    // settings.lowpass.on = true;
}

// -------------------------------------------------------------
// Set worst-case scenario for display task
// Simulate all keys pressed so display has to render them all.
// -------------------------------------------------------------
void setWorstcaseDisplay() {
    for (int i = 0; i < 12; i++) {
        sysState.inputs[i] = 0; // Key pressed = 0
    }
}

// -------------------------------------------------------------
// Set worst-case scenario for scanKeys task
// Simulate all inputs as pressed for high-processing load.
// -------------------------------------------------------------
void setWorstCaseScankey() {
    for (int i = 0; i < 28; i++) {
        sysState.inputs[i] = 0; // All bits low = max load
    }
}

// -------------------------------------------------------------
// Set worst-case scenario for joystick task
// (Empty stub - optional implementation later)
// -------------------------------------------------------------
void setWorstCaseJoystick() {
    // Example idea: simulate extreme joystick deviation if needed
}

// -------------------------------------------------------------
// Set worst-case scenario for CAN TX task
// (Empty stub - optional implementation later)
// -------------------------------------------------------------
void setWorstCaseCanTx() {
    // Example idea: fill TX queue to simulate full pipeline
}

// -------------------------------------------------------------
// Set worst-case scenario for decode task
// (Stub for future simulation setup)
// -------------------------------------------------------------
void setWorstCaseDecode() {
    // sysState.posId = 0; // Uncomment if needed
}
