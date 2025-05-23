# LUGUAN Synthesizer

This report consolidates all aspects of the LUGUAN Synthesizer project into a single README. It covers task identification, timing analysis and CPU utilisation, shared data structures with inter-task dependencies, as well as user instructions.

---
## Check out the demo video
1. Function Display
   
[![Watch the video](https://img.youtube.com/vi/E8nPDUOH0bU/0.jpg)](https://www.youtube.com/watch?v=E8nPDUOH0bU)

2. Nice Song played

[![Watch the video](https://img.youtube.com/vi/A7JH9cY9dsU/0.jpg)](https://www.youtube.com/watch?v=A7JH9cY9dsU)


## Table of Contents

1. [Task Identification and Implementation](#task-identification-and-implementation)
2. [Timing Analysis and CPU Utilisation](#timing-analysis-and-cpu-utilisation)
3. [Shared Data Structures, Synchronisation, and Inter-Task Dependencies](#shared-data-structures-synchronisation-and-inter-task-dependencies)
4. [User Instruction](#user-instruction)

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

- **Auto-Positioning**  
  *Type:* Function (executed once during setup)  
  *Description:* Determines board positioning by exchanging signals (via West/East detection and CAN communication) at startup.

- **BackendTask**  
  *Type:* Thread  
  *Description:* Uses double buffering to compute the audio output for pressed keys. It handles polyphony by summing wave amplitudes, applies ADSR envelope effects, performs low pass filtering (LPF) and adds multiple effect buffers.

---

## 2. Timing Analysis and CPU Utilisation

The following table summarizes the worst-case execution times, initiation intervals, and CPU usage contributions of each task. These values capture both the theoretical and measured performance of the system:

| Task No | Task                                   | Execution Time T<sub>i</sub> (ms) | Initiation Time τ<sub>i</sub> (ms) | CPU Usage Contribution |
|---------|----------------------------------------|-----------------------------------|------------------------------------|------------------------|
| 1       | Decode                                 | 0.021                             | 33.6                               | 0.063%                 |
| 2       | CAN_TX                                 | 0.317                             | 100                                | 0.317%                 |
| 3       | Joystick                               | 0.327                             | 100                                | 0.327%                 |
| 4       | Scan Key                               | 0.133                             | 20                                 | 0.665%                 |
| 5       | Display                                | 18.408                            | 100                                | 18.408%                |
| 6       | Backend (single board)                 | 37.805                            | 50                                 | 75.61%                 |
| **Sum** |                                        |                                   |                                    | **95.39%**             |

**Detailed Analysis:**

- **Overall CPU Usage:**  
  The CPU utilisation is approximately **95.39%**, based on worst-case scenarios for all tasks, including minimal contributions from IRQs (e.g., CAN_RX IRQ).This means that all built-in functions utilizes nearly all computation power leaving a headroom for about **5%**.

- **Task Performance Insights:**  
  - The **BackendTask** is the most demanding, as it computes waveform amplitudes for all pressed keys and applies additional audio effects. In extreme worst-case conditions (e.g., 48 keys pressed across 4 boards with all features enabled), its execution time would be much higher; however, such scenarios are very rare.
  - A typical worst-case scenario (all keys pressed on one board with effects enabled) results in 75.61% CPU usage for Backend processing.
  - Fast tasks like **DecodeTask** rely primarily on combinational logic, ensuring minimal execution time.

- **Priority Reordering:**  
  The analysis led to a reordering of task priorities (from highest to lowest):  
  **ScanKeysTask (6) > DecodeTask (5) > BackendTask (4) > CAN_TX_Task (3) > DisplayUpdateTask (2) > ScanJoystickTask (1)**  
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
     **Backend** uses the updated `settings` to compute the new waveform amplitudes, which **SampleISR** then uses to generate the audio output.

2. **Joystick Movement and UI Update:**
   - **Detection:**  
     Movement of the joystick is detected by **ScanJoystickTask**, which updates the `movement` data structure.
   - **UI Response:**  
     **DisplayUpdateTask** reads the updated `movement` data and adjusts the user interface (e.g., scrolling menus, highlighting options).
   - **Further Interactions:**  
     This data flow can trigger additional routines such as auto-detection or other UI changes.

3. **Auto-Positioning at Startup:**
   - **Initialisation:**  
     On startup, the **Auto-Positioning** function determines the board’s position by exchanging signals via dedicated detection lines (West/East) and CAN messages.
   - **Configuration:**  
     The auto-positioning results update `sysState` (and potentially `settings`), ensuring each board correctly identifies its role in the synthesizer system.

4. **CAN Message Exchange and Data Synchronisation:**
   - **Ongoing Operation:**  
     Regular CAN message exchanges are handled by **CAN_TX_Task** and **CAN_RX_ISR** with the help of the message queues.
   - **Data Processing:**  
     **DecodeTask** processes these messages to update shared data structures, which are then used by other tasks (e.g., BackgroundCalcTask for sound generation).

5. **Waveform Computation and Audio Output:**
   - **Buffering and Computation:**  
     **Backend** computes waveform amplitudes based on the current key presses and effect settings, using a double-buffering strategy to ensure continuous data availability.
   - **Audio Generation:**  
     **SampleISR** accesses these buffers (with semaphore protection) and uses the samples to drive the audio output via `analogWrite`.

### Dependencies

The following table summarizes each task's dependency on the shared data objects. (Empty cells indicate the data is not used by that task.)

| Data Name             | sysState   | settings    | movement      | notes         |
|-----------------------|------------|-------------|---------------|---------------|
| **ScanKeysTask**      | Mutex      | Mutex       | Atomic Load   | Mutex         |
| **DisplayUpdateTask** | Mutex      | Mutex       | Atomic Load   | Null          |
| **BackendTask**       | Null       | Atomic Load | Null          | Atomic Load   |
| **ScanJoystickTask**  | Null       | Null        | Atomic Store  | Null          |
| **DecodeTask**        | Mutex      | Mutex       | Null          | Mutex         |
| **Sample ISR**        | Atomic Load| Atomic Load | Null          | Null          |

- **Additional Notes:**  
  - All RX & TX messages are protected by message queues and semaphores.
  - Two different buffers shared between BackgroundCalcTask and SampleISR are protected by a semaphore.
  - No deadlocks because the functions that reads to those structs do not have ability to write back to those blocks.
### Dependency Diagram

Below is a dependency diagram that illustrates the overall structure.  

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/pic.png)

---

## LUGUAN Keyboard User Instructions

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

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/MENUPAGE.jpg)
![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/PAGE2.jpg)
### Metronome (Met)
- Provides steady beats for accurate rhythm.
- Adjust speed with **Knob 3** (*8 levels*).

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/MET.jpg)

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

### Reverb (REV)

- Press **Knob 2** to enable the reverb.
- Adjust the reverb strength using **Knob 3**.

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/EFF.jpg)


### Distortion (DIS)

- Press **Knob 2** to enable the distortion.
- Adjust the distortion strength using **Knob 3**.
![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/DIS.jpg)

### Chorus (CHO)

- Press **Knob 2** to enable the chorus.
- Adjust the chorus strength using **Knob 3**.

![image](https://github.com/SaxonShang/LUGUAN2/blob/main/doc/CHO.jpg)


This manual provides an overview of the *LUGUAN Keyboard* functions, making it easy to navigate and customize your sound. Enjoy your music creation! 🎵

---
