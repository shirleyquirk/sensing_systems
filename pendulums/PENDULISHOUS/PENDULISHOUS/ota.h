void ota_handler(void *parameters){
      //ArduinoOTA
    ArduinoOTA.setPort(3232);
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword("Pangolin303");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      //Serial.println("Start updating " + type);
      udp.beginPacket(broadcast_ip,udp_log_port);
      udp.printf("Start updating %s\n",type);
      udp.endPacket();
    })
    .onEnd([]() {
      //Serial.println("\nEnd");
      udp.beginPacket(broadcast_ip,udp_log_port);
      udp.printf("\nEnd\n");
      udp.endPacket();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      udp.beginPacket(broadcast_ip,udp_log_port);
      udp.printf("Progress: %u%%\r", (progress / (total / 100)));
      udp.endPacket();
    })
    .onError([](ota_error_t error) {
      udp.beginPacket(broadcast_ip,udp_log_port);
      udp.printf("Error[%u]: ", error);
      //Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) udp.printf("Auth Failed\n");//Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) udp.printf("Begin Failed\n");//Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) udp.printf("Connect Failed\n");//Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) udp.printf("Receive Failed\n");//Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) udp.printf("End Failed\n");//Serial.println("End Failed");
      udp.endPacket();
    });

  ArduinoOTA.begin();
  for(;;){
    ArduinoOTA.handle();
    vTaskDelay(500);
  }
}
