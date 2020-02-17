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


//  #ifdef BOARD_HAS_USB_SERIAL
//    #include <SLIPEncodedUSBSerial.h>
//    SLIPEncodedUSBSerial SLIPSerial( thisBoardsSerialUSB );
//  #else
//    #include <SLIPEncodedSerial.h>
//     SLIPEncodedSerial SLIPSerial(Serial);
//  #endif /*BOARD_HAS_USB_SERIAL*/

void restart(OSCMessage &msg){
  log_println("restarting...\n");
  delay(500);
  ESP.restart();
}

void osc_send_loop(void * parameters){
  //SLIPSerial.begin(115200);//230400,460800
  //IPAddress maxwell(192,168,8,5);
  OSCBundle osc_bundle;
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
          //special top button dealy
          if (i==3 && j==0){//top encoder button
            log_println("sending /playrandom to broadcast:5005");
            OSCMessage msg("/playrandom");
            msg.add(1);
            udp.beginPacket(broadcast_ip,5005);//we need mutexes for this shit
            msg.send(udp);
            udp.endPacket();
          }
        }
      }
    }
    if (osc_bundle.size()>0){
      udp.beginPacket(maxwell,1234);
      //SLIPSerial.beginPacket();
      osc_bundle.send(udp);
      udp.endPacket();
      //SLIPSerial.endPacket();
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

//void async_osc_read_loop(void *parameters){
//  if (asudp.listen(8888)){
//    
//  }else{
//    //we're fucked?
//  }
//}

void osc_read_loop(void *parameters){

  for(;;){
    vTaskDelay(100);
    OSCMessage msg;
    int size;
    /*
      while(!SLIPSerial.endofPacket()) {
        if( (size =SLIPSerial.available()) > 0){
             while(size--)
                bundleIN.fill(SLIPSerial.read());
           }else{
        vTaskDelay(100);
           }
      }
      //bundle.empty();
      */
    msg.empty();
    if( (size = udp.parsePacket())>0){//asyncudp?
      if (size == 0) continue;
      while(size--)
        msg.fill(udp.read());
      if(!msg.hasError()){
        osc_addr_mask=0;
        char buf[32];
        //for (int i=0;i<bundleIN.size();i++){
          //bundleIN.getOSCMessage(i)->getAddress(buf);
          msg.getAddress(buf);
          log_printf("osc msg received: %s\n",buf);
        //}
        msg.dispatch("/brightness",global_brightness);
        msg.dispatch("/restart",restart);
        msg.route("/all",osc_route_all);
        msg.route("/moss",osc_route_moss);
        msg.route("/origin",osc_route_origin);
        msg.route("/earth",osc_route_earth);
        msg.route("/top",osc_route_top);
      }else{
        log_printf("osc bundle has error:");
        //log_printf(" size: %i",bundleIN.size());
        char buf[32];
        //bundleIN.getOSCMessage(0)->getAddress(buf);
        msg.getAddress(buf);
        log_printf(" addr:%s",buf);
        log_printf(" error:%i\n",msg.getError());//bundleIN.getOSCMessage(0)->getError());
        //send it anyway?

      }
      perfOSCReadCounter++;

    }//else udp read error
  }
}
