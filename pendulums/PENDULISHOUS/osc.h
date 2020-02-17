//OSC
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

void start_cycle(OSCMessage &msg){
      xTaskCreatePinnedToCore(
                  cycle,          /* Task function. */
                  "Cycle!",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  10,                /* Priority of the task. */
                  NULL,            /* Task handle. */  
                  1);               /* Task CPU */
}

void stepleft(OSCMessage &msg){
  stepper.step(-1);
}

void stepright(OSCMessage &msg){
  stepper.step(1);
}
void osc_level(OSCMessage &msg){
  level(0.0);
}

void osc_zero(OSCMessage &msg){
  float verticality=get_verticality();
  level_offset = -verticality;
  prefs.putFloat("level_offset",level_offset);
}
void adjust(OSCMessage &msg){
  float ret;
  //adjust by float degrees
  if (msg.size() < 1) return;
  if (msg.isFloat(0)){
    ret = msg.getFloat(0);
  }else{return;}
  level_offset = ret;
  prefs.putFloat("level_offset",level_offset);
  level_changed=true;
}

void restart(OSCMessage &msg){
  ESP.restart();
}

void forget_WiFi(OSCMessage &msg){
  WiFiManager wm;
  wm.resetSettings();
  ESP.restart();
}

void set_rise_degrees(OSCMessage &msg){
  int ret;
  if (msg.size() < 1) return;
  if (msg.isInt(0)){
    ret = msg.getInt(0); 
    prefs.putInt("rise_degrees",ret);
    rise_degrees=ret;
  }
}

void osc_calm(OSCMessage &msg){
  if (msg.size() <1) return;
  int ret=-1;
  if (msg.isInt(0)) ret = msg.getInt(0);
  if (ret<0) return;
  calm_time = ret;
  prefs.putInt("calm_time",calm_time);
}

void osc_engaged(OSCMessage &msg){
  if (msg.size() <1) return;
  int ret=-1;
  if (msg.isInt(0)) ret = msg.getInt(0);
  if (ret<0) return;
  engaged_time = ret;
  prefs.putInt("engaged_time",engaged_time);
}
void osc_pause(OSCMessage &msg){
  if (msg.size() <1) return;
  int ret=-1;
  if (msg.isInt(0)) ret = msg.getInt(0);
  if (ret<0) return;
  pause_time = ret;
  prefs.putInt("pause_time",pause_time);
}


void osc_swingtime(OSCMessage &msg){
  if (msg.size() <1) return;
  int ret=-1;
  if (msg.isInt(0)) ret = msg.getInt(0);
  if (ret<0) return;
  swing_time = ret;
  prefs.putInt("swing_time",swing_time);  
}

void osc_cycle(OSCMessage &msg, int addrOffset){
  msg.dispatch("/calm",osc_calm,addrOffset);
  msg.dispatch("/engaged",osc_engaged,addrOffset);
  msg.dispatch("/pause",osc_pause,addrOffset);
  msg.dispatch("/swingtime",osc_swingtime,addrOffset);
}

void osc_handler(void *parameters){
      int size;
    OSCMessage msg;
    for(;;){
      msg.empty();
      if( (size = udp.parsePacket())>0){
       while(size--)
         msg.fill(udp.read());
    
        if(!msg.hasError()){
          char buf[64];
          msg.getAddress(buf);
          udp.beginPacket(broadcast_ip,udp_log_port);
          udp.printf("Received osc message: %s\n",buf);
          udp.endPacket();
          msg.dispatch("/go", start_cycle);
          msg.dispatch("/adjust",adjust);
          msg.dispatch("/restart",restart);
          msg.dispatch("/forget_WiFi",forget_WiFi);
          msg.dispatch("/rise_degrees",set_rise_degrees);
          msg.dispatch("/stepright",stepright);
          msg.dispatch("/stepleft",stepleft);
          msg.dispatch("/zero",osc_zero);
          msg.route("/cycle",osc_cycle);
          msg.dispatch("/level",osc_level);
        }else{
              udp.beginPacket(broadcast_ip,udp_log_port);
              udp.printf("OSC message error:%s",msg.getError());
              udp.endPacket();
        }
     }
     vTaskDelay(200);
   }
}
