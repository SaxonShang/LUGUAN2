# LUGUAN Synthesizer

This document consolidates all aspects of the LUGUAN Synthesizer project into a single README. It covers task identification, timing analysis and CPU utilisation, shared data structures with inter-task dependencies, as well as user instructions and additional notes.

---

## Table of Contents

1. [Task Identification and Implementation](#task-identification-and-implementation)
2. [Timing Analysis and CPU Utilisation](#timing-analysis-and-cpu-utilisation)
3. [Shared Data Structures, Synchronisation, and Inter-Task Dependencies](#shared-data-structures-synchronisation-and-inter-task-dependencies)
4. [User Manual](#user-manual)
5. [Additional Notes](#additional-notes)

---

## 1. Task Identification and Implementation

### Tasks and Their Implementation

- **CAN_TX_ISR**  
  *Type:* Interrupt  
  *Description:* Triggered when a CAN message is successfully transmitted; releases the semaphore to allow other transmissions.

- **CAN_RX_ISR**  
  *Type:* Interrupt  
  *Description:* Activated upon reception of a CAN message; transfers the received data to the incoming message queue (`msgInQ`).

- **CAN_TX_Task**  
  *Type:* Thread  
  *Description:* Waits for messages in the outgoing queue (`msgOutQ`) and transmits them on the CAN bus after acquiring a semaphore.

- **DecodeTask**  
  *Type:* Thread  
  *Description:* Processes messages from the incoming queue (`msgInQ`), interprets key press/release events, and updates system settings accordingly.

- **SampleISR**  
  *Type:* Interrupt  
  *Description:* Runs at a 22kHz sample rate to update the phase accumulator and generate the appropriate audio waveform output using `analogWrite`.

- **ScanKeysTask**  
  *Type:* Thread  
  *Description:* Executes every 20ms to scan all board inputs (keys, knobs, etc.), detect board connectivity changes, update the system state, and generate CAN messages based on key events.

- **DisplayUpdateTask**  
  *Type:* Thread  
  *Description:* Runs every 100ms to update the OLED display and manage UI menus, including toggling the status LED.

- **ScanJoystickTask**  
  *Type:* Thread  
  *Description:* Frequently reads the joystick inputs and outputs movement data to update the UI.

- **Auto-Detection**  
  *Type:* Function (executed once during setup)  
  *Description:* Determines board positioning by exchanging signals (via West/East detection and CAN communication) at startup.

- **BackgroundCalcTask**  
  *Type:* Thread  
  *Description:* Uses double buffering to compute the audio output for pressed keys. It handles polyphony by summing wave amplitudes, applies fade and ADSR envelope effects, injects low-frequency oscillation (LFO), and performs low pass filtering (LPF).

---

## 2. Timing Analysis and CPU Utilisation

The following table summarizes the worst-case execution times, initiation intervals, and CPU usage contributions of each task. These values capture both the theoretical and measured performance of the system:

| Task No | Task                                   | Execution Time T<sub>i</sub> (ms) | Initiation Time τ<sub>i</sub> (ms) | T<sub>i</sub>/τ<sub>i</sub> | CPU Usage Contribution |
|---------|----------------------------------------|-----------------------------------|------------------------------------|-----------------------------|------------------------|
| 1       | Decode                                 | 0.014                             | 33.6                               | 0.04%                       | 0.04%                  |
| 2       | CAN_TX (excluding propagation delay)   | 0.299                             | 100                                | 29.9%                       | 29.9%                  |
| 3       | Joystick                               | 0.33                              | 100                                | 0.33%                       | 0.33%                  |
| 4       | Scan Keys                              | 0.112                             | 20                                 | 0.56%                       | 0.56%                  |
| 5       | Display                                | 17.2                              | 100                                | 17.2%                       | 17.2%                  |
| 6       | Background Calculation (single board)  | 34.1                              | 50                                 | 68.2%                       | 68.2%                  |
| **Sum** |                                        |                                   |                                    |                             | **87%**                |

**Detailed Analysis:**

- **Overall CPU Usage:**  
  The CPU utilisation is approximately **87%**, based on worst-case scenarios for all tasks, including minimal contributions from IRQs (e.g., CAN_RX IRQ).

- **Task Performance Insights:**  
  - The **BackgroundCalcTask** is the most demanding, as it computes waveform amplitudes for all pressed keys and applies additional audio effects. In extreme worst-case conditions (e.g., 48 keys pressed across 4 boards with all features enabled), its execution time would be much higher; however, such scenarios are very rare.
  - A typical worst-case scenario (all keys pressed on one board with minimal effects enabled) results in 68.2% CPU usage for BackgroundCalcTask.
  - Fast tasks like **DecodeTask** rely primarily on combinational logic, ensuring minimal execution time.

- **CAN_TX Timing:**  
  Initial measurements of CAN_TX included a physical propagation delay (minimum 0.7ms). Since this delay does not consume CPU cycles, the final measurements exclude it to provide an accurate reflection of CPU usage.

- **Priority Reordering:**  
  The analysis led to a reordering of task priorities (from highest to lowest):  
  **ScanKeysTask (6) > DecodeTask (5) > BackgroundCalcTask (4) > CAN_TX_Task (3) > DisplayUpdateTask (2) > ScanJoystickTask (1)**  
  Correct priority settings are essential to prevent issues such as missed data transfers or display lag.

---

## 3. Shared Data Structures, Synchronisation, and Inter-Task Dependencies

This section outlines the global data objects used across tasks, the mechanisms protecting these shared resources, and detailed examples of how data flows between tasks.

### Global Data Objects

- **sysState:**  
  Contains the key matrix state, knob values, and other system parameters.

- **settings:**  
  Stores configuration parameters used across tasks.

- **movement:**  
  Holds data related to joystick movement.

### Synchronisation Mechanisms

- **Mutexes:**  
  Used to protect shared objects (e.g., `sysState` and `settings`). Tasks such as ScanKeysTask, DisplayUpdateTask, and DecodeTask lock these mutexes during access.

- **Atomic Operations:**  
  Employed for single-word data (e.g., the current step size variable) to ensure safe updates between ISRs and threads.

- **Message Queues and Semaphores:**  
  - **Incoming (msgInQ) and Outgoing (msgOutQ) CAN Message Queues:**  
    Provide safe buffering of CAN messages between ISRs and processing tasks.
  - **Counting Semaphore for CAN_TX:**  
    Regulates access to the three available CAN transmission mailboxes.

### Detailed Inter-Task Data Flow and Dependencies

1. **Knob Input Change and Sound Generation:**
   - **Detection:**  
     The user rotates a knob, and **ScanKeysTask** reads the change, updating the `sysState` object.
   - **CAN Message Creation:**  
     A CAN message is generated based on the updated knob input and queued in `msgOutQ`.
   - **Transmission & Reception:**  
     **CAN_TX_Task** sends the message on the CAN bus; **CAN_RX_ISR** captures the message and places it in `msgInQ`.
   - **Processing & Update:**  
     **DecodeTask** processes the message, updating the `settings` object with the new knob value.
   - **Audio Output:**  
     **BackgroundCalcTask** uses the updated `settings` to compute the new waveform amplitudes, which **SampleISR** then uses to generate the audio output.

2. **Joystick Movement and UI Update:**
   - **Detection:**  
     Movement of the joystick is detected by **ScanJoystickTask**, which updates the `movement` data structure.
   - **UI Response:**  
     **DisplayUpdateTask** reads the updated `movement` data and adjusts the user interface (e.g., scrolling menus, highlighting options).
   - **Further Interactions:**  
     This data flow can trigger additional routines such as auto-detection or other UI changes.

3. **Auto-Detection at Startup:**
   - **Initialisation:**  
     On startup, the **Auto-Detection** function determines the board’s position by exchanging signals via dedicated detection lines (West/East) and CAN messages.
   - **Configuration:**  
     The auto-detection results update `sysState` (and potentially `settings`), ensuring each board correctly identifies its role in the synthesizer system.

4. **CAN Message Exchange and Data Synchronisation:**
   - **Ongoing Operation:**  
     Regular CAN message exchanges are handled by **CAN_TX_Task** and **CAN_RX_ISR** with the help of the message queues.
   - **Data Processing:**  
     **DecodeTask** processes these messages to update shared data structures, which are then used by other tasks (e.g., BackgroundCalcTask for sound generation).

5. **Waveform Computation and Audio Output:**
   - **Buffering and Computation:**  
     **BackgroundCalcTask** computes waveform amplitudes based on the current key presses and effect settings, using a double-buffering strategy to ensure continuous data availability.
   - **Audio Generation:**  
     **SampleISR** accesses these buffers (with semaphore protection) and uses the samples to drive the audio output via `analogWrite`.

### Dependencies

The following table summarizes each task's dependency on the shared data objects. (Empty cells indicate the data is not used by that task.)

| Data Name             | sysState   | settings    | movement      | notes         |
|-----------------------|------------|-------------|---------------|---------------|
| **ScanKeysTask**      | Mutex      | Mutex       | Atomic Load   | Mutex         |
| **DisplayUpdateTask** | Mutex      | Mutex       | Atomic Load   | Null          |
| **BackgroundCalcTask**| Null       | Atomic Load | Null          | Atomic Load   |
| **ScanJoystickTask**  | Null       | Null        | Atomic Store  | Null          |
| **DecodeTask**        | Mutex      | Mutex       | Null          | Mutex         |
| **Sample ISR**        | Atomic Load| Atomic Load | Null          | Null          |

- **Additional Notes:**  
  - All RX & TX messages are protected by message queues and semaphores.
  - Two different buffers shared between BackgroundCalcTask and SampleISR are protected by a semaphore.
  - No deadlocks because the functions that reads to those structs do not have ability to write back to those blocks.
### Dependency Diagram

Below is a placeholder for the dependency diagram that illustrates the overall structure.  

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/keyboard.drawio (1).png)

---

## LUGUAN Keyboard User Manual

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/MENU.jpg)

### Home Page
1. **Menu:** Press **Knob 1** to open the menu page.
2. **Waveform Selection:** Choose from seven waveform types: *Sawtooth, Sine, Square, Triangle, Saxophone, Piano, and Bell*. Rotate **Knob 2** to switch.
3. **Octave Control:** Adjust the octave for the master board; slave boards follow accordingly. Use **Knob 3**.
4. **Volume Adjustment:** Increase or decrease volume by rotating **Knob 4** clockwise (*Level 1: Lowest, Level 8: Highest*).

### Menu Page
- Use the **joystick** to navigate through features.
- Press the **joystick** to select a feature.
- Press the **joystick** again to return to the menu page.
- Move to the second page if the menu list exceeds the page height.
- Adjust the state parameter (**On/Off**) using **Knob 2**.

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/MENB2.jpg)
![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/PAGE2.jpg)
### Metronome (Met)
- Provides steady beats for accurate rhythm.
- Adjust speed with **Knob 3** (*8 levels*).

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/MET.jpg)

### Fade
- Holds sound for a set duration, controlling sustain time and fade speed.
- Adjust **sustain time** with **Knob 3**.
- Adjust **fade speed** with **Knob 4**.

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/FADE.jpg)

### Low Frequency Oscillator (LFO)
- Adds a **low-frequency sine wave** for natural sound modulation.
- Adjust **frequency** with **Knob 3**.
- Adjust **LFO volume** with **Knob 4**.

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/OCI.jpg)

### ADSR (Attack, Decay, Sustain, Release)
- Modifies tone amplitude over time.
- Adjust **Attack** using **Knob 2**.
- Adjust **Decay** using **Knob 3**.
- Adjust **Sustain** using **Knob 4**.

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/ADSR.jpg)

### Low Pass Filter (LPF)
- Filters out high-frequency sounds for a smoother tone.
- Adjust **cutoff frequency** (*500Hz - 2000Hz*) using **Knob 3**.

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/LPF.jpg)

### Effect
- Rotate **Knob 2** to select an effect.
- Press **Knob 2** to enable the selected effect.
- Adjust the effect level using **Knob 3**.

The image below shows the page of effect in 3 effect mode
![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/EFF.jpg)
![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/DIS.jpg)
![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/CHO.jpg)

This manual provides an overview of the *LUGUAN Keyboard* functions, making it easy to navigate and customize your sound. Enjoy your music creation! 🎵

---
