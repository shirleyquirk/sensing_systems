#include <FastLED.h>
#define PANEL0LEDPIN 25
#define PANEL1LEDPIN 14
#define PANEL2LEDPIN 2
#define PANEL3LEDPIN 13

int encoder_width = 50;



void show_encoder(encoder_t *enc,panel_t *pan){
  if (pan->enabled){
    switch (enc->n_gradient_colours){
      case 1:
        fill_solid(&(pan->leds[enc->led_offset]),LEDS_PER_ENC,enc->gradient_colours[0]);
        break;
      case 2:
        fill_gradient_RGB(&(pan->leds[enc->led_offset]),LEDS_PER_ENC,enc->gradient_colours[0],enc->gradient_colours[1]);
        break;
      case 3:
        fill_gradient_RGB(&(pan->leds[enc->led_offset]),LEDS_PER_ENC,enc->gradient_colours[0],enc->gradient_colours[1],enc->gradient_colours[2]);
    }
  }else{
    fill_solid(&(pan->leds[enc->led_offset]),LEDS_PER_ENC,CRGB(0));
  }
  for (int i=0;i<LEDS_PER_ENC;i++){//TODO:integer math
    float ledx = (float)(i*enc->max_val)/21.0;//(float)enc->min_val + (float)(i*(enc->max_val-enc->min_val))/21.0;
    float rampx = (float)(enc->max_val+encoder_width)/(float)(enc->max_val)*(float)(enc->val);//(float)enc->min_val + (float)((enc->val-enc->min_val)*(enc->max_val + encoder_width - enc->min_val))/(float)(enc->max_val-enc->min_val);
    float ledy = (rampx - ledx) / (float)encoder_width;
    if (ledy<0.0) ledy=0.0;
    if (ledy>1.0) ledy=1.0;
    uint8_t fadebyfract = (uint8_t)(ledy*255.0);
    pan->leds[enc->led_offset+i].fadeLightBy(fadebyfract);//dim8_video(fadebyfract);
  }
}


CRGB panel0leds[3*LEDS_PER_ENC+4+1];//added one so maybe we don't crash
CRGB panel1leds[3*LEDS_PER_ENC+4+1];
CRGB panel2leds[3*LEDS_PER_ENC+4+1];
CRGB panel3leds[LEDS_PER_ENC+4+1];
void ledsetup(){
  LEDS.addLeds<WS2812,PANEL0LEDPIN,GRB>(panel0leds,0,3*LEDS_PER_ENC+4);
  LEDS.addLeds<WS2812,PANEL1LEDPIN,GRB>(panel1leds,0,3*LEDS_PER_ENC+4);
  LEDS.addLeds<WS2812,PANEL2LEDPIN,GRB>(panel2leds,0,3*LEDS_PER_ENC+4);
  LEDS.addLeds<WS2812,PANEL3LEDPIN,GRB>(panel3leds,0,LEDS_PER_ENC+4);
  panels[0].leds = &panel0leds[0];
  panels[1].leds = &panel1leds[0];
  panels[2].leds = &panel2leds[0];
  panels[3].leds = &panel3leds[0];
  LEDS.setBrightness(128);
}
void led_loop(void *parameters){
  

  for(;;){
    //led arrays should be directly updated by each encoder/button when changed. fine do it here
    for(int i=0;i<N_PANELS;i++){
      panel_t *pan = &(panels[i]);
        for (int j=0;j<pan->n_encoders;j++){
          encoder_t *enc = &(pan->encoders[j]);
          show_encoder(enc,pan);
        }
        for (int j=0;j<pan->n_buttons;j++){
          button_t *but = &(pan->buttons[j]);
          if (pan->enabled){
            pan->leds[but->led_offset]=but->state_colours[but->state];
          }else{
            pan->leds[but->led_offset]=CRGB(0);
          }
        }
    }
    if (xSemaphoreTake(button_led_sem,1)==pdTRUE){//if button working, wait 1 tick to see if it comes back
      FastLED.show();
      xSemaphoreGive(button_led_sem);
      perfLEDCounter++;
    }else{
      log_println("LED couldn't take semaphore");
    }
    vTaskDelay(60);
  }
  
}
