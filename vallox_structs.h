#ifndef VALLOX_STRUCTS_H
#define VALLOX_STRUCTS_H

#define packet_log_size 50

#define KPI_LOG_SIZE 360 //How many seconds we collect KPI logs


#define FAN_BOOST_TIME 0
#define FAN_BOOST_SPEED 1
#define RH_BOOST_START 2
#define RH_BOOST_STOP 3
#define RH_HYSTERESIS 4
#define CONTROL_ENABLED 5
#define BASE_FAN_SPEED 6
#define PENALTY_TIMER 7
#define FAN_MIN_BOOST_TIME 8
#define BASE_FAN_SPEED_WINTER 9
#define BOOST_FAN_SPEED_WINTER 10

//Prototypes
float lamp_decoder(uint8_t value);


// Used to control our own locig to activate FAN boosting
struct IntParam{
  uint8_t data;
  uint8_t address;
  String name;
  uint8_t min_value;
  uint8_t max_value;
};


struct ValloxVariable{
  const uint8_t position;
  const char* description;
  float value;
  float (*decoder)(uint8_t value);
};

struct ValloxPacket{
  uint8_t system;
  uint8_t sender;
  uint8_t receiver;
  uint8_t variable;
  uint8_t vardata;
  uint8_t checksum;
};

struct ValloxKPIEntry{
  uint32_t log_position;
  int8_t fresh_air_temp;
  int8_t dirty_air_temp;
  int8_t incoming_air_temp;
  int8_t outgoing_air_temp;
  uint8_t heater_on;
  uint8_t fan_speed;
  uint8_t humidity;
  float bathroom_temp;
};

uint8_t fan_speeds[] = {0x01, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF}; // 0 => same as speed 1 so 1:1 can be used


float NTC_LOOKUP[] = {-74, -70, -66, -62, -59, -56, -54, -52, -50, -48, -47, -46, -44, -43, -42, -41, -40, -39, -38, -37, -36, -35, -34, -33, -33, -32, -31, -30, -30, -29, -28.3, -27.7, -27.3, -26.6, -26, -25.3, -24.7, -24.3, -23.7, -23.3, -22.7, -22.3, -21.7, -21.3, -20.7, -20.3, -19.8, -19.4, -19, -18.7, -18.4, -17.8, -17.3, -16.8, -16.4, -16, -15.7, -15.3, -14.8, -14.3, -14, -13.6, -13.2, -12.8, -12.4, -12, -11.7, -11.4, -11, -11.6, -10.3, -9.8, -9.4, -9.1, -8.8, -8.4, -8.1, -7.8, -7.4, -7.1, -6.8, -6.4, -6.1, -5.8, -5.4, -5, -4.7, -4.4, -4, -3.6, -3.3, -3, -2.7, -2.4, -2.1, -1.6, -1.25, -1, -0.75, -0.5, -0.25, 0, 0.33, 0.66, 1, 1.3, 1.6, 2, 2.3, 2.6, 2.9, 3.3, 3.6, 3.9, 4.25, 4.5, 4.75, 5, 5.25, 5.7, 6, 6.2, 6.5, 6.8, 7.3, 7.6, 8, 8.4, 8.8, 9, 9.3, 9.6, 10, 10.3, 10.6, 11, 11.3, 11.6, 12, 12.3, 12.6, 13, 13.4, 13.8, 14.1, 14.4, 14.7, 15, 15.4, 15.8, 16.1, 16.4, 16.8, 17.2, 17.5, 18, 18.4, 18.7, 19, 19.4, 19.9, 20.4, 20.8, 21.1, 21.4, 21.8, 22.1, 22.4, 22.8, 23.2, 23.8, 24.1, 24.4, 24.8, 25.3, 25.6, 26.3, 26.8, 27, 27.4, 27.8, 28.3, 28.7, 29.3, 29.7, 30.4, 30.8, 31.4, 31.8, 32.3, 32.8, 33.4, 33.8, 34.4, 34.8, 35.3, 35.8, 36.3, 36.8, 37.3, 38, 38, 39, 40, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 48, 48, 49, 50, 51, 52, 53, 53, 54, 55, 56, 57, 59, 60, 61, 62, 63, 65, 66, 68, 69, 71, 73, 75, 77, 79, 81, 82, 86, 90, 93, 97, 100, 100, 100, 100, 100, 100, 100, 100, 100};


// Used to control our own locig to activate FAN boosting
IntParam control_parameters[] = {{90, FAN_BOOST_TIME, "fan_boost_time", 0, 255},
                                {6,  FAN_BOOST_SPEED, "fan_boost_speed", 3, 8},
                                {15,  FAN_MIN_BOOST_TIME, "fan_min_boost_time", 0, 255},
                                {70, RH_BOOST_START, "rh_boost_start", 10, 100},
                                {58, RH_BOOST_STOP, "rh_boost_stop", 10, 100},
                                {10, RH_HYSTERESIS, "rh_hysteresis", 0, 20},
                                {1,  CONTROL_ENABLED, "control_enabled", 0, 1},
                                {4,  BASE_FAN_SPEED, "fan_base_speed", 3, 8},
                                {10,  PENALTY_TIMER,  "penalty_timer", 0, 255},
                                {3,  BASE_FAN_SPEED_WINTER, "fan_base_speed_winter", 2, 8},
                                {5,  BOOST_FAN_SPEED_WINTER, "fan_boost_speed_winter", 2, 8},
                                };

uint8_t control_variable_count = sizeof(control_parameters) / sizeof(control_parameters[0]);


void init_control_parameters(void){
  for (uint8_t i=0; i < control_variable_count; i++){
    control_parameters[i].data = EEPROM.read(control_parameters[i].address); 
  }
}

float passthrough_decoder(uint8_t value){
  return float(value);
}

float temp_decoder(uint8_t ntc_val){
  return NTC_LOOKUP[ntc_val];
}

float fan_decoder(uint8_t value){
  switch(value){
    case 0x01:{
      return 1.0;
    }
    case 0x03:{
      return 2.0;
    }
    case 0x07:{
      return 3.0;
    }
    case 0x0F:{
      return 4.0;
    }
    case 0x1F:{
      return 5.0;
    }
    case 0x3F:{
      return 6.0;
    }
    case 0x7F:{
      return 7.0;
    }
    case 0xFF:{
      return 8.0;
    }
  }
  return value;
}

float fan_encoder(uint8_t speed){
  if (speed < 1){
    speed = 1;
  }
  else if (speed > 8){
    speed = 8;
  }
  return float(fan_speeds[speed]);
}

void print_error_text(uint8_t code){
  switch(code){
    case 0x05:{
      Serial.println("Incoming air sensor fail");
      break;
    }
    case 0x06:{
      Serial.println("CO2 alarm");
      break;
    }
    case 0x07:{
      Serial.println("Outside temp sensor fail");
      break;
    }
    case 0x08:{
      Serial.println("Outgoing air temp sensor fail");
      break;
    }
    case 0x09:{
      Serial.println("Waterblock ice warning");
      break;
    }
    case 0x0A:{
      Serial.println("Dirty air temp sensor fail");
      break;
    }
  }
}

float rh_decode(uint8_t value){
    // 0x33 = 0%
    // 0xFF = 100%
    return (value-51.0)/2.04;
}

float heater_percent(uint8_t value){
  return value/2.5;
}

void clear_eeprom(uint8_t size){
  for (uint8_t i = 0; i<size; i++){
    EEPROM.write(i, 0xFF);
  }
  EEPROM.commit();
}


ValloxVariable global_state[] = {
  {0x29, "fan_speed", 0, fan_decoder}, // This is used somewhere directly, do not alter positions
  {0x2A, "max_rh", 0, rh_decode},
  {0x2E, "ma_volt", 0, passthrough_decoder}, // mA/voltage input
  {0x32, "outside_temp", 0, temp_decoder},
  {0x33, "dirty_air_temp", 0, temp_decoder},
  {0x34, "temp_out", 0, temp_decoder},
  {0x35, "temp_in", 0, temp_decoder},
  {0x36, "error_code", 0, passthrough_decoder},
  {0x55, "heater_on_percent", 0, heater_percent},
  {0x56, "heater_off_percent", 0, heater_percent},
  {0x57, "target_temp", 0, temp_decoder},
  {0x6D, "flags2", 0, passthrough_decoder},  // TODO decode flags
  {0x6F, "flags4", 0, passthrough_decoder},  // TODO decode flags
  {0x70, "flags5", 0, passthrough_decoder},  // TODO decode flags
  {0x71, "flags6", 0, passthrough_decoder},  // Bit 4=remotecontrol, 5=Takkakytkin aktivointi, 6=Takka/tehostus luku
  {0x79, "fireplace_boost_timer", 0, passthrough_decoder},
  {0xA3, "lamps", 0, lamp_decoder},
  {0xA4, "after_heater_set_temp", 0, temp_decoder},
  {0xA5, "max_fan_speed", 0, fan_decoder},
  {0xA6, "service_interval", 0, passthrough_decoder},
  {0xA7, "preheater_set_temp", 0, temp_decoder},
  {0xA8, "incoming_air_stop_temp", 0, temp_decoder},
  {0xA9, "base_fan_speed", 0, fan_decoder},
  {0xAA, "time_to_service", 0, passthrough_decoder},  // time in months
  {0xAF, "passthrough_temp", 0, temp_decoder},
  {0xB0, "dc_fan_in_set_speed", 0, passthrough_decoder},
  {0xB1, "dc_fan_out_set_speed", 0, passthrough_decoder},
  {0xB2, "ice_protection_temp_hyst", 0, passthrough_decoder},
  {0xF1, "power_button", 0, passthrough_decoder},
  {0xF2, "CO2_button", 0, passthrough_decoder},
  {0xF3, "RH_button", 0, passthrough_decoder},
  {0xF4, "heater_button", 0, passthrough_decoder},
  {0xF5, "filter_led", 0, passthrough_decoder},
  {0xF6, "heater_led", 0, passthrough_decoder},
  {0xF7, "error_led", 0, passthrough_decoder},
  {0xF8, "service_led", 0, passthrough_decoder}
};

uint8_t variable_count = sizeof(global_state) / sizeof(global_state[0]);


bool set_global_variable(const char* name, int16_t value){
  for (uint8_t i=0; i<variable_count; i++){
    if (strcmp(global_state[i].description, name) == 0){
      global_state[i].value = value;
      return true;
    }
  }
  return false; 
}

uint8_t read_global_variable(const char* name){
  for (uint8_t i=0; i<variable_count; i++){
    if (strcmp(global_state[i].description, name) == 0){
      return global_state[i].value;
    }
  }
  return 0;
}

float read_global_variable_float(const char* name){
  for (uint8_t i=0; i<variable_count; i++){
    if (strcmp(global_state[i].description, name) == 0){
      return global_state[i].value;
    }
  }
  return 0.0;
}


float lamp_decoder(uint8_t value){
  set_global_variable("power_button", (value & 0x01)>>0);
  set_global_variable("CO2_button", (value & 0x02)>>1);
  set_global_variable("RH_button", (value & 0x04)>>2);
  set_global_variable("heater_button", (value & 0x08)>>3);
  set_global_variable("filter_led", (value & 0x10)>>4);
  set_global_variable("heater_led", (value & 0x20)>>5);
  set_global_variable("error_led", (value & 0x40)>>6);
  set_global_variable("service_led", (value & 0x80)>>7);
  return float(value);
}

uint8_t packet_log_position = 0;
ValloxPacket packet_log[packet_log_size] = {};

ValloxKPIEntry kpi_log[KPI_LOG_SIZE] = {};
uint16_t current_kpi_position = 0;

#endif
