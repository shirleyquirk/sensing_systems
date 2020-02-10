#include <FastLED.h>


void led_loop(void *parameters){
  for(int i=0;i<N_PANELS;i++){
    panel_t *pan = &(panels[i]);
    int n_leds = pan->n_encoders*LEDS_PER_ENC + pan->n_buttons;
    pan->n_leds=n_leds;
    pan->leds = (CRGB*)malloc(n_leds * sizeof(CRGB));//obvs not necessary but fuckit
    LEDS.addLeds<WS2812,pan->led_pin,GRB>(pan->leds,0,n_leds);
  }
  LEDS.setBrightness(128);
  for(;;){
    //led arrays should be directly updated by each encoder/button when changed. fine do it here
    for(int i=0;i<N_PANELS;i++){
      panel_t *pan = &(panels[i]);
      for (int j=0;j<pan->n_encoders;j++){
        encoder_t *enc = pan->encoders[j];
        show_encoder(enc,pan);
      }
      for (int j=0;j<pan->n_buttons;j++){
        button_t *but = pan->buttons[j];
        pan->leds[but->led_offset]=but->state_colours[but->state];
      }
    }
    FastLED.show();
    vTaskDelay(30);
  }
  
}

void show_encoder(encoder_t *enc,panel_t *pan){
  switch (enc->n_gradient_colours){
    case 1:
      fill_solid(&(pan->leds[enc->led_offset]),LEDS_PER_ENC,enc->gradient_colours[0]);
      break;
    case 2:
      fill_gradient_RGB(&(pan->leds[enc->led_offset]),LEDS_PER_ENC,enc->gradient_colours[0],enc->gradient_colours[1]);
      break;
    case 3:
      fill_gradient_RGB(&(pan->le
  //fadebyfract = (enc->val*255 - i*(enc->max_val-enc->min_val)*255/22) / width;
  led[i] = dim8_raw(fadebyfract);//dim8_video(fadebyfract);
}
