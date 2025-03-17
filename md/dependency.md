# SpaceY Dependencies

The table below 
Empty means not using that data
| Data Name          | sysState | settings    | movement    | notes | 
|--------------------|----------|-------------|-------------|-------|
| ScanKeysTask       | Mutex    | Mutex       | Atomic Load | Mutex |
| DisplayUpdateTask  | Mutex    | Mutex       | Atomic Load | Null  |
| BackgroundCalcTask | Null     | Atomic Load | Null        | Atomic Load |
| ScanJoystickTask   | Null     | Null        | Atomic Store| Null  |
| DecodeTask         | Mutex    | Mutex       | Null        | Mutex |
| Sample ISR         | Atomic Load|Atomic Load|Null|Null|

All RX&TX Messages are protected by message queue and semaphore.
Two different buffers which are shared by BackCal and Sample ISR, buffers are protected by semephore.

Intertask blocking dependencies:

![image](https://github.com/Shiyizhuanshi/ES-synth-starter-Han/assets/105670417/c6b2db20-d41d-4658-b8cd-a84e755536b1)

The above graph shows the dependency structure of the tasks. All of the task dependencies are one way, therefore there is no possible deadlocks unless timing is failed and unexpected behaviors happen.

Some examples of data flow and dependency:

A knob is changed to change tone: ScanKeyTask (change systate)-> CANTX->CANRX->Decode task (changes settings)->backcal to use settings to generate sound

A joystick is moved to change menu: joysickscantask(changes movement)->displayupdate(changes update accurdingly)

No deadlocks because the functions that reads to those structs do not have ability to write back to those blocks. Even if they do, the part of the data they write back to are only accessed by themselves and not other structs. 