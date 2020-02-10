#define ENCODERS_H

#include "ESP32Encoder.h"
#include <driver/gpio.h>
#define panel_a_en_pin GPIO_NUM_12
#define panel_b_en_pin GPIO_NUM_26
#define panel_c_en_pin GPIO_NUM_15

#define encoder_1a_pin 22
#define encoder_1b_pin 23

#define encoder_2a_pin 19
#define encoder_2b_pin 21

#define encoder_3a_pin 4
#define encoder_3b_pin 16

#define encoder_4a_pin 32
#define encoder_4b_pin 33


typedef struct encoder_t{
  boolean enabled;
  pcnt_unit_t unit;
  int value;
  int min_val;
  int max_val;
  void (*update)(encoder_t *enc);
  const char* addr;
}encoder_t;

ESP32Encoder encoder[4];
encoder_t virtual_encoders[10];

void encoder_update_callback(encoder_t *enc){
  Serial.printf("/%s/%i\n",enc->addr,enc->value);
}

inline void clamp(encoder_t enc){
  enc.value = (enc.value > enc.max_val) ? enc.max_val : (enc.value < enc.min_val) ? enc.min_val : enc.value;
}

encoder_t new_encoder(boolean enabled,ESP32Encoder *enc,const char* addr){
  encoder_t ret;
  ret.min_val=0;
  ret.max_val=127;
  ret.enabled=enabled;
  ret.unit = enc->unit;
  ret.update = encoder_update_callback;
  ret.value = 0;
  ret.addr = addr;
  return ret;
}
void encoder_loop(void * parameters){
  // Enable the weak pull down resistors
  ESP32Encoder::useInternalWeakPullResistors=true;
  //set up enable pins
  gpio_pad_select_gpio(panel_a_en_pin);
  gpio_set_direction(panel_a_en_pin,GPIO_MODE_OUTPUT);
  gpio_set_level(panel_a_en_pin,LOW);  
  
  gpio_pad_select_gpio(panel_b_en_pin);
  gpio_set_direction(panel_b_en_pin,GPIO_MODE_OUTPUT);
  gpio_set_level(panel_b_en_pin,HIGH);
    
  gpio_pad_select_gpio(panel_c_en_pin);
  gpio_set_direction(panel_c_en_pin,GPIO_MODE_OUTPUT);
  gpio_set_level(panel_c_en_pin,HIGH);

  //set up encoders
  encoder[0].attachHalfQuad(encoder_1a_pin,encoder_1b_pin);
  encoder[1].attachHalfQuad(encoder_2a_pin,encoder_2b_pin);
  encoder[2].attachHalfQuad(encoder_3a_pin,encoder_3b_pin);
  encoder[3].attachHalfQuad(encoder_4a_pin,encoder_4b_pin);

#define enc_min 0
#define enc_max 127
#define N_ENCODERS 10
  virtual_encoders[0] = new_encoder(true,&encoder[0],"A1");
  virtual_encoders[1] = new_encoder(true,&encoder[1],"A2");
  virtual_encoders[2] = new_encoder(true,&encoder[2],"A3");
  virtual_encoders[3] = new_encoder(false,&encoder[0],"B1");
  virtual_encoders[4] = new_encoder(false,&encoder[1],"B2");
  virtual_encoders[5] = new_encoder(false,&encoder[2],"B3");
  virtual_encoders[6] = new_encoder(false,&encoder[0],"C1");
  virtual_encoders[7] = new_encoder(false,&encoder[1],"C2");
  virtual_encoders[8] = new_encoder(false,&encoder[2],"C3");
  virtual_encoders[9] = new_encoder(false,&encoder[3],"D");


  for(;;){
    //so, we need to poll the peripheral count anyway, we take its raw value and update our virtual encoders accordingly
    for (int i=0;i<N_ENCODERS;i++){
      if (virtual_encoders[i].enabled){
        int16_t c;
        pcnt_get_counter_value(virtual_encoders[i].unit,&c);
        if (c!=0){
          pcnt_counter_clear(virtual_encoders[i].unit);
          virtual_encoders[i].value+=c;
          clamp(virtual_encoders[i]);
          virtual_encoders[i].update(&virtual_encoders[i]);
        }
      }
    }
    vTaskDelay(100);
  }
}
