#define dataPin 17
#define clockPin 5
#define latchPin 18


uint16_t readButtons(){
  //pause the led loop it's fucking with us
  vTaskSuspend(led_handle);
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
  vTaskResume(led_handle);
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
    uint16_t button = readButtons();
    if (button != prev_button){
      log_println(button,BIN);
      for (int i=0;i<N_PANELS;i++){
        panel_t *pan = &panels[i];
        if (pan->enabled){
          uint16_t panel_buttons = (button >> pan->button_offset)&0b1111;
          uint16_t prev_buttons = (prev_button>>pan->button_offset)&0b1111;
          byte button_changed = panel_buttons ^ prev_buttons;
          for (int j=0;j<4;j++){
            button_t *but = &(pan->buttons[j]);
            if (button_changed & (1<<j)){//pan->buttons[i] has changed
              //either gone from 0-1 or 1-0
              if (panel_buttons & (1<<j)){//gone from 0 to 1, i.e. depressed to released
                //maybe send noteoff
                log_printf("panel:%i button:%i noteoff\n",i,j);
              }else{
                //noteon
                but->state = (but->state+1) % but->n_states;
                but->updated=true;
                log_printf("panel:%i button:%i noteon state:%i\n",i,j,but->state);
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
