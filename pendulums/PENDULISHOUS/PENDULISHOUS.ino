#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <Preferences.h>
Preferences prefs;

#include <WiFiUdp.h>
const int udp_log_port = 5554;
IPAddress broadcast_ip;
WiFiUDP udp;

#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <OSCMessage.h>

const uint64_t pend1mac=0xa4cf129a1a88;
#define HOSTNAME "Pendulum_2"

 #include <Stepper.h>
 
// change this to the number of steps on your motor
#define STEPS 200


#define AIN1 19
#define BIN1 22
#define AIN2 18
#define BIN2 23
#define PWMA 5
#define PWMB 6
#define STBY 9

#define EM  4

Stepper stepper(STEPS, AIN2, AIN1, BIN1, BIN2);


#include <MPU9250_asukiaaa.h>

#ifdef _ESP32_HAL_I2C_H_
#define SDA_PIN 15
#define SCL_PIN 2                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
#endif

MPU9250_asukiaaa mySensor;
float aX, aY, aZ, aSqrt, gX, gY, gZ, mDirection, mX, mY, mZ;






float level_offset=2.0;
int calm_time = 2000;
int engaged_time = 6000;
int pause_time = 4000;
int swing_time = 200000;
int rise_degrees = 180;

boolean level_changed = 0;

void cycle(void * parameters){
        Serial.println("ON!");
        udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("ON! calm:%i engaged:%i degrees:%i pause:%i swing:%i\n",calm_time,engaged_time,rise_degrees,pause_time,swing_time);
  udp.endPacket();
    delay(calm_time);// Calm before engage!  
    //vTaskDelay(calm_time);
    digitalWrite(EM,HIGH);           // Engage EM
    Serial.printf("EM ENGAGED - PENDULUM LOCKED for %i milliseconds",engaged_time);
      udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("EM ENGAGED - PENDULUM LOCKED for %i millis\n",engaged_time);
  udp.endPacket();
    delay(engaged_time);// Wait till we rise!
    stepper.step(-degrees_to_steps(rise_degrees));
    Serial.println("Raised to position");
      udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("Raised to position. pausing for %i millis\n",pause_time);
  udp.endPacket();
    delay(pause_time); //pause before DROP 
    digitalWrite(EM,LOW);           // Disengage EM
    Serial.println("PENDULUM RELEASED!");
      udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("PENDULUM RELEASED! waiting for %i millis\n",swing_time);
  udp.endPacket();
    delay(swing_time); // wait for pendulum to stop!
    Serial.println("Return to home position");
      udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("Return to home position\n");
  udp.endPacket();
    level();
    Serial.println("Welcome home cowboy - now rest up 'til your next swing");
      udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("Welcome home cowboy - now rest up 'til your next swing\n");
  udp.endPacket();
}

void level() {
  float verticality=get_verticality();

    //if verticality is <0
      udp.beginPacket(broadcast_ip,udp_log_port);
      udp.printf("adjusting back to level");
      udp.endPacket();
      //27*STEPS would be 360 degrees
      // so step verticality*27*steps/360
      
      stepper.step(int((verticality+level_offset)*27.0*float(STEPS)/360.0));
      verticality = get_verticality();//do it one more time
      stepper.step(int((verticality+level_offset)*27.0*float(STEPS)/360.0));
      udp.beginPacket(broadcast_ip,udp_log_port);
      udp.printf("hopefully this is better?\n");
      udp.endPacket();
      
      get_verticality();

}

#include "osc.h"

#include "ota.h"


void setup() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    Serial.begin(115200);
    delay(200);
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    Serial.printf("MAC ADDR:%x:%x:%x:%x:%x:%x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    stepper.setSpeed(18);
      pinMode(EM, OUTPUT);        
      digitalWrite(EM,LOW); 
    delay(1000);
    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    //reset settings - wipe credentials for testing
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("Pendulum_1_AP"); // anonymous ap
    wm.setTimeout(120);
    res = wm.autoConnect(HOSTNAME"_AP","Pangolin303"); // password protected ap
    if (res) {
      broadcast_ip = WiFi.localIP();
      broadcast_ip[3] = 255;
      Serial.println("Connected to WiFI");
      udp.beginPacket(broadcast_ip,udp_log_port);
      udp.printf("Connected to WiFi sucessfully, ipaddress = %d.%d.%d.%d\n",broadcast_ip[0],broadcast_ip[1],broadcast_ip[2],WiFi.localIP()[3]);
      udp.endPacket();
    }else{

      Serial.println("Failed to connect");
      ESP.restart();
    }


  MDNS.addService("osc", "udp", 8888);

  #ifdef _ESP32_HAL_I2C_H_ // For ESP32
  Wire.begin(SDA_PIN, SCL_PIN);
  mySensor.setWire(&Wire);
#endif

  mySensor.beginAccel();


  //get level offset from nvs
  prefs.begin("pendulum",false);
  level_offset = prefs.getFloat("level_offset",0.0);
  udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("Read saved preferences:\n");
  udp.printf("Level offset:%f ",level_offset);
  rise_degrees = prefs.getInt("rise_degrees",rise_degrees);
  udp.printf("rise degrees:%i ",rise_degrees);
  calm_time = prefs.getInt("calm_time",calm_time);
  udp.printf("calm time:%i ",calm_time);
  engaged_time = prefs.getInt("engaged_time",engaged_time);
  udp.printf("engaged_time:%i ",engaged_time);
  pause_time = prefs.getInt("pause_time",pause_time);
  udp.printf("pause time:%i ",pause_time);
  swing_time = prefs.getInt("swing_time",swing_time);
  udp.printf("swing time:%i\n",swing_time);
  udp.endPacket();

  udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("Started...\n");
  udp.endPacket();



    udp.beginPacket(broadcast_ip,udp_log_port);
  udp.printf("Levelling\n");
  udp.begin(8888);

    xTaskCreate(
                  osc_handler,          /* Task function. */
                  "OSC Handler Loop",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  1,                /* Priority of the task. */
                  NULL);            /* Task handle. */  
                  
    xTaskCreate(
                  ota_handler,          /* Task function. */
                  "OTA Handler Loop",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  1,                /* Priority of the task. */
                  NULL);            /* Task handle. */   
}





inline int degrees_to_steps(int deg){
  return deg*27*STEPS/360;
}



void loop() {

    // put your main code here, to run repeatedly:
    //udp.beginPacket(broadcast_ip,udp_log_port);
    //udp.printf("8\n");
    //udp.endPacket();

   delay(200);
   static unsigned int last_level_time=millis();
   if (level_changed){
    if (millis()-last_level_time > 1000){
      level();
      level_changed=false;
      last_level_time = millis();
    }
   }
}

#define N_ACCEL_UPDATES 3
#define ACCEL_DELAY 20
float get_verticality(){
  float ret=0.0;
    int n=0;
    for (int i=0;i<N_ACCEL_UPDATES;i++){
      if (mySensor.accelUpdate() ==0){
        aX = mySensor.accelX();
        aY = mySensor.accelY();
        //aZ = mySensor.accelZ();
        //aSqrt = mySensor.accelSqrt();
        ret += degrees(atan2(aY,-aX));
        n++;
      }
      delay(ACCEL_DELAY);
    }
    if (n==0){
      udp.beginPacket(broadcast_ip,udp_log_port);
      udp.printf("ERROR CANNOT READ ACCEL\n");
      udp.endPacket();
      return ret;
    }
    ret = ret / (float)n;
    udp.beginPacket(broadcast_ip,udp_log_port);
    udp.printf("verticality: %f\n",ret);
    udp.endPacket();
    return ret;
}
