#define dataPin 17
#define clockPin 5
#define latchPin 18


uint16_t readButtons(){
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
  //ets_delay_us(1);
  digitalWrite(latchPin,LOW);
  uint16_t out=0;
  for(int i=15;i>=0;i--){
    digitalWrite(clockPin,HIGH);
    delayMicroseconds(1);//ets_delay_us(1);//
    out |= digitalRead(dataPin)<<i;
    digitalWrite(clockPin,LOW);
    delayMicroseconds(1);//ets_delay_us(1);//
  }
  return out;
}

uint16_t prev_button;
void button_loop(void * parameters){
  pinMode(dataPin,INPUT_PULLDOWN);
  pinMode(clockPin,OUTPUT);
  pinMode(latchPin,OUTPUT);
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
