/*
 * report network connectivity
 * 
 * supposed to reconnect wtf
 */
#include "logging.h"
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
