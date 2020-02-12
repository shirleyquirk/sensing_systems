#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager


#include <WiFiUdp.h>

IPAddress broadcast_ip;
WiFiUDP udp;

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


//#define LOGBUF_SIZE 256
//char logbuf[LOGBUF_SIZE];
//TaskHandle_t logging_handle;
//void log_loop(void *parameters){
//  for(;;){
//    if (strlen(logbuf)>0){//strlen(logbuf) > 0
//      udp.beginPacket(LOG_DEST,LOG_PORT);
//      udp.printf("%s",logbuf);
//      strcpy(logbuf,"");
//      udp.endPacket();
//    }
//    vTaskDelay(200);
//  }
//}
//#define SINGLEBUF_SIZE 128
//#define log_printf(...) do{char buf[SINGLEBUF_SIZE]; snprintf(buf,SINGLEBUF_SIZE,__VA_ARGS__); strncat(logbuf,buf,LOGBUF_SIZE-strlen(logbuf));}while(0)

unsigned int perfArduinoOTACounter=0;

TaskHandle_t ota_handle;
void ArduinoOTA_handler(void * parameters){
  const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
  for (;;){
    ArduinoOTA.handle();
    perfArduinoOTACounter++;
    vTaskDelay(xDelay);
  }
}
void resetWifi( OSCMessage &msg){
  WiFiManager wm;
  int timeout = 120;
  if (msg.isInt(0)){
    timeout = msg.getInt(0);
  }else{
    log_printf("/resetWifi value expected an int\n");
  }

  log_printf("starting config portal with timeout of %is",timeout);
  wm.resetSettings();
  if (!wm.autoConnect(AP_SSID,AP_PASS)){
    //then we're fucked. restart
    //log_printf("config portal timeout\n"); 
    ESP.restart();
  }
}


void setup_common(){
  //Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;
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
                    2000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    &ota_handle);            /* Task handle. */
//  xTaskCreate(
//                    log_loop,          /* Task function. */
//                    "Logging",        /* String with name of task. */
//                    2000,            /* Stack size in bytes. */
//                    NULL,             /* Parameter passed as input of the task */
//                    1,                /* Priority of the task. */
//                    &logging_handle);            /* Task handle. */
  //osc listening not for pyramid
//  MDNS.addService("osc", "udp", 8888);
//  udp.begin(8888);
  log_printf("portTICK_PERIOD_MS=%i\n",portTICK_PERIOD_MS);
  //Serial.println("End setup_common");
}
