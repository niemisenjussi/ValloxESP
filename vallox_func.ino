#include "vallox_structs.h"

#define SYSTEM 0
#define SENDER 1
#define RECEIVER 2
#define VARIABLE 3
#define VARDATA 4
#define CHECKSUM 5


bool validateValloxPacket(ValloxPacket *packet){
  uint8_t checksum = packet->system + packet->sender + packet->receiver + packet->variable + packet->vardata;
  if (checksum == packet->checksum){
    return true;
  }
  return false;
}

bool setFanSpeed(uint8_t speed){
  uint8_t raw_speed = fan_encoder(speed);
  ValloxPacket master = generatePacket(0x11, 0x20, 0x29, raw_speed); //0x01, 0x11, 0x20, 0x29, 0x07, 0x62,
  ValloxPacket remote = generatePacket(0x21, 0x10, 0x29, raw_speed); //0x01, 0x21, 0x10, 0x29, 0x07, 0x62
  
  if (speed >= 2 && speed <= 8){
    Serial.println("Setting speed to"+String(speed));
    Serial2.write(reinterpret_cast<uint8_t*>(&master), sizeof(master));
    Serial2.write(reinterpret_cast<uint8_t*>(&remote), sizeof(remote));
    Serial.println("Fan speed set to"+String(speed));
    return true;
  }
  return false;
}

ValloxPacket generatePacket(uint8_t sender, uint8_t receiver, uint8_t variable, uint8_t value){
  uint8_t checksum = 0x01 + sender + receiver + variable + value;
  ValloxPacket packet = {0x01, sender, receiver, variable, value, checksum};
  return packet;
}

bool parsePacket(ValloxPacket *packet){
  // parse only packets which are send by master
  if (packet->sender >= 0x11 && packet->sender <= 0x1F){
    for (uint8_t i = 0; i < variable_count; i++){
      if (global_state[i].position == packet->variable){
        global_state[i].value = global_state[i].decoder(packet->vardata);
        break;
      }
    }
  }
  packet_log[packet_log_position++] = *packet;
  if (packet_log_position > packet_log_size-1){
    packet_log_position = 0;
  }
}

bool KPItick(ValloxKPIEntry *entry){
  kpi_log[current_kpi_position++] = *entry;
  if (current_kpi_position > KPI_LOG_SIZE-1){
    current_kpi_position = 0;
  }
}


IntParam* get_control_param_pointer(String name){
  for (uint8_t i=0; i < control_variable_count; i++){
    if (control_parameters[i].name == name){
      return &control_parameters[i];
    }
  }
}
