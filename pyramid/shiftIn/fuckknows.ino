///* button loop
// * encoders
// * leds
// * osc out when encoder changes
// * osc in enable disable encoder
// * 
// * /select "name1" or "name2" or "name3
// * 
// * 
// * 
// * 
// * 
// */
//
//typedef struct panel_t{
//  uint16_t button_mask;
//  int enable_pin;
//  int led_pin;
//  boolean enabled;
//  button_t[] buttons;
//  
//}panel_t;
//
//
//
//board_t enabled_board;
//
// void select(board_t board){
//  if ( enabled_board.enable_pin >= 0 )
//    digitalWrite(enabled_board.enable_pin,HIGH);//turn off
//  digitalWrite(board.enable_pin,LOW);//turn on
//  enabled_board = board;
// }
//
// /*
//  * buttons
//  * 
//  * on button press, if button_state & button_enabled_mask
//  * 
//  */
//
//typedef enum 
//{
//  TOGGLE
//  MOMENTARY
//} button_type_t;
//
//
//
//void button_pressed(button_t but){
//  if (but.enabled){
//    switch(but.type){
//      case TOGGLE:
//        but.toggle_state = !but.toggle_state;
//        //send osc message
//        //update led array
//        but.panel->
//        break;
//      case MOMENTARY:
//        break;
//    }
//  }
//  
//}
//
//typedef button_t{
//  button_type_t type;
//  boolean toggle_state;
//  int led_idx;
//  panel_t *panel;
//}
//
//#define ENC_N_LEDS 22
//typedef struct panel_t{
//  uint16_t button_mask;
//  int enable_pin;
//  int led_pin;
//  boolean enabled;
//  button_t *buttons;
//  uint8_t n_buttons;
//  encoder_t *encoders;
//  uint8_t n_encoders;
//  CRGB *leds;
//  uint8_t n_leds;
//  
//}panel_t;
//
///*
// * setup:
// * panel_t mossPanel = {
// *      .enable_pin = 12,
// *      .led_pin = 0,
// *      .button_mask = 0b1111000000000000,
// *      .enabled = false,
// *      }
// * mossPanel.buttons = 
// *    {
//        (button_t){ .type = TOGGLE,
//                    .toggle_state=false,
//                    .led_idx = 0,
//      }
//   mossPanel.encoders = 
//      {
//        (encoder_t){ .value = 0;
//                     .led_idx = 1;
//      }
// * 
// * 
// * 
// */
