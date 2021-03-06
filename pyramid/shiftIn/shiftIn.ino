
#define HOSTNAME "Pyramid"
#define OTA_PASS "Pangolin303"

const IPAddress maxwell(192,168,8,255);
#define LOG_DEST maxwell
//"Weichwuermer"
//#define LOG_DEST "192.168.1.9"
//undefine LOG_DEST for broadcast address
#define LOG_PORT 5551

#define AP_SSID HOSTNAME"_AP"
#define AP_PASS "Pangolin303"



#define N_PANELS 4
#include "common.h"

SemaphoreHandle_t button_led_sem=NULL;


unsigned int perfButtonCounter=0;
unsigned int perfEncoderCounter=0;
unsigned int perfOSCReadCounter=0;
unsigned int perfOSCWriteCounter=0;
unsigned int perfLEDCounter=0;

TaskHandle_t button_handle;
TaskHandle_t osc_send_handle;
TaskHandle_t osc_read_handle;
TaskHandle_t encoder_handle;
TaskHandle_t led_handle;
void led_loop(void* parameters);
void perf_loop(void *parameters){
  TickType_t last_time = xTaskGetTickCount();
  for(;;){
    TickType_t now = xTaskGetTickCount();
    log_printf("button reads:%d encoder reads:%d osc reads:%d osc writes:%d ota_handler:%d led:%d millis:%d\n",perfButtonCounter,perfEncoderCounter,perfOSCReadCounter,perfOSCWriteCounter,perfArduinoOTACounter,perfLEDCounter,now-last_time);
    log_printf("stack: button:%d encoder:%d osc read:%d osc send:%d ota:%d led:%d perf:%d\n",
                    uxTaskGetStackHighWaterMark(button_handle),
                    uxTaskGetStackHighWaterMark(encoder_handle),
                    uxTaskGetStackHighWaterMark(osc_read_handle),
                    uxTaskGetStackHighWaterMark(osc_send_handle),
                    uxTaskGetStackHighWaterMark(ota_handle),
                    uxTaskGetStackHighWaterMark(led_handle),
                    uxTaskGetStackHighWaterMark(NULL));
    last_time=now;
    perfButtonCounter=0;
    perfEncoderCounter=0;
    perfOSCReadCounter=0;
    perfOSCWriteCounter=0;
    perfArduinoOTACounter=0;
    if (perfLEDCounter==0){
      //led loop may have crashed
      //hack it back alive
      log_printf("RESARTING LED TASK\n");
      vTaskDelete(led_handle);
          xTaskCreate(
                    led_loop,          /* Task function. */
                    "Led Loop",        /* String with name of task. */
                    2000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    &led_handle);            /* Task handle. */ 
    }
    perfLEDCounter=0;
    vTaskDelay(10000);
  }
}


#include "ESP32Encoder.h"
#define panel_a_en_pin GPIO_NUM_27
#define panel_b_en_pin GPIO_NUM_26
#define panel_c_en_pin GPIO_NUM_15
#define FASTLED_ESP32_I2S true
#include <FastLED.h>
#define LEDS_PER_ENC 22
#define OSC_ADDR_BUFSIZE 64
#define MAX_ENCODER_COLOURS 4

typedef struct encoder_t{
  int val;
  int max_val;
  int min_val;
  pcnt_unit_t unit;
  char osc_addr[OSC_ADDR_BUFSIZE];
  boolean updated;
  int led_offset;
  int n_gradient_colours;//1-4
  CRGB gradient_colours[MAX_ENCODER_COLOURS];  ///save across reboots              4 words
}encoder_t;

#define MAX_BUTTON_COLORS 32
typedef struct button_t{
  uint8_t n_states;                       //save across reboots                   1 byte
  uint8_t state;
  int led_offset;
  CRGB state_colours[MAX_BUTTON_COLORS]; //want to save this across reboots        32 *3 bytes
  char osc_addr[OSC_ADDR_BUFSIZE];
  boolean updated;
  boolean momentary;
}button_t;

typedef struct panel_t{
  boolean enabled;
  boolean has_en_pin;
  gpio_num_t enable_pin;
  uint8_t n_encoders;
  encoder_t *encoders;
  uint8_t n_buttons;
  button_t *buttons;
  CRGB *leds;
  int n_leds;
  const char * osc_addr;
  int button_offset;
}panel_t;

panel_t panels[N_PANELS];


#include "leds.h"
//#include "encoders.h"
#include "buttons.h"

#include "osc.h"


#define encoder_1a_pin 22
#define encoder_1b_pin 23
#define encoder_2a_pin 19
#define encoder_2b_pin 21
#define encoder_3a_pin 4
#define encoder_3b_pin 16
#define encoder_4a_pin 32
#define encoder_4b_pin 33
ESP32Encoder encoder[4];



void encoder_loop(void * parameters){
    // Enable the weak pull down resistors
  ESP32Encoder::useInternalWeakPullResistors=true;
  //set up enable pins
  gpio_pad_select_gpio(panel_a_en_pin);
  gpio_set_direction(panel_a_en_pin,GPIO_MODE_OUTPUT);
  gpio_set_level(panel_a_en_pin,HIGH);  
  
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

  for(int i=0;i<3;i++){
    panel_t *pan = &panels[i];
    for (int j=0;j<3;j++){
      pan->encoders[j].unit = encoder[j].unit;
    }
  }
  panels[3].encoders[0].unit=encoder[3].unit;
  for(;;){
    for(int i=0;i<N_PANELS;i++){
      panel_t pan = panels[i];
      if (pan.enabled){
        for(int j=0;j<pan.n_encoders;j++){
          int16_t c;
          encoder_t *enc = &(pan.encoders[j]);
          pcnt_get_counter_value(enc->unit,&c);
          if(c!=0){
            pcnt_counter_clear(enc->unit);
            enc->val += c;
            if (enc->val > enc->max_val) enc->val = enc->max_val;
            if (enc->val < enc->min_val) enc->val = enc->min_val;
            log_printf("panel:%i encoder:%i val:%i\n",i,j,enc->val);
            enc->updated=true;
          }
        }
      }else{//read top encoder anyway
        if (i==3){//top
          int16_t c;
          encoder_t *enc = &(pan.encoders[0]);
          pcnt_get_counter_value(enc->unit,&c);
          if(c!=0){
            pcnt_counter_clear(enc->unit);
            enc->val += c;
            if (enc->val > enc->max_val) enc->val = enc->max_val;
            if (enc->val < enc->min_val) enc->val = enc->min_val;
            log_printf("top panel encoder val:%i\n",enc->val);
            enc->updated=true;
          }
        }
      }
    }
    perfEncoderCounter++;
    vTaskDelay(100);
  }
}

#define ENC_MIN_VAL 0

#define ENC_MAX_VAL 255
void setup() {
  setup_common();

  vSemaphoreCreateBinary(button_led_sem);

  //setup leds
  ledsetup();
  //setup panels
  panels[0].osc_addr = "/moss";
  panels[0].enable_pin = panel_a_en_pin;
  panels[1].osc_addr = "/earth";
  panels[1].enable_pin = panel_b_en_pin;
  panels[2].osc_addr = "/origin";
  panels[2].enable_pin = panel_c_en_pin;
  panels[3].osc_addr = "/top";
  panels[3].enable_pin = (gpio_num_t)255;
  for (int i=0;i<3;i++){
    panel_t *pan = &(panels[i]);
    pan->enabled = false;
    pan->has_en_pin = true;
    pan->n_encoders = 3;
    pan->encoders = (encoder_t *)malloc(3*sizeof(encoder_t));
    for(int j=0;j<3;j++){
      encoder_t *enc = &(pan->encoders[j]);
      enc->min_val = ENC_MIN_VAL;
      enc->max_val = ENC_MAX_VAL;
      enc->val = (ENC_MAX_VAL + ENC_MIN_VAL)/2;
      snprintf(enc->osc_addr,OSC_ADDR_BUFSIZE,"%s/encoder/%i",pan->osc_addr,j+1);//oh. didn't malloc
      enc->updated = false;
      enc->led_offset = j*LEDS_PER_ENC;
      enc->n_gradient_colours = 1;
      enc->gradient_colours[0] = CRGB(0x0000ff);
    }
    pan->n_buttons = 4;
    pan->button_offset=4*i;
    pan->buttons = (button_t *)malloc(4*sizeof(button_t));
    for (int j=0;j<4;j++){
      button_t *but = &(pan->buttons[j]);
      but->n_states = 2;
      but->state = 0;
      but->momentary=false;
      but->led_offset = 3*LEDS_PER_ENC+j;
      but->state_colours[0]=CRGB(0x004444);
      but->state_colours[1]=CRGB(0x444400);
      snprintf(but->osc_addr,OSC_ADDR_BUFSIZE,"%s/button/%i",pan->osc_addr,j+1);
      but->updated=false;
    }
    
  }

  
  panel_t *top = &panels[3];
  top->enabled=false;
  top->has_en_pin=false;
  top->n_encoders=1;
  top->encoders =(encoder_t *)malloc(1*sizeof(encoder_t));
  encoder_t *enc = top->encoders;
  enc->min_val = ENC_MIN_VAL;
  enc->max_val = ENC_MAX_VAL;
  enc->val = (ENC_MIN_VAL+ENC_MAX_VAL)/2;
  sprintf(enc->osc_addr,"/top/encoder/1");
  enc->updated = false;
  enc->led_offset = 0;
  enc->n_gradient_colours = 1;
  enc->gradient_colours[0] = CRGB(0x0000ff);
  top->n_buttons = 4;
  top->button_offset=4*3;
  top->buttons = (button_t *)malloc(4*sizeof(button_t));
  button_t *topbut = &(top->buttons[0]);
  topbut->n_states=1;
  topbut->state=0;
  topbut->led_offset = LEDS_PER_ENC+3;
  topbut->state_colours[0]=CRGB(0x440044);
  topbut->state_colours[1]=CRGB(0x044440);
  topbut->updated=false;
  for (int j=1;j<4;j++){
    button_t *but = &(top->buttons[j]);
    but->n_states=2;
    but->momentary = false;
    but->state=0;
    but->led_offset= LEDS_PER_ENC+j-1;
    but->state_colours[0]=CRGB(0x004444);
    but->state_colours[1]=CRGB(0x444400);
    snprintf(but->osc_addr,OSC_ADDR_BUFSIZE,"%s/button/%i",top->osc_addr,j+1);
    but->updated=false;
  }


  
//  Serial.begin(115200); can't serial if slip

    xTaskCreate(
                    button_loop,          /* Task function. */
                    "Button Loop",        /* String with name of task. */
                    2000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    2,                /* Priority of the task. */
                    &button_handle);            /* Task handle. */

    xTaskCreate(
                    encoder_loop,          /* Task function. */
                    "Encoder Loop",        /* String with name of task. */
                    2000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    2,                /* Priority of the task. */
                    &encoder_handle);            /* Task handle. */
    xTaskCreatePinnedToCore(
                    osc_read_loop,          /* Task function. */
                    "OSC Read Loop",        /* String with name of task. */
                    4000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    2,                /* Priority of the task. */
                    &osc_read_handle,1);            /* Task handle. */
    xTaskCreate(
                    osc_send_loop,          /* Task function. */
                    "OSC Send Loop",        /* String with name of task. */
                    4000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    2,                /* Priority of the task. */
                    &osc_send_handle);            /* Task handle. */
//    xTaskCreate(
//                    perf_loop,          /* Task function. */
//                    "Perf Monitor Loop",        /* String with name of task. */
//                    2000,            /* Stack size in bytes. */
//                    NULL,             /* Parameter passed as input of the task */
//                    1,                /* Priority of the task. */
//                    NULL);            /* Task handle. */   
    xTaskCreate(
                    led_loop,          /* Task function. */
                    "Led Loop",        /* String with name of task. */
                    2000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    &led_handle);            /* Task handle. */ 
    //enable_panel(&panels[0]);                    
    log_println("End Of Setup");     
}

void loop() {
  //delay(100);
  yield();
  vTaskDelay(1000);
  //delay(1000);

}
