# LUGUAN Synthesizer

The LUGUAN Synthesizer is an embedded systems project that implements a real-time, multi-tasking music synthesizer. It features key scanning, waveform generation, CAN bus communication, and a comprehensive user interface—all built upon a robust FreeRTOS framework. This document consolidates system analysis, task descriptions, dependency details, and the user manual into one comprehensive README.

---

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
   - [Task Descriptions](#task-descriptions)
   - [Dependencies and Data Flow](#dependencies-and-data-flow)
   - [Critical Instant Analysis](#critical-instant-analysis)
3. [User Manual](#user-manual)
4. [License and Further Information](#license-and-further-information)

---

## Overview

The LUGUAN Synthesizer delivers full-featured audio synthesis using an STM32 microcontroller and the STM32duino framework. By leveraging both interrupts and multi-threading, the system concurrently handles key scanning, display updates, audio signal generation, and CAN bus messaging. The design emphasizes robust real-time performance, safe inter-task data sharing, and a user-friendly interface.

---

## System Architecture

### Task Descriptions

The system is composed of several tasks implemented either as threads or interrupts:

- **CAN_TX_ISR**  
  *Type:* Interrupt  
  *Description:* Triggered when a CAN message is successfully transmitted; it releases the semaphore to allow other transmissions.

- **CAN_RX_ISR**  
  *Type:* Interrupt  
  *Description:* Activated on CAN message reception; it immediately transfers the received message to the incoming queue (`msgInQ`).

- **CAN_TX_Task**  
  *Type:* Thread  
  *Description:* Waits for messages in the outgoing queue (`msgOutQ`) and transmits them via the CAN bus after acquiring the semaphore.

- **DecodeTask**  
  *Type:* Thread  
  *Description:* Processes incoming CAN messages from `msgInQ` and issues commands to other tasks based on the message content.

- **SampleISR**  
  *Type:* Interrupt  
  *Description:* Runs at a 22kHz sample rate to generate the output waveform by updating the phase accumulator and driving the speaker output.

- **ScanKeysTask**  
  *Type:* Thread  
  *Description:* Executes every 20ms to scan all inputs (keys, knobs, etc.) and detect board connectivity changes; updates system state and CAN messages as needed.

- **DisplayUpdateTask**  
  *Type:* Thread  
  *Description:* Runs every 100ms to update the OLED display, manage UI menus, and toggle the status LED.

- **ScanJoystickTask**  
  *Type:* Thread  
  *Description:* Continuously reads the joystick inputs and outputs movement data for UI interaction.

- **Auto-Detection**  
  *Type:* Function (run once at startup)  
  *Description:* Determines the board’s position by exchanging signals (West/East detection and CAN messages) when the system starts.

- **BackgroundCalcTask**  
  *Type:* Thread  
  *Description:* Uses double buffering to compute the audio output for all pressed keys. It handles polyphony by summing wave amplitudes, applies fade and ADSR envelope effects, injects LFO modulation, and performs low pass filtering.

---

### Dependencies and Data Flow

#### Shared Data Structures and Synchronization

- **Global Data Objects:**  
  - `sysState` (includes key matrix state and knob values)  
  - Other shared objects (e.g., settings, movement variables) are accessed via atomic operations or protected by mutexes.

- **Synchronization Mechanisms:**  
  - **Mutexes:** Protect shared objects (e.g., `sysState`) for tasks like ScanKeysTask, DisplayUpdateTask, and DecodeTask.  
  - **Atomic Operations:** Used for single-word data updates (e.g., current step size in the ISR).  
  - **Message Queues and Semaphores:**  
    - Incoming and outgoing CAN messages are buffered using queues (`msgInQ` and `msgOutQ`).  
    - A counting semaphore governs access to the CAN bus transmission mailboxes.

#### Inter-Task Data Flow Examples

- **Knob Change Event:**  
  A change in knob input is captured by ScanKeysTask and sent via the CAN_TX_Task. The CAN_RX_ISR and subsequent DecodeTask process the message, updating settings that the BackgroundCalcTask then uses to generate the correct audio output.

- **Joystick Movement:**  
  Joystick data is acquired by ScanJoystickTask and directly informs the DisplayUpdateTask to update the UI accordingly.

These dependency structures are designed to be one-way to prevent deadlocks. Data flows through clearly defined channels, and the use of mutexes and atomic operations ensures safe, timely access without resource contention.

---

### Critical Instant Analysis

The system’s real-time performance was analyzed under worst-case conditions. The table below summarizes the measured execution times, initiation intervals, and their impact on CPU usage:

| Task No | Task                                  | Execution Time T<sub>i</sub> (ms) | Initiation Time τ<sub>i</sub> (ms) | T<sub>i</sub>/τ<sub>i</sub> | CPU Usage Contribution |
|---------|---------------------------------------|-----------------------------------|------------------------------------|-----------------------------|------------------------|
| 1       | Decode                                | 0.014                             | 33.6                               | 0.04%                       | 0.04%                  |
| 2       | CAN_TX (excluding propagation delay)  | 0.299                             | 100                                | 29.9%                       | 29.9%                  |
| 3       | Joystick                              | 0.33                              | 100                                | 0.33%                       | 0.33%                  |
| 4       | Scan Keys                             | 0.112                             | 20                                 | 0.56%                       | 0.56%                  |
| 5       | Display                               | 17.2                              | 100                                | 17.2%                       | 17.2%                  |
| 6       | Background Calculation (single board) | 34.1                              | 50                                 | 68.2%                       | 68.2%                  |
| **Sum** |                                       |                                   |                                    |                             | **87%**                |

**Key Insights:**

- The overall CPU usage is approximately 87%, leaving headroom for additional features.
- The Background Calculation Task is the most CPU-intensive, as expected, because it computes the waveforms for polyphonic audio.
- These measurements inform task prioritization (e.g., ScanKeys > Decode > BackgroundCalc > CAN_TX > Display > Joystick) to prevent missed deadlines and ensure smooth operation.

---

## User Manual

The following instructions guide you through using the LUGUAN Synthesizer:

### Home Page

1. **Menu:**  
   Press **Knob 1** to open the menu page.
2. **Waveform Selection:**  
   Choose from seven waveform types: sawtooth, sine, square, triangle, saxophone, piano, and bell by rotating **Knob 2**.
3. **Tone Control:**  
   Set the master board’s octave using **Knob 3**; slave boards adjust accordingly.
4. **Volume Control:**  
   Increase volume by rotating **Knob 4** clockwise (Volume 1 is the lowest; Volume 8 is the highest).

### Menu Page

- Use the **joystick** to navigate between features.
- Press the joystick to select a feature.
- Adjust settings with **Knob 2**.

### Metronome

- Provides a steady beat for rhythm accuracy.
- Adjust the metronome speed using **Knob 3** (8 levels).

### Fade

- Manages sound sustain and fade effects.
- Adjust sustain time with **Knob 3** and fade speed with **Knob 4**.

### Low Frequency Oscillator (LFO)

- Adds a low-frequency sine wave for natural sound modulation.
- Adjust LFO frequency with **Knob 3** and LFO volume with **Knob 4**.

### ADSR Envelope

- Modifies tone amplitude with attack, decay, sustain, and release parameters.
- Set parameters using **Knobs 2, 3,** and **4** respectively.

### Low Pass Filter (LPF)

- Removes high-frequency components for smoother tone quality.
- Set the cutoff frequency with **Knob 3** (range: 500Hz to 2000Hz).

---

## License and Further Information

This synthesizer project is part of an embedded systems coursework initiative. For further details on development, troubleshooting, and additional features, please refer to the project repository and accompanying documentation. Contributions and suggestions for improvements are welcome.

*This README consolidates system analysis, task management details, dependency graphs, and user instructions to provide a full overview of the SpaceY Synthesizer. For any further questions or contributions, please consult the source repository or contact the development team.*
