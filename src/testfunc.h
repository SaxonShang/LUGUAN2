#include <math.h>
#include <stdlib.h>
#include <pin_definitions.h>



void setWorstcaseBackCalc(){
    //the worst case senerio for backcalculation thread is when all 48 keys on all 4 keyboards have been pressed
    //this function fills the first 48 index of the notes array, where about the array is filled does not make an impact on the excution time
    for (int i=12;i<24;i++){
        notes.notes[i].active=true;
    }
    // and it will be very time consuming to turn all of the features on, the value of the features do not matter much, we just use their default settings
    // settings.adsr.on=true;// either adsr or fade, adsr does more computation
    // settings.lfo.on=true;
    // settings.lowpass.on=true;
}
void setWorstcaseDisplay(){ // worst case of display is when all the keys have been pressed and it has to display all of them
    for (int i=0;i<12;i++){
        sysState.inputs[i]=0;
    }
}
void setWorstCaseJoystick(){

}
void setWorstCaseScankey(){
    for (int i=0;i<28;i++){// all 0 means all the key have been pressed, worst case
        sysState.inputs[i]=0;
    }
}
void setWorstCaseCanTx(){

}
void setWorstCaseDecode(){
    // sysstate.posID=0;
}
// void backCalcTime(){
//     setWorstcaseBackCalc();
//     uint32_t startTime = micros();
//     // delayMicroseconds(3);
//     // Serial.println(micros()-startTime);

    
// 	for (int iter = 0; iter < 32; iter++) {
// 		 backgroundCalcTask(NULL);
//      Serial.println(micros()-startTime);
// 	}
	

// }



// void testSetup(){
//   sysState.knobValues[2].current_knob_value = 4;
//   sysState.knobValues[3].current_knob_value = 6;
//   //Set pin directions
//   generatePhaseLUT();
//   set_pin_directions();
//   set_notes();
//   init_settings();
//   Serial.begin(9600);
//   Serial.println("Serial port initialised");
//   sysState.knobValues[2].current_knob_value = sysState.posId + 3;
//   backCalcTime();
// }