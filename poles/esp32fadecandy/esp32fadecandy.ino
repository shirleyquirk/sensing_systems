#define HOSTNAME "Fadecandy_3"
#define OTA_PASS "Pangolin303"

#define LOG_DEST "Weichwuermer"
//#define LOG_DEST "192.168.1.9"
//undefine LOG_DEST for broadcast address
#define LOG_PORT 5555

#define AP_SSID HOSTNAME"_AP"
#define AP_PASS "Pangolin303"

#include "common.h"

//////////////////////////////
//   Sketch Specific Below  //
//////////////////////////////

#define FASTLED_ESP32_I2S true
//#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>


#define PIN1 23
#define PIN2 22
#define N_PIXELS 200
#define OPC_PORT 7890
WiFiServer opc_server(OPC_PORT);

CRGB leds[N_PIXELS];
typedef struct frame_t{
  CRGB leds[N_PIXELS];
  TickType_t timestamp;
  boolean full;
  frame_t *next_frame;
  frame_t *prev_frame;
}frame_t;

#define N_BUFFERS 30
frame_t buffers[N_BUFFERS];
frame_t *fbPrev, *fbNext, *fbNew;

SemaphoreHandle_t frames_mutex;
int perfFrameCounter=0;
int perfPacketCounter=0;


void setup() {
  Serial.begin(115200);
  setup_common();
  //announce fadecandy service if that ever helps
  //log_printf("fyi, portTICK_PERIOD_MS=%i\n",portTICK_PERIOD_MS);
  //////////FASTLED//////////////
  LEDS.addLeds<WS2812,PIN1,GRB>(leds,0,N_PIXELS/2);
  LEDS.addLeds<WS2812,PIN2,GRB>(leds,N_PIXELS/2,N_PIXELS/2);
  LEDS.setBrightness(128);
  //FastLED.setDither(0);
  FastLED.show();

  prefs.begin("fadecandy",false);
  smoothing=prefs.getFloat("smoothing",smoothing);

  /////////BUFFERS////////////////
  fbNew = &(buffers[2]);
  fbPrev = &(buffers[0]);
  fbNext = &(buffers[1]);
  for (int i=0;i<N_BUFFERS;i++){
    buffers[i].full=false;
    buffers[i].next_frame = &(buffers[(i+1)%N_BUFFERS]);
    buffers[i].prev_frame = &(buffers[(i-1+N_BUFFERS)%N_BUFFERS]);
  }

  //////////OPC SERVER///////////
  opc_server.begin();
//    xTaskCreate(
//                    opc_server_task,          /* Task function. */
//                    "OPC Server",        /* String with name of task. */
//                    10000,            /* Stack size in bytes. */
//                    NULL,             /* Parameter passed as input of the task */
//                    3,                /* Priority of the task. */
//                    NULL);            /* Task handle. */

                   
}
float average_period;

float delay_millis = 0;
TickType_t last_pixel_timestamp=0;
uint8_t calculateInterpCoefficient(){
  //TickType_t last_period = fbNext->timestamp - fbPrev->timestamp;
  TickType_t last_period = (TickType_t)average_period;
  if (last_period <=0){
    return 0xff;
  }
  TickType_t delta = xTaskGetTickCount() - last_pixel_timestamp;//last_pixel_timestamp;//fbNext->timestamp;
  //aim to get fbPrev->timestamp-last_pixel_timestamp == fbNext->next_frame->timestamp - fbPrev->timestamp
  
  uint8_t coef = (delta >= last_period) ? 0xff : (delta << 8) / last_period;
  return coef;
}

uint8_t interp_coeff;
unsigned int draw_buffer_full=0;
unsigned int frame_delay;
void fastled_show_task(void * parameter){
  TickType_t lastWakeTime;
  const TickType_t taskPeriod = 20;
  lastWakeTime = xTaskGetTickCount();
  for (;;){
    vTaskDelayUntil(&lastWakeTime,taskPeriod);
    //interpolate
    //uint8_t coef;
    interp_coeff = calculateInterpCoefficient();
    if (interp_coeff == 0xff){
      if (fbNext->next_frame->full){
        /*frame_t *recycle=fbPrev;
        fbPrev = fbNext;
        fbNext = fbNew;
        fbNew = recycle;
        */
        fbPrev = fbNext;
        frame_delay--;
        fbNext = fbNext->next_frame;
        fbPrev->prev_frame->full=false;
        last_pixel_timestamp = lastWakeTime;
        interp_coeff=0;
      }else{
        draw_buffer_full++;//we should probably slow down then 
      }
    }
    blend(fbPrev->leds,fbNext->leds,leds,N_PIXELS,interp_coeff);
    FastLED.show();//no point in this if leds hasn't been updated at all
    perfFrameCounter++;
  }
}

void opc_server_task(void * parameter){
  const TickType_t xDelay = 100 / portTICK_PERIOD_MS;
  for(;;){
    WiFiClient client = opc_server.available();
    if (client) {
      log_println("New OPC Client Connected: ");
      while (client.connected()){
        readFrame(client,fbNew);
        yield();
        //vTaskDelayUntil(xTimestamp,xPeriod);
      }
      client.stop();
    }
    vTaskDelay(xDelay);
  }  
}

int readFully (WiFiClient &client, uint8_t *buf, size_t len) {
  size_t i;

  for (i = 0; i < len; i++) {
    int b;

    if ((b = blockingRead(client)) < 0)
      return -1;

    buf[i] = (uint8_t) b;
  }

  return 0;
}

int blockingRead (WiFiClient &client) {
  int b;
  while ((b = client.read()) < 0) {
    yield();
    if (! client.connected())
      return -1;
  }
  return b;
}

unsigned int frame_buffer_full=0;
int readFrame(WiFiClient &client,frame_t *fb) {
  if (fb->full) {
    frame_buffer_full++;
    return -1;
  }
  uint8_t buf4[4];
  uint8_t cmd;
  size_t payload_length, leds_in_payload, i;

  // read channel number (ignored), command, and length
  if (readFully (client, buf4, sizeof (buf4)) < 0)
    return -2;

  cmd = buf4[1];
  payload_length = (((size_t) buf4[2]) << 8) + (size_t) buf4[3];
  leds_in_payload = payload_length / 3;
  if (leds_in_payload > N_PIXELS)
    leds_in_payload = N_PIXELS;
  if (cmd != 0)                 // we only support command 0, set pixel colors
    leds_in_payload = 0;

  // read pixel data; 3 bytes per pixel
  for (i = 0; i < leds_in_payload; i++) {
    if (readFully (client, buf4, 3) < 0)
      return -3;
    fb->leds[i] = CRGB(buf4[0], buf4[1], buf4[2]);
  }
 //FastLED.show();
  fb->full = true;
  fb->timestamp = xTaskGetTickCount();
  perfPacketCounter++;
  // discard any remaining data (e. g. if they sent us more pixels than we have)
  payload_length -= leds_in_payload * 3;

  for (; payload_length != 0; payload_length--) {
    if (blockingRead(client) < 0)
      return 1;
  }
  return 0;
}






void loop() {
  // put your main code here, to run repeatedly:
  //FastLED.delay(10);
    vTaskDelay(100);
    TickType_t xTimestamp;
    WiFiClient client = opc_server.available();
    if (client) {
      log_println("New OPC Client Connected: ");
      TickType_t xTimestamp = xTaskGetTickCount();
      TaskHandle_t xFastLED_show_handle;
      //fill buffer
      //reset buffers
      for (int i=0;i<N_BUFFERS;i++){
        buffers[i].full=0;
        buffers[i].timestamp = 0;
      }
      int res=-1;
      while(res<0){
      res = readFrame(client,fbPrev);
      }
      res = -1;
      while(res<0){
      res = readFrame(client,fbNext);
      }
      average_period = fbNext->timestamp - fbPrev->timestamp;
      frame_delay=1;
      last_pixel_timestamp=0;
      delay_millis=0;
      xTaskCreate(
                fastled_show_task,          /* Task function. */
                "FastLED.show()",        /* String with name of task. */
                10000,             /* Stack size in bytes.words? */ 
                NULL,             /* Parameter passed as input of the task */
                1,                /* Priority of the task. */
                &xFastLED_show_handle);            /* Task handle. */  
      while (client.connected()){
          int res = readFrame(client,fbNew);
          if (res >= 0) {//only if we've just filled it.
            //average_period = average_period * smoothing + (fbNew->timestamp - fbNew->prev_frame->timestamp)*(1-smoothing);
            //delay_millis = delay_millis * smoothing + (float)(last_pixel_timestamp - fbNext->next_frame->timestamp)*(1-smoothing);
            if (last_pixel_timestamp >0 && fbNext->next_frame->timestamp >0) {
              delay_millis = ((float)last_pixel_timestamp - (float)fbNext->next_frame->timestamp - 50);
              static boolean first_time = true;
              if (first_time){
                log_printf("first delay_millis:%f,last_pixel_timestamp:%i,fbNext->next->timestamp:%i",delay_millis,last_pixel_timestamp,fbNext->next_frame->timestamp);
                first_time = false;
              }
            }else{
              delay_millis = 0;
            }

            average_period = ((float)(fbNew->timestamp - fbPrev->timestamp) - delay_millis*smoothing ) / (float)(++frame_delay);
            fbNew = fbNew->next_frame;
            
          }
          
        //FastLED.delay(20);
        yield();
        if (perfPacketCounter>30) log_performance();
        vTaskDelay(5);
      }
      client.stop();
      vTaskDelete(xFastLED_show_handle);
      log_println("OPC Client Disconnected");
    }

}

TickType_t prev_update=0;
float draw_buffer_underflows_per_tick=0;
void log_performance(){
  TickType_t this_update = xTaskGetTickCount();
  
  float frameRate = (float)perfFrameCounter / (float)(this_update-prev_update);
  perfFrameCounter=0;
  log_printf("frameRate: %f",frameRate*1000);

  frameRate = (float) perfPacketCounter / (float)(this_update-prev_update);
  perfPacketCounter=0;
  log_printf("packetRate: %f\n",frameRate*1000);
  log_printf("period: %f\n",1.0/frameRate);
  log_printf("average period: %f\n",average_period);
  log_printf("draw_buffer underflows: %i ",draw_buffer_full);
  if (draw_buffer_full==0){
    //overflow_compensation--;
    //if (overflow_compensation<0) overflow_compensation=0;
  }
  draw_buffer_full=0;
  log_printf("frame_buffer overflows: %i\n",frame_buffer_full);
  frame_buffer_full=0;
  log_printf("delay_millis: %f\n",delay_millis);
  
  prev_update=this_update;
}
