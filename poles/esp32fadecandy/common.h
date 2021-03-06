#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager


#include <WiFiUdp.h>

IPAddress broadcast_ip;
WiFiUDP udp;

#include <Preferences.h>
Preferences prefs;

#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#define OTA_PORT 3232

//OSC
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

//Listen to logging with nc -kluw 1 LOG_PORT
#ifdef LOG_DEST
  #define __log(NAME,...) do{\
    udp.beginPacket(LOG_DEST,LOG_PORT);\
    udp.NAME(__VA_ARGS__);\
    udp.endPacket();\
    }while(0)
#else
  #define __log(NAME,...) do{\
    udp.beginPacket(broadcast_ip,LOG_PORT);\
    udp.NAME(__VA_ARGS__);\
    udp.endPacket();\
    }while(0)
#endif/*LOG_DEST*/

#define log_printf(...) __log(printf,__VA_ARGS__)
#define log_println(...) __log(println,__VA_ARGS__)
#define log_print(...) __log(print,__VA_ARGS__)

void wifi_reconnect(void *parameters){
  for(;;){
    while (WiFi.status() != WL_CONNECTED){
      //try to reconnect
      if (WiFi.reconnect()){
        vTaskDelay(1000);//wait for logging computer to reconnect too
        log_println("Reconnected to WiFi");
      }
      vTaskDelay(500);
    }
    vTaskDelay(1000);
  }
}

void ArduinoOTA_handler(void * parameters){
  const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
  for (;;){
    ArduinoOTA.handle();
    vTaskDelay(xDelay);
  }
}
void forgetWifi( OSCMessage &msg){
  WiFiManager wm;
  wm.resetSettings();
  ESP.restart();
}

void restart(OSCMessage &msg){
  ESP.restart();
}

float smoothing = 0.5;
void set_smoothing(OSCMessage &msg){
  float ret=-1.0;
  if (msg.isFloat(0)) ret=msg.getFloat(0);
  if (ret<0) return;
  if (ret>1) return;
  smoothing = ret;
  prefs.putFloat("smoothing",smoothing);
}
void OSCMessageTask(void * parameters){
  const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
  for (;;){
    int size;
    OSCMessage msg;
    if( (size = udp.parsePacket())>0){
      while(size--)
        msg.fill(udp.read());
      if(!msg.hasError()){
        msg.dispatch("/forget_Wifi",forgetWifi);
        msg.dispatch("/restart",restart);
        msg.dispatch("/smoothing",set_smoothing);
      }else{
        log_printf("OSC message error:%s",msg.getError());
      }
    }
    vTaskDelay(xDelay);
  }
}


void setup_common(){
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;
  wm.setTimeout(30);
  res = wm.autoConnect(AP_SSID,AP_PASS);//this needs to periodically check to see if our wifi comes online after we boot
  //can connect to fadecandy_ap, get fadecandy_1.local/exit
  if (res){
      broadcast_ip = WiFi.localIP();
      broadcast_ip[3]=255;
        //ArduinoOTA
      ArduinoOTA.setPort(OTA_PORT);
      ArduinoOTA.setHostname(HOSTNAME);
      ArduinoOTA.setPassword(OTA_PASS);
      ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";
  
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        log_printf("Start updating %s\n",type);
      })
      .onEnd([]() {
        log_println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        log_printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        log_printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) log_println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) log_println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) log_println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) log_println("Receive Failed");
        else if (error == OTA_END_ERROR) log_println("End Failed");
      });

      ArduinoOTA.begin();
      delay(100);
      udp.beginPacket(broadcast_ip,LOG_PORT);
      udp.println("OTA begun");
      udp.endPacket();
      //next line crashes if previous three haven't happened, if LOG_DEST is an mdns hostname
      //fuck knows why but it works like this so fuckit.
      log_printf("Connected to WiFi sucessfully, ipaddress = %d.%d.%d.%d\n",WiFi.localIP()[0],WiFi.localIP()[1],WiFi.localIP()[2],WiFi.localIP()[3]);
  }else{
    ESP.restart();
  }

  xTaskCreate(
                    ArduinoOTA_handler,          /* Task function. */
                    "ArduinoOTA.handle",        /* String with name of task. */
                    10000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    2,                /* Priority of the task. */
                    NULL);            /* Task handle. */

  //osc listening
  MDNS.addService("osc", "udp", 8888);
  udp.begin(8888);
  xTaskCreate(
                  OSCMessageTask,          /* Task function. */
                  "OSC Message Listener",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  2,                /* Priority of the task. */
                  NULL);            /* Task handle. */
    xTaskCreate(
                    wifi_reconnect,          /* Task function. */
                    "WiFi reconnect",        /* String with name of task. */
                    2000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL);            /* Task handle. */

}
