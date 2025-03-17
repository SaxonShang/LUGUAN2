# LUGUAN Synthesizer
*Demo Video:** [Watch Here]()

This document consolidates all aspects of the LUGUAN Synthesizer project into a single README. It covers task identification, timing and scheduling analysis, shared data structures and inter-task dependencies, as well as user instructions and additional notes.

---

## Table of Contents

1. [Task Identification and Implementation](#task-identification-and-implementation)
2. [Task Characterisation and Timing Analysis](#task-characterisation-and-timing-analysis)
3. [Critical Instant Analysis and CPU Utilisation](#critical-instant-analysis-and-cpu-utilisation)
4. [Shared Data Structures, Synchronisation, and Inter-Task Dependencies](#shared-data-structures-synchronisation-and-inter-task-dependencies)
5. [User Manual](#user-manual)
6. [Additional Notes](#additional-notes)

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

## 2. Task Characterisation and Timing Analysis

Each task is characterised by its theoretical minimum initiation interval and its worst-case execution time. These metrics are essential for ensuring that all real-time deadlines are met.

- **Theoretical Initiation Intervals:**  
  Defined by system requirements (e.g., ScanKeysTask runs every 20ms, DisplayUpdateTask every 100ms).

- **Measured Execution Times:**  
  Derived from extensive testing (refer to `testfunc.h` for details on worst-case scenarios).

---

## 3. Critical Instant Analysis and CPU Utilisation

The following table summarizes the worst-case execution times, initiation intervals, and CPU usage contributions of each task:

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
  The CPU utilisation is approximately **87%**, calculated using worst-case scenarios for all tasks, including certain IRQs (e.g., CAN_RX IRQ) that are minimal due to their short execution paths.

- **Task Performance:**  
  - The **BackgroundCalcTask** is the most demanding due to its comprehensive waveform computations. In an extreme worst-case (e.g., 48 keys pressed across 4 boards with all features enabled), execution time would be significantly higher; however, such scenarios are very rare.
  - A typical worst-case (all keys pressed on one board with minimal effects enabled) results in 68.2% CPU usage for that task.
  - Tasks like **DecodeTask** are extremely fast as they rely mainly on combinational logic.

- **CAN_TX Timing:**  
  Initial timing measurements for CAN_TX included the physical propagation delay (minimum 0.7ms). Since this delay does not consume CPU time, later measurements excluded it for accurate CPU usage calculation.

- **Priority Reordering:**  
  Based on the analysis, tasks have been reprioritized (from highest to lowest) as follows:  
  **ScanKeysTask (6) > DecodeTask (5) > BackgroundCalcTask (4) > CAN_TX_Task (3) > DisplayUpdateTask (2) > ScanJoystickTask (1)**  
  Correct priority settings are crucial to ensure timely execution and to prevent issues such as missed data or display malfunctions.

---

## 4. Shared Data Structures and Dependencies

This section outlines the global data objects used across tasks, the mechanisms in place to protect these shared resources, and details the data flows and dependencies between tasks.

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

### Inter-Task Dependencies and Detailed Data Flow

The tasks are designed with a unidirectional data flow to minimize the risk of deadlocks. Below are detailed examples of how data flows between tasks:

1. **Knob Input Change and Sound Generation:**
   - **Detection:**  
     The user rotates a knob, and **ScanKeysTask** detects this change by reading the key matrix and knob signals, updating the `sysState` object.
   - **CAN Transmission:**  
     The updated `sysState` triggers the creation of a CAN message. **CAN_TX_Task** picks up this message from the outgoing queue (`msgOutQ`) and transmits it over the CAN bus.
   - **Message Reception:**  
     The transmitted message is captured by **CAN_RX_ISR**, which places it into the incoming queue (`msgInQ`).
   - **Processing:**  
     **DecodeTask** processes the CAN message and updates the `settings` data structure with the new knob value.
   - **Sound Generation:**  
     Finally, **BackgroundCalcTask** uses the updated `settings` to compute new waveform amplitudes, which are then used by **SampleISR** to generate the corresponding audio output.

2. **Joystick Movement and UI Update:**
   - **Detection:**  
     When the user moves the joystick, **ScanJoystickTask** detects this movement and updates the `movement` data structure.
   - **UI Update:**  
     **DisplayUpdateTask** reads the updated `movement` data and adjusts the user interface accordingly—for example, scrolling through menu items or highlighting selections.
   - **Further Interaction:**  
     This flow can also trigger other events, such as refreshing the display or even prompting auto-detection routines if combined with other signals.

3. **Auto-Detection at Startup:**
   - **Initialisation:**  
     During system startup, the **Auto-Detection** function runs to determine the board’s position.
   - **Data Exchange:**  
     It exchanges information with adjacent boards using dedicated detection signals (e.g., West/East detection) and CAN messages.
   - **Configuration Update:**  
     The results of this auto-detection update `sysState` (and potentially `settings`), ensuring that each board correctly identifies its role (master or slave) in the synthesizer system.

4. **CAN Message Exchange and Data Synchronisation:**
   - **Routine Operation:**  
     Various tasks continuously exchange CAN messages to coordinate their functions.
   - **Transmission and Reception:**  
     **CAN_TX_Task** sends messages from `msgOutQ` using a counting semaphore to ensure that transmission mailboxes are available, while **CAN_RX_ISR** collects incoming messages into `msgInQ`.
   - **Processing and Update:**  
     **DecodeTask** processes these messages and accordingly updates shared data structures (such as `settings` or `sysState`), which then influence subsequent operations in other tasks like BackgroundCalcTask.

5. **Waveform Computation and Audio Output:**
   - **Data Buffering:**  
     **BackgroundCalcTask** computes waveform amplitudes based on the current key presses and effects settings, using double buffering to ensure continuous data availability.
   - **Audio Generation:**  
     **SampleISR** retrieves the buffered samples (with access controlled by a semaphore) and uses them to update the audio output via `analogWrite`, ensuring smooth and continuous sound production.

### Dependencies

The following table summarizes the dependencies of each task on the shared data objects. (Empty cells indicate that the data is not used by that task.)

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

### Dependency Diagram

Below is a placeholder for the dependency diagram that illustrates the overall structure.  
*(Replace "sPACE" with the correct image as needed.)*

![sPACE](https://github.com/Shiyizhuanshi/ES-synth-starter-Han/assets/105670417/c6b2db20-d41d-4658-b8cd-a84e755536b1)

---

## 5. User Manual

### LUGUAN Synthesizer User Manual

![sPACE](sPACE)

#### Home Page
1. **Menu:**  
   Press **Knob 1** to open the menu page.
2. **Waveform Selection:**  
   Choose from seven waveform types: sawtooth, sine, square, triangle, saxophone, piano, and bell by rotating **Knob 2**.
3. **Tone Control:**  
   Set the master board’s octave using **Knob 3**; slave boards adjust accordingly.
4. **Volume Control:**  
   Increase volume by rotating **Knob 4** clockwise (Volume 1 is the lowest; Volume 8 is the highest).

#### Menu Page
- Navigate between features using the **joystick**.
- Press the joystick to select a feature.
- Adjust settings with **Knob 2**.

#### Metronome (Met)
- Provides a steady beat for rhythm accuracy.
- Adjust metronome speed using **Knob 3** (8 levels).

#### Fade
- Controls sound sustain and fade effects.
- Adjust sustain time with **Knob 3** and fade speed with **Knob 4**.

#### Low Frequency Oscillator (LFO)
- Adds a low-frequency sine wave for natural sound modulation.
- Adjust LFO frequency with **Knob 3** and LFO volume with **Knob 4**.

#### ADSR Envelope
- Modifies tone amplitude using attack, decay, sustain, and release parameters.
- Set these parameters using **Knobs 2, 3,** and **4** respectively.

#### Low Pass Filter (LPF)
- Removes high-frequency components for smoother tone quality.
- Set the cutoff frequency with **Knob 3** (range: 500Hz to 2000Hz).

---

## 6. Additional Notes

- **Task Execution Testing:**  
  Worst-case scenarios for each task are documented in the source (refer to `testfunc.h`) to ensure all tasks meet their deadlines.

- **Priority Adjustments:**  
  If tasks do not perform as expected (e.g., missed data in DecodeTask or display lags), consult the established priority order and synchronization mechanisms.

- **Extensibility:**  
  With the current CPU utilisation at 87%, there is headroom for additional features. Any enhancements must preserve core functionality and allow restoration via the user interface or system reset.

- **Timing Analysis Recap:**  
  Detailed timing measurements indicate that while BackgroundCalcTask is the most demanding—especially under extreme worst-case conditions (e.g., 48 keys pressed across 4 boards)—such scenarios are rare. A more typical worst-case (all keys pressed on one board with minimal effects enabled) results in 68.2% CPU usage for that task, contributing to an overall CPU load of 87%.

- **CAN_TX Timing Consideration:**  
  Timing for CAN_TX was measured both with and without the physical propagation delay (minimum 0.7ms). The values excluding the propagation delay are used in final CPU utilisation calculations.

- **Reordering Task Priorities:**  
  Based on the analysis, tasks are prioritized (from highest to lowest) as follows:  
  **ScanKeysTask (6) > DecodeTask (5) > BackgroundCalcTask (4) > CAN_TX_Task (3) > DisplayUpdateTask (2) > ScanJoystickTask (1)**  
  Correct priority settings are crucial for ensuring timely execution and system stability.

---

*This document consolidates all critical aspects of the LUGUAN Synthesizer project. Please update the image placeholders as needed and refer to the source repository for further development details and troubleshooting.*
