#define dataPin 17
#define clockPin 5
#define latchPin 18


uint16_t readButtons(){
  //pause the led loop it's fucking with us
  //vTaskSuspend(led_handle); vTaskSuspend doesn't work
  if (xSemaphoreTake(button_led_sem,3)==pdTRUE){//if led currently 'show'ing, wait 1 tick and try again
    digitalWrite(clockPin,LOW);
    digitalWrite(latchPin,HIGH);
    //wait for data to collect
    //digitalWrite(clockPin,HIGH);
    //delayMicroseconds(20);
    //or clock pin high transition, right?
    //digitalWrite(clockPin,HIGH);
    //digitalWrite(clockPin,LOW);
    //set to serial output
    delayMicroseconds(1);
    //ets_delay_us(2);
    digitalWrite(latchPin,LOW);
    //led can go on now
    //vTaskResume(led_handle);
    xSemaphoreGive(button_led_sem);
  }else{//couldn't get semaphore
    if (button_led_sem==NULL){
      log_println("button_led semaphore is NULL!");
    }else{
      log_println("Button Couldn't take semaphore!");
    }
    return 0;
  }
  delayMicroseconds(1);
  uint16_t out=0;
  for(int i=15;i>=0;i--){
    out |= digitalRead(dataPin)<<i;
    digitalWrite(clockPin,HIGH);

    //gpio_set_level(GPIO_NUM_5,HIGH);
    //ets_delay_us(2);
    delayMicroseconds(1);

    digitalWrite(clockPin,LOW);
    //gpio_set_level(GPIO_NUM_5,LOW);
    delayMicroseconds(1);
    //ets_delay_us(2);

  }
  return out;
}

void button_loop(void * parameters){
  pinMode(dataPin,INPUT_PULLDOWN);
  pinMode(clockPin,OUTPUT);
  pinMode(latchPin,OUTPUT);
  uint16_t prev_button;
  for(;;){
    uint16_t button = readButtons();//this can now fail. 0 would be pretty unlikely lets say that's an error
    if (button==0) {vTaskDelay(15);continue;}//try again
    if (button != prev_button){
      log_println(button,BIN);
      for (int i=0;i<N_PANELS;i++){
        panel_t *pan = &panels[i];
        uint16_t panel_buttons = (button >> pan->button_offset)&0b1111;
        uint16_t prev_buttons = (prev_button>>pan->button_offset)&0b1111;
        byte button_changed = panel_buttons ^ prev_buttons;
        if (pan->enabled){

          for (int j=0;j<4;j++){
            button_t *but = &(pan->buttons[j]);
            if (button_changed & (1<<j)){//pan->buttons[i] has changed
              //either gone from 0-1 or 1-0
              if (panel_buttons & (1<<j)){//gone from 0 to 1, i.e. depressed to released
                //maybe send noteoff
                if (but->momentary){
                  but->state = 0;
                  //but->updated = true;
                }
         
                log_printf("panel:%i button:%i noteoff\n",i,j);
              }else{
                //noteon
                if (but->momentary){
                  but->state = 1;
                }else{
                  //HAAAAAACK
                  if (i == 3){
                    //top panel set other buttons states to 0
                    for (int k=1;k<4;k++){
                      panels[3].buttons[k].state = 0;
                    }
                    but->state = (1);
                    
                  }else{
                    but->state = (but->state+1) % but->n_states;
                  }
                }
                but->updated=true;
                log_printf("panel:%i button:%i noteon state:%i\n",i,j,but->state);
              }
            }
          }
        }else{//panel disabled but still want something
          if (i==3){//top panel
            if (button_changed & (1<<0)){//encoder button
              if (panel_buttons & (1<<0)){
                log_println("top panel encoder button noteoff");
              }else{
                log_println("top panel encoder button noteon");
                panels[3].buttons[0].updated=true;
              }
            }
          }
        }
      }
      prev_button=button;
    } 
    perfButtonCounter++;
    vTaskDelay(100);
  }
}
