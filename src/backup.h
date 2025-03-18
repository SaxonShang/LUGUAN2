// void backgroundCalcTask(void * pvParameters){
//   static uint32_t  phaseAcc=0;

//   static float prevfloatAmp=0;
//   while(1){
//     // Serial.print("inbackcalc");
// 	  xSemaphoreTake(sampleBufferSemaphore, portMAX_DELAY);
//     uint32_t writeCtr=0;
//   while( writeCtr < SAMPLE_BUFFER_SIZE/2){
//       int vol_knob_value=__atomic_load_n(&sysState.knobValues[3].current_knob_value,__ATOMIC_RELAXED);
//       // int tune_knob_value=__atomic_load_n(&sysState.knobValues[2].current_knob_value,__ATOMIC_RELAXED);
//       int version_knob_value=__atomic_load_n(&sysState.knobValues[1].current_knob_value,__ATOMIC_RELAXED);
//       bool hasActiveKey=false;
//       int keynum=0;
//       float floatAmp=0;







//       for (int i = 0; i < 96; i++) {
//         if (writeCtr< SAMPLE_BUFFER_SIZE/2){

          
//           bool isactive=__atomic_load_n(&notes.notes[i].active,__ATOMIC_RELAXED);

//           if (version_knob_value ==8){
            
//             if (isactive) {
//                 uint32_t stepSize=__atomic_load_n(&stepSizes[i%12],__ATOMIC_RELAXED);                
//                 // hasActiveKey=true;
//                 int tune=(i-i%12)/12+1;
//                 if ((tune-4)>=0){
//                   phaseAcc+=stepSize << (tune-4);}
//                 else{
//                   phaseAcc+=stepSize >> -(tune-4);
//                 }
//                 uint32_t Vout = (phaseAcc >> 24) - 128;
//                 Vout = (Vout >> (8 - vol_knob_value))+128 ;
                
//                 writeToSampleBuffer(Vout,writeCtr);
//                 writeCtr+=1;
//             }
//           }
//           else if(version_knob_value ==7){
//               if (isactive) {
//                 keynum+=1;
//                 hasActiveKey=true;
//                 floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,sineTable);
//               }
//               if (i==95 && keynum>0){
//                 floatAmp=floatAmp/keynum;
//                 floatAmp=calcNoEnvelopeVout(floatAmp,vol_knob_value);
//                 writeToSampleBuffer(int(floatAmp)  , writeCtr);
//                 writeCtr+=1;
//             }

//           }
//           else if (version_knob_value ==6){

//             if (isactive) {
//                 // testsinAcc=notes.notes[i].sinAcc;
//                 keynum+=1;
//                 hasActiveKey=true;

//                 floatAmp+=calcOtherVout( getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,sineTable),vol_knob_value,i);

                
//               }
//             if (i==95 && keynum>0){
//               floatAmp=floatAmp/keynum;
//               writeToSampleBuffer(int(floatAmp)  , writeCtr);
//               writeCtr+=1;
//             }

//           }
//           else if (version_knob_value ==5){
//             if (isactive) {

//                 keynum+=1;
//                 hasActiveKey=true;
//                 floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,pianoTable);

//               }
//             if (i==95 && keynum>0){
//               floatAmp=floatAmp/keynum;
//               // floatAmp= lowPassFilter(floatAmp,prevfloatAmp,500.0);
//               // prevfloatAmp=floatAmp;
//               uint32_t Vout = static_cast<uint32_t>(floatAmp*255)-128;
//                 Vout = Vout >> (8 - vol_knob_value);
//               writeToSampleBuffer(Vout+128, writeCtr);
//               writeCtr+=1;
//             }
          

//           }
//           else if (version_knob_value ==4){
//              if (isactive){
//             keynum+=1;
//             hasActiveKey=true;
//             float phase=notePhases[i];
//             float amp=getSample(phase,&notes.notes[i].floatPhaseAcc, squareTable);
//             floatAmp+=calcHornVout(amp,vol_knob_value,i);

//              }
          
//           if (i==95 && keynum>0){
//               floatAmp=floatAmp/keynum;
//               uint32_t Vout = static_cast<uint32_t>(floatAmp*127)-128;
//                 Vout = Vout >> (8 - vol_knob_value);
//                 writeToSampleBuffer(Vout+128, writeCtr);
//                 writeCtr+=1;
//             }

//           }
//           else if (version_knob_value ==3){
//              if (isactive){
//             keynum+=1;
//             hasActiveKey=true;
//             float phase=notePhases[i];
//             float amp=getSample(phase,&notes.notes[i].floatPhaseAcc, saxophoneTable);
//             floatAmp+=calcPianoVout(amp,vol_knob_value,i);
//              }
          
//           if (i==95 && keynum>0){
//               floatAmp=floatAmp/keynum;
//               uint32_t Vout = static_cast<uint32_t>(floatAmp*127)-128;
//                 Vout = Vout >> (8 - vol_knob_value);
//                 writeToSampleBuffer(Vout+128, writeCtr);
//                 writeCtr+=1;
//             }

//           }
//           else if (version_knob_value ==2){
//              if (isactive){
//             keynum+=1;
//             hasActiveKey=true;
//             float phase=notePhases[i];
//             floatAmp+=getSample(phase,&notes.notes[i].floatPhaseAcc, saxophoneTable);
//              }
          
//           if (i==95 && keynum>0){
//               floatAmp=floatAmp/keynum;
//               // floatAmp+=generateLFO(1);
//               uint32_t Vout = static_cast<uint32_t>(floatAmp*127)-128;
//                 Vout = Vout >> (8 - vol_knob_value);
//                 writeToSampleBuffer(Vout+128, writeCtr);
//                 writeCtr+=1;
//             }

//           }

//           else{
//             if (isactive){
//               keynum+=1;
//               hasActiveKey=true;
//               floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,triangleTable);

//             }
//             if (i==95 && keynum>0){
//               floatAmp=floatAmp/keynum;
//               uint32_t Vout = static_cast<uint32_t>(floatAmp*255)-128;
//                 Vout = Vout >> (8 - vol_knob_value);
//                 writeToSampleBuffer(Vout+128, writeCtr);
//                 writeCtr+=1;
//             }
//           }
//         }
//       }
//       if(!hasActiveKey && writeCtr< SAMPLE_BUFFER_SIZE/2){
//             writeToSampleBuffer(0,writeCtr);
//             writeCtr+=1;
//           }

//       }
//         }  
// }