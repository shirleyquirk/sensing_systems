/*
 * TODO: why aren't you using log_printf? you weirdo
 * 
 * 
 * 
 */

const IPAddress LOG_DEST(192,168,8,255);

 

 //Listen to logging with nc -kluw 1 LOG_PORT

  #define __log(NAME,...) do{\
    udp.beginPacket(LOG_DEST,LOG_PORT);\
    udp.NAME(__VA_ARGS__);\
    udp.endPacket();\
    }while(0)
/*
  #define __log(NAME,...) do{\
    udp.beginPacket(broadcast_ip,LOG_PORT);\
    udp.NAME(__VA_ARGS__);\
    udp.endPacket();\
    }while(0)
#endif/*LOG_DEST*/

#define log_printf(...) __log(printf,__VA_ARGS__)
#define log_println(...) __log(println,__VA_ARGS__)
#define log_print(...) __log(print,__VA_ARGS__)
