
int32_t osc_addr_mask;
//first four bits = panel
typedef enum {
  PANEL_MOSS=0,
  PANEL_EARTH,
  PANEL_ORIGIN,
  PANEL_TOP,
  INPUT_ENC,
  INPUT_BUT,
  INPUT_N_1,
  INPUT_N_2,
  INPUT_N_3,
  INPUT_N_4
} osc_addr_mask_t;
#define PANEL_ALL 15
//((1 << PANEL_TOP) || (1 << PANEL_MOSS) || (1 << PANEL_EARTH) || (1<<PANEL_ORIGIN))
const int INPUT_ALL = ((1 << INPUT_ENC) | (1 <<INPUT_BUT));
const int INPUT_N_ALL = ((1 << INPUT_N_1) | (1 << INPUT_N_2) | (1 << INPUT_N_3) | (1 << INPUT_N_4));





void disable_panel(panel_t* panel){
  if (panel->has_en_pin){
    gpio_set_level(panel->enable_pin,HIGH);
  }
  panel->enabled=false;
  log_println("done.");
}
void enable_panel(panel_t* panel){
  if (panel->has_en_pin){//must disable all other panels with en pins
    for(int i=0;i<N_PANELS;i++){
      if (panels[i].has_en_pin){
        gpio_set_level(panels[i].enable_pin,HIGH);
        panels[i].enabled=false;
      }
    }
    gpio_set_level(panel->enable_pin,LOW);
  }
  panel->enabled=true;
  log_println("done.");
}

void osc_encoder_width(OSCMessage &msg){
  int ret = -1;
  if (msg.isInt(0)) ret=msg.getInt(0);
  if (ret<0) return;
  encoder_width = ret;
}

void osc_panel_enable(OSCMessage &msg){
  log_printf("osc_addr_mask: %x\n",osc_addr_mask);
  for (int i=PANEL_MOSS;i<=PANEL_TOP;i++){
    if (osc_addr_mask & (1<<i)){
      log_printf("enabling panel %i...",i);
      enable_panel(&(panels[i]));
    }
  }
}

void osc_panel_disable(OSCMessage &msg){
  log_printf("osc_addr_mask: %x\n",osc_addr_mask);
  for (int i=PANEL_MOSS;i<=PANEL_TOP;i++){
    if (osc_addr_mask & (1<<i)){
      log_printf("disabling panel %i...",i);
        disable_panel(&(panels[i]));
    }
  }
}

void osc_colour(OSCMessage &msg){

  //we expect an int index and an uint32 colour
  int idx=-1;
  CRGB col=0;
  if (msg.size() <2) return;
  if (msg.isInt(0)) {
    idx=msg.getInt(0);
    if (idx<0) return;
  }else {
    return;
  }
  if (msg.isInt(1)) {
    col=CRGB((uint32_t)(msg.getInt(1)));
  }else{
    return;
  }
  log_printf("osc set colour:%#08X addr_mask:%#06X",col,osc_addr_mask);
  //now set the right colour
  for (int i=PANEL_MOSS;i<=PANEL_TOP;i++){
    if (osc_addr_mask & (1<<i)){
      panel_t *pan = &(panels[i]);
      if (osc_addr_mask & (1<<INPUT_ENC)){
        if (idx >= MAX_ENCODER_COLOURS) continue;
        encoder_t *enc = pan->encoders;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int encoder_index = j-INPUT_N_1;
          if (encoder_index >= pan->n_encoders) continue;
          if (osc_addr_mask & (1<<j))
            enc[encoder_index].gradient_colours[idx]=col;
        }
      }
      if (osc_addr_mask & (1<<INPUT_BUT)){
        if (idx >= MAX_BUTTON_COLORS) continue;
        button_t *but = pan->buttons;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int button_index = j-INPUT_N_1;
          if (button_index >= pan->n_buttons) continue;
          if (osc_addr_mask & (1<<j))
            but[button_index].state_colours[idx]=col;
        }
      }
    }
  }
}
void osc_n_states(OSCMessage &msg){
  int n=-1;
  if (msg.size() < 1) return;
  if (msg.isInt(0)) n=msg.getInt(0);
  if (n<0) return;//0 states is probably not what we want, but wont break anything
    log_printf("osc set n_states:%i addr_mask:%#06X",n,osc_addr_mask);
  //now set the right n_states
  for (int i=PANEL_MOSS;i<=PANEL_TOP;i++){
    if (osc_addr_mask & (1<<i)){
      panel_t *pan = &(panels[i]);
      if (osc_addr_mask & (1<<INPUT_ENC)){
        if (n >= MAX_ENCODER_COLOURS) continue;
        encoder_t *enc = pan->encoders;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int encoder_index = j-INPUT_N_1;
          if (encoder_index >= pan->n_encoders) continue;
          if (osc_addr_mask & (1<<j))
            enc[encoder_index].n_gradient_colours=n;
        }
      }
      if (osc_addr_mask & (1<<INPUT_BUT)){
        if (n >= MAX_BUTTON_COLORS) continue;
        button_t *but = pan->buttons;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int button_index = j-INPUT_N_1;
          if (button_index >= pan->n_buttons) continue;
          if (osc_addr_mask & (1<<j))
            but[button_index].n_states=n;
        }
      }
    }
  }

}
void osc_value(OSCMessage &msg){
  //not gonna happen
  float v = -1.0;
  if (msg.size() < 1) return;
  if (msg.isFloat(0)) v = msg.getFloat(0);
  if (v<0.0) return;
  if (v>1.0) return;
    log_printf("osc set value:%f addr_mask:%#06X",v,osc_addr_mask);
    //now set the right n_states
  for (int i=PANEL_MOSS;i<=PANEL_TOP;i++){
    if (osc_addr_mask & (1<<i)){
      panel_t *pan = &(panels[i]);
      if (osc_addr_mask & (1<<INPUT_ENC)){
        encoder_t *enc = pan->encoders;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int encoder_index = j-INPUT_N_1;
          if (encoder_index >= pan->n_encoders) continue;
          if (osc_addr_mask & (1<<j))
            enc[encoder_index].val = (int)(((float)(enc[encoder_index].max_val-enc[encoder_index].min_val))*v)+enc[encoder_index].min_val;
        }
      }
      if (osc_addr_mask & (1<<INPUT_BUT)){
        button_t *but = pan->buttons;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int button_index = j-INPUT_N_1;
          if (button_index >= pan->n_buttons) continue;
          if (osc_addr_mask & (1<<j))
            but[button_index].state = (int)((float)(but[button_index].n_states)*v);
        }
      }
    }
  }
}
void osc_max(OSCMessage &msg){
  int m=-1;
  if (msg.size() < 1) return;
  if (msg.isInt(0)) m=msg.getInt(0);
  if (m<0) return;
    log_printf("osc set max value:%d addr_mask:%#06X",m,osc_addr_mask);
   for (int i=PANEL_MOSS;i<=PANEL_TOP;i++){
    if (osc_addr_mask & (1<<i)){
      panel_t *pan = &(panels[i]);
      if (osc_addr_mask & (1<<INPUT_ENC)){
        encoder_t *enc = pan->encoders;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int encoder_index = j-INPUT_N_1;
          if (encoder_index >= pan->n_encoders) continue;
          if (osc_addr_mask & (1<<j))
            if (m>enc[encoder_index].min_val) enc[encoder_index].max_val = m;
        }
      }
    }
  }
}
void osc_min(OSCMessage &msg){
  int m=-1;
  if (msg.size() < 1) return;
  if (msg.isInt(0)) m=msg.getInt(0);
  if (m<0) return;
    log_printf("osc set min:%d addr_mask:%#06X",m,osc_addr_mask);
   for (int i=PANEL_MOSS;i<=PANEL_TOP;i++){
    if (osc_addr_mask & (1<<i)){
      panel_t *pan = &(panels[i]);
      if (osc_addr_mask & (1<<INPUT_ENC)){
        encoder_t *enc = pan->encoders;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int encoder_index = j-INPUT_N_1;
          if (encoder_index >= pan->n_encoders) continue;
          if (osc_addr_mask & (1<<j))
            if (m<enc[encoder_index].max_val) enc[encoder_index].min_val = m;
        }
      }
    }
  }
}

void osc_led_offset(OSCMessage &msg){
  int ledoff = -1;
  if (msg.size()<1) return;
  if (msg.isInt(0)) ledoff=msg.getInt(0);
  if (ledoff<0) return;
    log_printf("osc set led offset:%i addr_mask:%#06X",ledoff,osc_addr_mask);
  for (int i=PANEL_MOSS;i<=PANEL_TOP;i++){
    if (osc_addr_mask & (1<<i)){
      panel_t *pan = &(panels[i]);
      if (ledoff>=pan->n_leds) continue;
      if (osc_addr_mask & (1<<INPUT_ENC)){
        if (ledoff>=(pan->n_leds-LEDS_PER_ENC)) continue;
        encoder_t *enc = pan->encoders;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int encoder_index = j-INPUT_N_1;
          if (encoder_index >= pan->n_encoders) continue;
          if (osc_addr_mask & (1<<j))
            enc[encoder_index].led_offset=ledoff;
        }
      }
      if (osc_addr_mask & (1<<INPUT_BUT)){
        button_t *but = pan->buttons;
        for (int j=INPUT_N_1;j<=INPUT_N_4;j++){
          int button_index = j-INPUT_N_1;
          if (button_index >= pan->n_buttons) continue;
          if (osc_addr_mask & (1<<j))
            but[button_index].led_offset=ledoff;
        }
      }
    }
  }
}

void osc_route_message(OSCMessage &msg,int addrOffset){
  msg.dispatch("/colour",osc_colour,addrOffset);
  msg.dispatch("/n_states",osc_n_states,addrOffset);
  msg.dispatch("/value",osc_value,addrOffset);
  msg.dispatch("/max",osc_max,addrOffset);
  msg.dispatch("/min",osc_min,addrOffset);
  msg.dispatch("/led_offset",osc_led_offset,addrOffset);
}

void osc_route_inputs_all(OSCMessage &msg,int addrOffset){
  osc_addr_mask |= INPUT_N_ALL;
  osc_route_message(msg,addrOffset);
}
void osc_route_inputs_1(OSCMessage &msg,int addrOffset){
  osc_addr_mask |= (1<<INPUT_N_1);
  osc_route_message(msg,addrOffset);
}
void osc_route_inputs_2(OSCMessage &msg,int addrOffset){
  osc_addr_mask |= (1<<INPUT_N_2);
  osc_route_message(msg,addrOffset);
}
void osc_route_inputs_3(OSCMessage &msg,int addrOffset){
  osc_addr_mask |= (1<<INPUT_N_3);
  osc_route_message(msg,addrOffset);
}
void osc_route_inputs_4(OSCMessage &msg,int addrOffset){
  osc_addr_mask |= (1<<INPUT_N_4);
  osc_route_message(msg,addrOffset);
}
void osc_route_n_inputs(OSCMessage &msg,int addrOffset){
  msg.route("/all",osc_route_inputs_all,addrOffset);
  msg.route("/1",osc_route_inputs_1,addrOffset);
  msg.route("/2",osc_route_inputs_2,addrOffset);
  msg.route("/3",osc_route_inputs_3,addrOffset);
  msg.route("/4",osc_route_inputs_4,addrOffset);
}
void osc_route_input_all(OSCMessage &msg,int addrOffset){
  osc_addr_mask |= INPUT_ALL;
  osc_route_n_inputs(msg,addrOffset);
}
void osc_route_encoder(OSCMessage &msg,int addrOffset){
  osc_addr_mask |= (1<<INPUT_ENC);
  osc_route_n_inputs(msg,addrOffset);
}

void osc_route_button(OSCMessage &msg,int addrOffset){
  osc_addr_mask |= (1<<INPUT_BUT);
  osc_route_n_inputs(msg,addrOffset);
}
void osc_route_input(OSCMessage &msg,int addrOffset){
    log_printf("route_input: %x\n",osc_addr_mask);
  msg.dispatch("/enable",osc_panel_enable,addrOffset);
  msg.dispatch("/disable",osc_panel_disable,addrOffset);
  msg.route("/all",osc_route_input_all,addrOffset);
  msg.route("/encoder",osc_route_encoder,addrOffset);
  msg.route("/button",osc_route_button,addrOffset);
}
void osc_route_all(OSCMessage &msg,int addrOffset){
  osc_addr_mask = PANEL_ALL;
  osc_route_input(msg,addrOffset);
}
void osc_route_top(OSCMessage &msg,int addrOffset){
  osc_addr_mask = (1<<PANEL_TOP);
  osc_route_input(msg,addrOffset);
}
void osc_route_moss(OSCMessage &msg,int addrOffset){
  osc_addr_mask = (1<<PANEL_MOSS);
  osc_route_input(msg,addrOffset);
}
void osc_route_origin(OSCMessage &msg,int addrOffset){
  osc_addr_mask = (1<<PANEL_ORIGIN);
  log_printf("route_origin : %x\n",osc_addr_mask);
  
  osc_route_input(msg,addrOffset);
}
void osc_route_earth(OSCMessage &msg,int addrOffset){
  osc_addr_mask = (1<<PANEL_EARTH);
  osc_route_input(msg,addrOffset);
}
