|Task no|Task|Execution time $T_{i}$ (ms)|Initiation time $\tau_{i}$ (ms)|$\frac{\tau_{7}T_{i}}{\tau_{i}}$|$\frac{T_{i}}{\tau_{i}}$|
| -------- | :--------: | :-------: | :--------: | :--------: | :--------: |
|1|Decode|0.014|33.6|0.041666667|0.04%|
|~~2~~|~~CAN_TX with propagation delay~~|~~58.0~~|~~100~~|~~58~~|~~58.00%~~|
|3|CAN_TX without propagation delay|0.299|100|0.299|29.9%|
|4|Joystick|0.33|100|0.33|0.33%|
|5|Scan keys|0.112|20|0.56|0.56%|
|6|Display|17.2|100|17.2|17.2%|
|~~7~~|~~Backcalc all features on 1 board~~|~~47.2~~|~~50~~|~~94.4~~|~~94.4%~~|
|~~8~~|~~Backcalc all features on 4 board~~|~~108.8~~|~~50~~|~~217.6~~|~~217.6%~~|
|9|Backcalc all features off 1 board|34.1|50|68.2|68.2%|
|~~10~~|~~Backcalc all features off 2 boards~~|~~46.2~~|~~50~~|~~92.4~~|~~92.4%~~|
|~~11~~|~~Backcalc all features off 4 board~~|~~70.3~~|~~50~~|~~140.6~~|~~140.6%~~|
| |Sum| | |86.63066667|87%

The CPU usage is 87%.
Critical Instant Analysis is performed on all of the tasks in our code using their worst case senarios. There are also some IRQs such as the CAN_RX IRQ, however, they are expected to be quite fast because the low code length and functions used. The Initiation time and excution time is recoded in the table above. 

It can be seen from the table that the backgroundcalculation task that calculates different amplitudes of different keys has the most CPU usage and excution time. This is expected because it has to create all of the waves for all of the keys that are pressed and each key's additional effect. With all 4 boards connected, and a worst case of 48 keys all pressed and all features enabled, the excution time simply goes through the roof. However, such worst senerio is extremely rare, and a more considarble worst case senerio is when all of the keys are pressed on 1 board, and all the effects are not on. We can see then it only uses 68.2%, leading a total CPU usage of 87%. Meaning there are still room for additional features to be enabled.

Other tasks do not use such a long time to excute, and they align perfectly with expectations. For example, the decode task is extremely fast, this is because it only contains combinational logic. The worst case senerios can be found in the testfunc.h file, and the specific worst case senario is commented there. Some tasks do not have worst case senerios, for example, joystickscan task, which all it does is scanning 1 joystick positions.

For the timing measurement on CAN_TX, it initially includes the propagation delay that exists during physical CAN bus data transmission. However, since the CAN bus transmission has a minimum propagation delay of at least 0.7ms and since the transmission of data does not occupy CPU space, it is decided to take another timing measurement on CAN_TX excluding this physical propagation delay. The obtained values are used in final calculations. 

From this timing anaylsis, we can also reorder the order of piority of all the tasks, namely:

scankey 6 decode 5 backcalc 4 cantx 3 display 2 joystick 1

If the piorities are not set right, some tasks will struggle to find time to excute its functions, we had problems such as data is not passed into the decode task, or the display task is not displaying properly.











