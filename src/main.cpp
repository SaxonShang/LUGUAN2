#include <STM32FreeRTOS.h>
#include <ES_CAN.h>
#include <cmath>

#include "read_inputs.h"
#include "pin_definitions.h"
#include "read_inputs.h"
#include "wave_synth.h"
#include "menu.h"
#include "testfunc.h"
#include "effect.h"
// Writes the calculated sample value into the active buffer
void writeToSampleBuffer(uint32_t Vout, uint32_t writeCtr){
  if (writeBuffer1){
                sampleBuffer1[writeCtr] = Vout;
              }
            else{
                sampleBuffer0[writeCtr] = Vout ;
            }
}
// Background calculation task for audio processing
void backgroundCalcTask(void * pvParameters){
  static float prevfloatAmp=0;
  while(1){
    xSemaphoreTake(sampleBufferSemaphore, portMAX_DELAY);
    uint32_t writeCtr=0;
    while( writeCtr < SAMPLE_BUFFER_SIZE/2){
      int vol_knob_value = settings.volume; 
      int version_knob_value = 8 - settings.waveIndex;
      bool hasActiveKey=false;
      int keynum=0;
      float floatAmp=0;
      for (int i=0; i<96;i++){
        if (writeCtr< SAMPLE_BUFFER_SIZE/2){        
          bool isactive=__atomic_load_n(&notes.notes[i].active,__ATOMIC_RELAXED);
          if (isactive){
            keynum+=1;
            hasActiveKey=true;
            if (version_knob_value==8){//sawtooth
              float Amp=calcSawtoothAmp(&notes.notes[i].floatPhaseAcc,vol_knob_value,i);
              // u_int32_t Vout=
              floatAmp+=addEffects(Amp,vol_knob_value,i);
              floatAmp= applyEffects(floatAmp);

            }
            else if(version_knob_value==7){//sin wave
              float Amp=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,sineTable);
              floatAmp+=addEffects(Amp,vol_knob_value,i);
              floatAmp= applyEffects(floatAmp);
            }
            else if(version_knob_value==6){//square wave
              // floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,squareTable);
              float Amp=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,squareTable);
              floatAmp+=addEffects(Amp,vol_knob_value,i);
              floatAmp= applyEffects(floatAmp);
            }
            else if(version_knob_value==5){//triangle wave
              // floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,triangleTable);
              float Amp=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,triangleTable);
              floatAmp+=addEffects(Amp,vol_knob_value,i);
              floatAmp= applyEffects(floatAmp);
            }
            else if(version_knob_value==4){//piano
              // floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,pianoTable);
              float Amp=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,pianoTable);
              floatAmp+=addEffects(Amp,vol_knob_value,i);
              floatAmp= applyEffects(floatAmp);

            }
            else if(version_knob_value==3){//saxophone
              // floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,saxophoneTable);
              float Amp=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,saxophoneTable);
              floatAmp+=addEffects(Amp,vol_knob_value,i);
              floatAmp= applyEffects(floatAmp);
            }
            else if(version_knob_value==2){//bell
              // floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,bellTable);
              float Amp=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,bellTable);
              floatAmp+=addEffects(Amp,vol_knob_value,i);
              floatAmp= applyEffects(floatAmp);
            }
            else if(version_knob_value==1){//alarm
             
              float amp=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc, squareTable);
              floatAmp+=calcHornVout(amp,vol_knob_value,i);
              floatAmp+=addEffects(amp,vol_knob_value,i);
              floatAmp= applyEffects(floatAmp);
              
            }
            else{ //random
              floatAmp+=getSample(notePhases[i],&notes.notes[i].floatPhaseAcc,dongTable);
              floatAmp= applyEffects(floatAmp);
            }

          }
          if (i==95 && keynum>0){
              floatAmp=floatAmp/keynum;
              floatAmp+=addLFO(floatAmp,vol_knob_value);
              floatAmp=addLPF(floatAmp,&prevfloatAmp);
              u_int32_t Vout=static_cast<u_int32_t>(floatAmp);
              writeToSampleBuffer(Vout, writeCtr);
              writeCtr+=1;
            }
        }
      }
      if(!hasActiveKey && writeCtr< SAMPLE_BUFFER_SIZE/2){
            writeToSampleBuffer(0,writeCtr);
            writeCtr+=1;
          }
    }
    vTaskDelay(1);  // 让出 CPU
  }

}

void sampleISR() {
  // Serial.println("isr");
  static uint32_t readCtr = 0;
  static uint32_t metronomeCounter=0;
  int metronomespeed=__atomic_load_n(&settings.metronome.speed,__ATOMIC_RELAXED);
  bool metroOn=__atomic_load_n(&settings.metronome.on,__ATOMIC_RELAXED);
  int posId=__atomic_load_n(& sysState.posId ,__ATOMIC_RELAXED);
  if (posId == 0 ){
    if (readCtr == SAMPLE_BUFFER_SIZE/2) {
      readCtr = 0;
      writeBuffer1 = !writeBuffer1;
      presssedTimeCount();
      xSemaphoreGiveFromISR(sampleBufferSemaphore, NULL);
      }
    if (metronomespeed!=8 && metronomeCounter>=metronomeTime[7-metronomespeed] && metroOn){ 
      analogWrite(OUTR_PIN, 255);
      metronomeCounter=0;
    }
    else{
      if (writeBuffer1){
        
        analogWrite(OUTR_PIN, sampleBuffer0[readCtr++]);
        metronomeCounter+=1;
        }
      else{ 
        analogWrite(OUTR_PIN, sampleBuffer1[readCtr++]);
        metronomeCounter+=1;
    }
    }
  }
  else{
     if (readCtr == SAMPLE_BUFFER_SIZE/2) {
      readCtr=0;
      xSemaphoreGiveFromISR(sampleBufferSemaphore, NULL);
     }
     else{
      readCtr++;
     }
  }
}

void CAN_RX_ISR (void) {
	uint8_t RX_Message_ISR[8];
	CAN_RX(ID, RX_Message_ISR);
	xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
}

void CAN_TX_ISR (void) {
	xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}

void send_handshake_signal(int stateW, int stateE){
    setRow(5);
    delayMicroseconds(3);
    digitalWrite(OUT_PIN, stateW);
    delayMicroseconds(3);
    setRow(6);
    delayMicroseconds(3);
    digitalWrite(OUT_PIN, stateE);
    delayMicroseconds(3);
}

// reutrn the posId of the board
int auto_detect_init(){
  uint8_t detext_RX_Message[8] = {0};  //CAN RX message
  uint8_t detext_TX_Message[8] = {0};  //CAN RX message
  uint32_t detect_CAN_ID = 0x123; //CAN ID
  int wait_count = 300;
  int ready_board_count = 0;
  std::bitset<28> inputs;
  std::bitset<1> WestDetect;
  std::bitset<1> EastDetect;
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(20, 20);
  u8g2.print("Auto Detecting...");
  u8g2.sendBuffer();
  for (int i = 0; i < 100; i++){
      send_handshake_signal(1,1);
      delay(30);
  }
  inputs = readInputs();
  WestDetect = extractBits<28, 1>(inputs, 23, 1);
  EastDetect = extractBits<28, 1>(inputs, 27, 1);
  delay(2000);
  //after 3s, sends handshake status to the one board to process
  //if the board is the most west then it is the main board which needs to 
  //process the handshake status of the other boards
  //if not then it is the other boards which need to send the handshake status to the main board
  if (!WestDetect[0]){
    Serial.println("west borad detected");
    // if west detect, waiting for message from the main board
    do{
        Serial.println("waiting for main board message!");
        delay(100);
        while (CAN_CheckRXLevel())
        CAN_RX(detect_CAN_ID, detext_RX_Message);
        Serial.println("RX: ");
        Serial.print((char)detext_RX_Message[0]);
        Serial.println(detext_RX_Message[1]);
    } while (detext_RX_Message[0] !=  'C' && detext_RX_Message[1] != 0);
    Serial.println("main board message confirmed!");
    // when main board is determined, set all handshake signals to 0
    // give time for all boards to receive the handshake signal
    for (int i = 0; i < 10; i++){
        send_handshake_signal(0,0);
        delay(30);
    }
    // keep updating the handshake signal until the west is detected
    do{
        inputs = readInputs();
        WestDetect = extractBits<28, 1>(inputs, 23, 1);
        // Serial.println(WestDetect[0]);
        delay(10);
    } while (WestDetect[0]);
    Serial.println("update west detect!");
    delay(200);
    //once west is detected, wait the message from west board
    do{
        Serial.println("waiting for west board message!");
        while (CAN_CheckRXLevel()) CAN_RX(detect_CAN_ID, detext_RX_Message);
        Serial.println(CAN_CheckRXLevel());
        Serial.print((char)detext_RX_Message[0]);
        Serial.println(detext_RX_Message[1]);
    } while (detext_RX_Message[0] != 'M');

    for (int i = 0; i < 10; i++){
        send_handshake_signal(1,1);
        delay(10);
    }
    Serial.println("update east detect!");
    detext_TX_Message[0] = 'M';
    detext_TX_Message[1] = detext_RX_Message[1] + 1;
    //send board position to next board
    if (detext_RX_Message[1] != 2){
      CAN_TX(detect_CAN_ID, detext_TX_Message);
      Serial.println("send board pos to east board");
      Serial.print((char) detext_TX_Message[0]);
      Serial.println(detext_TX_Message[1]);
    }
    else{
      Serial.println("I am the most east board!");
      Serial.println(detext_TX_Message[1]);
    }
    return detext_TX_Message[1];
  }
  else{
    delay(500);
    Serial.println("I am the main board!");
    // if nothing on the west, then it is the main board with position 0
    detext_TX_Message[0] = 'C';
    detext_TX_Message[1] = 0;
    // tell the other boards that the main board is determined
    CAN_TX(detect_CAN_ID, detext_TX_Message);
    Serial.println("send main board message to other boards!");
    //give the other boards time to receive the message
    delay(100);
    send_handshake_signal(1,1);
    //give the other boards time to update West Detect
    delay(200);
    detext_TX_Message[0] = 'M';
    CAN_TX(detect_CAN_ID, detext_TX_Message);
    Serial.println("send board info to other boards!");
    Serial.print((char) detext_TX_Message[0]);
    Serial.println(detext_TX_Message[1]);
    return 0;
  }
}

void scanJoystickTask(void * pvParameters) {
  const TickType_t xFrequency3 = 100/portTICK_PERIOD_MS;
  int joystickX;
  int joystickY;
  int origin = 490;
  int deadZone = 150;
  
  std::string previous_movement = "origin";
  std::string current_movement;
  TickType_t xLastWakeTime3 = xTaskGetTickCount();
  while (1) {
    vTaskDelayUntil( &xLastWakeTime3, xFrequency3);
    joystickX = analogRead(JOYX_PIN);
    joystickY = analogRead(JOYY_PIN);
    if (abs(joystickX - origin) <= deadZone && abs(joystickY - origin) <= deadZone){
      current_movement = "origin";
    }
    else if (joystickX - origin > deadZone){
      current_movement = "left";
    }
    else if (joystickX - origin < -deadZone){
      current_movement = "right";
    }
    else if (joystickY - origin > deadZone){
      current_movement = "down";
    if (selected_option < total_menu_items - 1) {
                selected_option++;

                // **确保滚动菜单**
                if ((selected_option * ITEM_SPACING) >= (menu_offset + PAGE_HEIGHT)) {
                    menu_offset += ITEM_SPACING;
                }
            }

    }
    else if (joystickY - origin < -deadZone){
      current_movement = "up";

   if (selected_option > 0) {
                selected_option--;

                // **确保菜单不会超出顶部**
                if ((selected_option * ITEM_SPACING) < menu_offset) {
                    menu_offset = std::max(0, menu_offset - ITEM_SPACING);
                }
            }
    }

    if (current_movement != previous_movement && current_movement == "origin"){
      movement = previous_movement;
    }
    else{movement = "origin";}
    // Serial.println(movement.c_str());
    previous_movement = current_movement;
  }
}

void scanKeysTask(void * pvParameters) {
  Serial.println("scanKeysTask started!");
  const TickType_t xFrequency1 = 20/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime1 = xTaskGetTickCount();

  std::bitset<12> keys;
  std::bitset<8> current_knobs;
  std::bitset<4> current_knobs_click;

  std::bitset<12> previou_keys("111111111111");
  std::bitset<8> previous_knobs("00000000");
  std::bitset<4> previous_knobs_click("0000");

  std::bitset<1> currentJoystickState{"0"};
  std::bitset<1> oldJoystickState{"0"};

  std::bitset<1> old_WestDetect;
  std::bitset<1> old_EastDetect;

  int previousTune = 0;
  int currentTune = 0;
  while (1){ 
    vTaskDelayUntil( &xLastWakeTime1, xFrequency1);

    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    xSemaphoreTake(notes.mutex, portMAX_DELAY);
    xSemaphoreTake(settings.mutex, portMAX_DELAY);
    sysState.inputs = readInputs();
    
    keys = extractBits<inputSize, 12>(sysState.inputs, 0, 12);
    sysState.WestDetect = extractBits<28, 1>(sysState.inputs, 23, 1);
    sysState.EastDetect = extractBits<28, 1>(sysState.inputs, 27, 1);
    current_knobs = extractBits<inputSize, 8>(sysState.inputs, 12, 8);
    current_knobs_click = extractBits<inputSize, 4>(sysState.inputs, 20, 2).to_ulong() << 2 | extractBits<inputSize, 4>(sysState.inputs, 24, 2).to_ulong();
    currentJoystickState = extractBits<inputSize, 1>(sysState.inputs, 22, 1).to_ulong();

    if (currentJoystickState != oldJoystickState && currentJoystickState == 0){
      sysState.joystickState = !sysState.joystickState;
    }

    updateKnob(sysState.knobValues, previous_knobs, current_knobs, previous_knobs_click, current_knobs_click);

    update_menu_settings(sysState.currentMenu, currentTune);

    if (currentTune != previousTune && sysState.posId == 0 && !sysState.EastDetect[0]){
      Serial.println("tune value changed!");
      __atomic_store_n(&settings.tune , currentTune, __ATOMIC_RELAXED);
      // settings.tune = currentTune;
      TX_Message[0] = 'T';
      TX_Message[1] = currentTune;
      xQueueSend( msgOutQ, const_cast<uint8_t*>(TX_Message), portMAX_DELAY);
    }


    // Serial.println(settings.waveIndex);
    //if there is nothing on the west and east before, but now there is something on the west or east
    //then update the posId
    if (old_EastDetect[0] && old_WestDetect[0] && (!sysState.WestDetect[0])){
      Serial.println("request posId");
      delay(100);
      TX_Message[0] = 'N';
      TX_Message[1] = sysState.local_boardId;
      xQueueSend( msgOutQ, const_cast<uint8_t*>(TX_Message), portMAX_DELAY);
    }

    // if nothing on west and east then local play mode
    if (sysState.EastDetect[0] && sysState.WestDetect[0]){
      sysState.posId = 0;
      // int tune = settings.tune;
      int tune=__atomic_load_n(&settings.tune,__ATOMIC_RELAXED);
      for (int i = 0; i < 12; i++){
        if (keys.to_ulong() != 0xFFF){
          if (!keys[i]) {
            notes.notes[(tune-1)*12+i].active = true;
          }
          else{
            notes.notes[(tune-1)*12+i].active =false;
          }
        }
        else{
          notes.notes[(tune-1)*12+i].active =false;
        }
      }
    }

    // transmit mode, do not play note locally
    else{
      for (int i = 0; i < 12; i++){
        if (keys[i] != previou_keys[i]){
          TX_Message[0] = keys[i] ? 'R' : 'P';
          TX_Message[1] = i;
          TX_Message[2] = settings.tune;
          TX_Message[3] = sysState.posId;
          xQueueSend( msgOutQ, const_cast<uint8_t*>(TX_Message), portMAX_DELAY);
        }
      }
    }

    previousTune = currentTune;
    previou_keys = keys;
    previous_knobs = current_knobs;
    previous_knobs_click = current_knobs_click;
    oldJoystickState = currentJoystickState;
    old_WestDetect = sysState.WestDetect;
    old_EastDetect = sysState.EastDetect;
    xSemaphoreGive(sysState.mutex);
    xSemaphoreGive(notes.mutex);
    xSemaphoreGive(settings.mutex);
  }
}

void displayUpdateTask(void * pvParameters) {
  const TickType_t xFrequency2 = 100/portTICK_PERIOD_MS;
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
    vTaskDelayUntil( &xLastWakeTime2, xFrequency2);
    u8g2.clearBuffer();
    

    if (sysState.posId != 0){
      u8g2.setCursor(30, 15);
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.print("Slave board");
      pressedKeyH = 25;
    }
    else{
      pressedKeyH = 20;
      if (counter != 0){
        counter --;
      }
      xSemaphoreTake(sysState.mutex, portMAX_DELAY);
      
      xSemaphoreTake(settings.mutex, portMAX_DELAY);



      if(sysState.knobValues[0].clickState){
        u8g2.setFont(u8g2_font_6x10_tr);
        if (movement == "down"){
          index += 3;
        }
        else if (movement == "up"){
          index -= 3;
        }
        else if (movement == "right"){
          index += 1;
        }
        else if (movement == "left"){
          index -= 1;
        }
        index = constrain(index, 0, 6);

        if ((index * ITEM_SPACING) >= (menu_offset + PAGE_HEIGHT)) {
            menu_offset += ITEM_SPACING;
        } else if ((index * ITEM_SPACING) < menu_offset) {
            menu_offset -= ITEM_SPACING;
        }

        // for (int i = 0; i < 7; i++){
        //   if (i< 3){
        //     u8g2.drawStr(10 + 35 * i, 14, menu_first_level[i].c_str());
        //   }
        //   else{
        //     u8g2.drawStr(10 + 35 * (i-3), 25, menu_first_level[i].c_str());
        //   }
        //   if (index == i){
        //     int x_pos = (i < 3) ? 10 + 35 * i : 10 + 35 * (i-3);
        //     int y_pos = (i < 3) ? 5 : 15;
        //     u8g2.drawFrame(x_pos-3, y_pos, 32, 12);
        //   }
        // }
        for (int i = 0; i < 7; i++) {
            int x_pos, y_pos;

            if (i < 3) {  // 第一行
                x_pos = 10 + 35 * i;
                y_pos = 14;
            } 
            else if (i < 6) {  // 第二行
                x_pos = 10 + 35 * (i - 3);
                y_pos = 25;
            } 
            else {  // 第三行（只有一个 "Effect" 选项）
                x_pos = 10 + 35;  // 让 Effect 置于中间
                y_pos = 36;
            }

              if (index == 6) {  
                  if (menu_offset < ITEM_SPACING * 2) {
                      menu_offset += 2;  // **缓慢滑动**
                  }
              } else if (index != 6) {
                  menu_offset = 0;  // **缓慢回到默认位置**
              }
            // 绘制菜单项
            // u8g2.drawStr(x_pos, y_pos, menu_first_level[i].c_str());

            // 高亮选中的菜单项
            // if (index == i) {
            //   if (index == 6) {  // 选中了 effect block
            //     if ((index * ITEM_SPACING) >= (menu_offset + PAGE_HEIGHT - ITEM_SPACING)) {
            //         menu_offset = (index * ITEM_SPACING) - (PAGE_HEIGHT - ITEM_SPACING); 
            //     }
            // } else {
            //     // 确保菜单不会超出顶部
            //     if ((index * ITEM_SPACING) < menu_offset) {
            //         menu_offset = std::max(0, menu_offset - ITEM_SPACING);
            //     }
            // }
            //     u8g2.drawFrame(x_pos - 3, y_pos - 9, 32, 12);
            // }
          int final_y_pos = y_pos - menu_offset;

    // **如果在可见区域，才绘制**
          if (final_y_pos >= 10 && final_y_pos < PAGE_HEIGHT + 10) {
              u8g2.setCursor(x_pos, final_y_pos);
              u8g2.print(menu_first_level[i].c_str());

        // 高亮选中的菜单项
        if (index == i) {
            u8g2.drawFrame(x_pos - 3, final_y_pos - 9, 32, 12);
        }
          }

        }
        if (sysState.joystickState){
          menu(index, settings);
        }
      }
      else{
        sysState.currentMenu = "Main";
        u8g2.setCursor(15, 10);
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.print("LUGUAN Keyboard");
        u8g2.setFont(u8g2_font_5x8_tr);
        for (int i = 0; i < 4; i++){
          u8g2.drawFrame(8+30*i, 20, 25, 20);
          if (i !=0 && 
            (extractBits<inputSize, 2>(sysState.inputs, 12, 2) != previous_knob3 ||
              extractBits<inputSize, 2>(sysState.inputs, 14, 2) != previous_knob2 ||
              extractBits<inputSize, 2>(sysState.inputs, 16, 2) != previous_knob1)){
            counter = stay_time/100;
          }
          if (counter != 0  && i ==3){
            u8g2.drawStr(10+30*i, 29, std::to_string(settings.volume).c_str());
          }
          else if (counter != 0 && i ==2 ){
            u8g2.drawStr(10+30*i, 29, std::to_string(settings.tune).c_str());
          }
          else if (counter != 0 && i ==1 ){
            u8g2.drawStr(10+30*i, 29, waveNames[settings.waveIndex].c_str());
          }
          else{
            u8g2.drawStr(10+30*i, 29, bottomBar_menu[i].c_str());
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
    //display pressed keys
    std::vector<std::string> pressedKeys;
    for (int i = 0; i < 12; i++){
      if (sysState.inputs[i] == 0){
        pressedKeys.push_back(noteNames[i]);
      }
    }
    for (int i = 0; i < pressedKeys.size(); i++){
      u8g2.setCursor(10+ 10*i, pressedKeyH);
      u8g2.print(pressedKeys[i].c_str());
    }

    u8g2.sendBuffer();
    digitalToggle(LED_BUILTIN);
  }
}

void decodeTask(void * pvParameters) {
  Serial.println("decodeTask started!");
  while (1) {
    xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    xSemaphoreTake(notes.mutex, portMAX_DELAY);
    xSemaphoreTake(settings.mutex, portMAX_DELAY);
    // for (int i=0; i<48; i++){
    if (sysState.posId == 0){
      if (RX_Message[0] == 'P'){
        sysState.inputs[RX_Message[1]] = 0;
        notes.notes[(RX_Message[2]-1)*12+RX_Message[1]].active = true;
      }
      else if (RX_Message[0] == 'R'){
        sysState.inputs[RX_Message[1]] = 1;
        notes.notes[(RX_Message[2]-1)*12+RX_Message[1]].active = false;
      }
    }
    // // if board 1 receives a message from board 0, it will send the message back to the board 0
    else if(RX_Message[3] == 0 && sysState.posId == 1
          && (RX_Message[0] == 'R' || RX_Message[0] == 'P')){
        RX_Message[3] = 1;
        xQueueSend( msgOutQ, const_cast<uint8_t*>(RX_Message), portMAX_DELAY);
    }
    
    if (RX_Message[0] == 'T'){
      Serial.println("update tune!");
      settings.tune = RX_Message[1] + sysState.posId;
      settings.tune = constrain(settings.tune, 0, 8);
      Serial.println(settings.tune);
    }

    if (RX_Message[0] == 'N'){
      Serial.println("new board request receriaved!");
      TX_Message[0] = 'U';
      TX_Message[1] = sysState.posId;
      TX_Message[2] = RX_Message[1];
      xQueueSend( msgOutQ, const_cast<uint8_t*>(TX_Message), portMAX_DELAY);
    }

    if (RX_Message[0] == 'U' && RX_Message[2] == sysState.local_boardId){
      if (RX_Message[1] >= sysState.posId){
        sysState.posId = RX_Message[1] + 1;
        settings.tune = sysState.posId + 3;
        Serial.print("update posId: ");
        Serial.println(sysState.posId);
      }
    }

    xSemaphoreGive(notes.mutex);
    xSemaphoreGive(sysState.mutex);
    xSemaphoreGive(settings.mutex);
  }
}

void CAN_TX_Task (void * pvParameters) {
  Serial.println("CAN_TX_Task started!");
	uint8_t msgOut[8];
	while (1) {
		xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
		xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
    Serial.print("TX: ");
    Serial.print((char) msgOut[0]);
    Serial.print(msgOut[1]);
    Serial.print(msgOut[2]);
    Serial.println();
		CAN_TX(ID, msgOut);
    
	}
}

void fillmsgQ(){
  uint8_t RX_Message[8] = {'P', 4, 0, 0, 0, 0, 0, 0}; 
  for (int a = 0; a < 60; a++){
    xQueueSend(msgOutQ, RX_Message, portMAX_DELAY);
  }
}

void time_CAN_TX_Task () {
  // Serial.println("CAN_TX_Task started!");
	uint8_t msgOut[8];
  // Generic message

  for (int i=0;i<60;i++){
      xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
      // CAN_TX(ID, msgOut);
  }
  
		
    
	// }
}


void backCalcTime(){
    setWorstcaseBackCalc();
    uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		 backgroundCalcTask(NULL);
     
	}
  Serial.println(micros()-startTime);
}
void displayTime(){
    setWorstcaseDisplay();
    uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		 displayUpdateTask(NULL);
     Serial.println(micros()-startTime);
	}
}

void joystickTime(){
  //joystick has no worst case because it is a simple scan of joystick values
      uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		 scanJoystickTask(NULL);
     Serial.println(micros()-startTime);
	}

}
void canTXtime(){
  fillmsgQ();
  uint32_t startTime = micros();

	for (int iter = 0; iter < 32; iter++) {
		 time_CAN_TX_Task();
     fillmsgQ();
	}
  Serial.println(micros()-startTime);
  
}
void scankeyTime(){
  setWorstCaseScankey();
      uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		 scanKeysTask(NULL);
    //  Serial.println(micros()-startTime);
	}
  Serial.println(micros()-startTime);
}
void decodeTime(){
  // setWorstCaseScankey();
      uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		 decodeTask(NULL);
    //  Serial.println(micros()-startTime);
	}
  Serial.println(micros()-startTime);
}
void testSetup(){

  sysState.knobValues[2].current_knob_value = 4;
  sysState.knobValues[3].current_knob_value = 6;
  Serial.begin(9600);
  Serial.println("Serial port initialised");
  generatePhaseLUT();
  set_pin_directions();
  set_notes();
  init_settings();
  CAN_Init(true);
  setCANFilter(0x123,0x7ff);
  CAN_RegisterRX_ISR(CAN_RX_ISR);
  CAN_RegisterTX_ISR(CAN_TX_ISR);
  CAN_Start();
  //initial_display();
  // joystickTime();
  // displayTime();
  // scankeyTime();
  // backCalcTime();
  canTXtime();
  // decodeTime();
  while(1){

  }
}

void setup() {
  // testSetup();
  sysState.knobValues[2].current_knob_value = 4;
  sysState.knobValues[3].current_knob_value = 6;
  //Set pin directions
  generatePhaseLUT();
  set_pin_directions();
  set_notes();
  init_settings();
  
  initial_display();
  initEffects();

  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *sampleTimer = new HardwareTimer(Instance);

  //Initialise sample timer
  sampleTimer->setOverflow(22000, HERTZ_FORMAT);
  sampleTimer->attachInterrupt(sampleISR);
  sampleTimer->resume();

  //Initialise CAN TX semaphore
  CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);
  sampleBufferSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(sampleBufferSemaphore);

  //Initialise CAN Bus
  CAN_Init(false);
  setCANFilter(0x123,0x7ff);
  CAN_RegisterRX_ISR(CAN_RX_ISR);
  CAN_RegisterTX_ISR(CAN_TX_ISR);
  CAN_Start();

  //Initialise serial port
  Serial.begin(9600);
  Serial.println("Serial port initialised");
  
  sysState.posId = auto_detect_init();
  delay(200);
  initial_display();
  Serial.print("posId: ");
  Serial.println(sysState.posId);
  sysState.knobValues[2].current_knob_value = sysState.posId + 3;
  Serial.print("UID: ");
  Serial.println(sysState.local_boardId);

  //Create tasks
  xTaskCreate(
  backgroundCalcTask,		/* Function that implements the task */
  "BackCalc",		/* Text name for the task */
  256 ,      		/* Stack size in words, not bytes */
  NULL,			/* Parameter passed into the task */
  4,			/* Task priority */
  &BackCalc_Handle );	/* Pointer to store the task handle */

  xTaskCreate(
  scanKeysTask,		/* Function that implements the task */
  "scanKeys",		/* Text name for the task */
  256,      		/* Stack size in words, not bytes */
  NULL,			/* Parameter passed into the task */
  6,			/* Task priority */
  &scanKeysHandle );	/* Pointer to store the task handle */

  xTaskCreate(
  displayUpdateTask,		/* Function that implements the task */
  "displayUpdate",		/* Text name for the task */
  256 ,      		/* Stack size in words, not bytes */
  NULL,			/* Parameter passed into the task */
  2,			/* Task priority */
  &displayUpdateHandle );	/* Pointer to store the task handle */

  xTaskCreate(
  decodeTask,		/* Function that implements the task */
  "decode",		/* Text name for the task */
  256 ,      		/* Stack size in words, not bytes */
  NULL,			/* Parameter passed into the task */
  5,			/* Task priority */
  &decodeTaskHandle );	/* Pointer to store the task handle */

  xTaskCreate(
  scanJoystickTask,		/* Function that implements the task */
  "scanJoystick",		/* Text name for the task */
  256 ,      		/* Stack size in words, not bytes */
  NULL,			/* Parameter passed into the task */
  1,			/* Task priority */
  &scanJoystick_Handle );	/* Pointer to store the task handle */

  xTaskCreate(
  CAN_TX_Task,		/* Function that implements the task */
  "CAN_TX",		/* Text name for the task */
  256 ,      		/* Stack size in words, not bytes */
  NULL,			/* Parameter passed into the task */
  3,			/* Task priority */
  &CAN_TX_Handle );	/* Pointer to store the task handle */

  notes.mutex = xSemaphoreCreateMutex();
  sysState.mutex = xSemaphoreCreateMutex(); //Create mutex
  settings.mutex= xSemaphoreCreateMutex();
  vTaskStartScheduler();



}

void loop() {
}