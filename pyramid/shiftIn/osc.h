/*
 * OSC Messaging
 * 
 * input:
 * /enable_panel string panel_name  (top,moss,origin,earth)
 * /disable_panel string panel_name
 *  
 * 
 * /set/{panel}/{button,encoder}/{all,1,2,3,4}/value int32 (step number or 
 *                                            /max_value
 *                                            /min_value   (max>min, for buttons if max==min then momentary. i guess send noteoffs
 * output:
 * /{top,moss,origin,earth}/{button,encoder}/{1,2,3,4} int32 value, step number, or noteoff
 * encoders send float 0-1
 * buttons send noteons that's it. 
 * 
 */
  #include <OSCBundle.h>
  #include <OSCBoards.h>
  #include <OSCTiming.h>

  #define OSC_MESG_SIZE 128


  #ifdef BOARD_HAS_USB_SERIAL
    #include <SLIPEncodedUSBSerial.h>
    SLIPEncodedUSBSerial SLIPSerial( thisBoardsSerialUSB );
  #else
    #include <SLIPEncodedSerial.h>
     SLIPEncodedSerial SLIPSerial(Serial);
  #endif /*BOARD_HAS_USB_SERIAL*/
OSCBundle osc_bundle;

void osc_send_loop(void * parameters){
  SLIPSerial.begin(115200);//230400,460800

  for(;;){
    //loop through panels
    for(int i=0;i<N_PANELS;i++){
      //loop through encoders
      panel_t *pan = &(panels[i]);
      for(int j=0;j<pan->n_encoders;j++){
        encoder_t *enc = &(pan->encoders[j]);
        if (enc->updated) {
          osc_bundle.add(enc->osc_addr).add((float)(enc->val-enc->min_val)/(float)(enc->max_val-enc->min_val));
          enc->updated=false;
        }
      }
      //loop through buttons
      for(int j=0;j<pan->n_buttons;j++){
        button_t *but = &(pan->buttons[j]);
        if (but->updated) {
          osc_bundle.add(but->osc_addr).add(but->state);
          but->updated=false;
        }
      }
    }
    if (osc_bundle.size()>0){
      SLIPSerial.beginPacket();
      osc_bundle.send(SLIPSerial);
      SLIPSerial.endPacket();
      osc_bundle.empty();
    }
    perfOSCWriteCounter++;
    vTaskDelay(100);
  }
}

void global_brightness(OSCMessage &msg){
  int b=-1;
  if (msg.size()<1) return;
  if (msg.isInt(0)) b = msg.getInt(0);
  if (b<0) return;
  if (b>255) return;
  LEDS.setBrightness(b);
}

#include "osc_routing.h"
void osc_read_loop(void *parameters){
  
  for(;;){
    OSCBundle bundleIN;
    int size;
    while(!SLIPSerial.endofPacket()) {
      if( (size =SLIPSerial.available()) > 0){
           while(size--)
              bundleIN.fill(SLIPSerial.read());
         }else{
      vTaskDelay(100);
         }
    }
    if(!bundleIN.hasError()){
      osc_addr_mask=0;
      char buf[32];
      for (int i=0;i<bundleIN.size();i++){
        bundleIN.getOSCMessage(i)->getAddress(buf);
        log_printf("osc msg received: %s\n",buf);
      }
      bundleIN.dispatch("/brightness",global_brightness);
      bundleIN.route("/all",osc_route_all);
      bundleIN.route("/moss",osc_route_moss);
      bundleIN.route("/origin",osc_route_origin);
      bundleIN.route("/earth",osc_route_earth);

    }else{
      log_printf("osc bundle has error:");
      log_printf(" size: %i",bundleIN.size());
      char buf[32];
      bundleIN.getOSCMessage(0)->getAddress(buf);
      log_printf(" addr:%s",buf);
      log_printf(" error:%i\n",bundleIN.getOSCMessage(0)->getError());
    }
    perfOSCReadCounter++;
    vTaskDelay(100);
  }
}
