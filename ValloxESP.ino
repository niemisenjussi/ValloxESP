#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "EEPROM.h"

#include "vallox_structs.h"

#include <ArduinoOTA.h>

#define SW_VERSION 1.1

const char *ssid = "WLAN_SSID";
const char *password = "WLAN_PASSWORD";

WebServer server(80);

 

// Vallox UART connection
#define RXD2 16
#define TXD2 17

#define LED 2

#define EEPROM_SIZE 255

TaskHandle_t webTask;
TaskHandle_t valloxTask;
TaskHandle_t otaTask;
TaskHandle_t wlanWatchdogTask;
TaskHandle_t KPIloggerTask;
TaskHandle_t RHmonitorTask;
TaskHandle_t valloxTaskMon;

volatile uint32_t webtask_stack = 0;
volatile uint32_t valloxtask_stack = 0;
volatile uint32_t otatask_stack = 0;
volatile uint32_t wlantask_stack = 0;
volatile uint32_t rhtask_stack = 0;

volatile uint32_t ticker = 0;
volatile uint16_t stuck_time = 0;
volatile uint8_t vallox_restarts = 0;

// Global temp variables
volatile float g_temperature = 0;
volatile uint8_t g_humidity = 0;
volatile float g_avg_humidity = 0.0;
volatile float g_before_boost = 0.0; //What was boost humidity before boosting started


volatile uint16_t boost_time = 0; //How long boost has been in use
volatile uint16_t cooldown_time = 0; //How long cooldown has been lasted
volatile bool boost_active = false;
volatile bool cooldown_active = false;

volatile bool winter_mode = false; //Winter/summer fan speed settings


#define AVERAGE_SAMPLES 240 //Number of seconds/samples to average

volatile uint32_t total_packets = 0; //Track total received packets
volatile uint32_t total_invalid_packets = 0; //Track total invalid received packets



void webTaskMain(void * pvParameters){
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.begin(ssid, password);
  // Wait for connection
  int maxwait = 10;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    maxwait -= 1;
    if (maxwait == 0){
       Serial.println("Connection Failed! Rebooting...");
       delay(1000);
       ESP.restart();
    }
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", []() {
    digitalWrite(LED, !digitalRead(LED));
    server.send(200, "text/plain", "hello from esp32!");
  });

  server.on("/measure", []() {
    digitalWrite(LED, !digitalRead(LED));
    String resp = "";
    resp += "{\"temperature\":"+String(g_temperature);
    resp += ",\"humidity\":"+String(g_humidity);
    resp += ",\"before_boost_humidity\":"+String(g_before_boost);
    resp += ",\"avg_humidity\":"+String(g_avg_humidity)+"}";
    
    server.send(200, "application/json", resp);
  });

  server.on("/vallox", []() {
    digitalWrite(LED, !digitalRead(LED));
    String resp = "{";
    for (uint8_t i = 0; i<variable_count; i++){
      if (i > 0){
        resp += ",";
      }
      resp += "\"" + String(global_state[i].position) + "\":{";
      resp += "\"desc\":\"" + String(global_state[i].description) + "\",";
      resp += "\"value\":" + String(global_state[i].value);
      resp += "}";
    }
    resp += ",\"total_packets\":" + String(total_packets);
    resp += ",\"total_invalid_packets\":" + String(total_invalid_packets);
    resp += ",\"humidity\":"+String(g_humidity);
    resp += ",\"before_boost_humidity\":"+String(g_before_boost);
    resp += ",\"avg_humidity\":"+String(g_avg_humidity);
    resp += ",\"webtask\":"+String(webtask_stack);
    resp += ",\"valloxtask\":"+String(valloxtask_stack);
    resp += ",\"otatask\":"+String(otatask_stack);
    resp += ",\"bathroom_temp\":"+String(g_temperature);
    resp += ",\"wlantask\":"+String(wlantask_stack);
    resp += ",\"rhtask\":"+String(rhtask_stack);
    resp += ",\"stuck_time\":"+String(stuck_time);
    resp += ",\"vallox_restarts\":"+String(vallox_restarts);
    resp += ",\"boost_time\":"+String(boost_time);
    resp += ",\"outside_temp\":"+String(read_global_variable_float("outside_temp"));
    resp += ",\"cooldown_time\":"+String(cooldown_time);
    if (boost_active == true){
      resp += ",\"boost_active\":true";
    }
    else{
      resp += ",\"boost_active\":false";
    }

    if (cooldown_active == true){
      resp += ",\"cooldown_active\":true";
    }
    else{
      resp += ",\"cooldown_active\":false";
    }
    
    if (winter_mode == true){
      resp += ",\"winter_mode\":true";
    }
    else{
      resp += ",\"winter_mode\":false";
    }

    resp += "}";

    server.send(200, "application/json", resp);
    webtask_stack = uxTaskGetStackHighWaterMark(NULL);
  });

  server.on("/vallox_packet_log", []() {
    digitalWrite(LED, !digitalRead(LED));
    String resp = "[";
    uint8_t readpos = packet_log_position;
    bool first = true;
    for (uint8_t i = 0; i<packet_log_size; i++){
      if (packet_log[readpos].system != NULL){
        if (first == false){
          resp += ",";
        }
        resp += "{";
        resp += "\"system\":" + String(packet_log[readpos].system)+",";
        resp += "\"sender\":" + String(packet_log[readpos].sender)+",";
        resp += "\"receiver\":" + String(packet_log[readpos].receiver)+",";
        resp += "\"variable\":" + String(packet_log[readpos].variable)+",";
        resp += "\"vardata\":" + String(packet_log[readpos].vardata)+",";
        resp += "\"checksum\":" + String(packet_log[readpos].checksum)+"";
        resp += "}";
        first = false;
      }
      if (readpos == 0){
        readpos = packet_log_size -1;
      }
      readpos -= 1;
    }
    resp += "]";

    server.send(200, "application/json", resp);
  });

  server.on("/setparam/{}/{}", []() {
    String parameter = server.pathArg(0);
    bool found = false;
    
    for (uint8_t i=0; i<control_variable_count; i++){
      if (control_parameters[i].name == parameter){
        uint8_t address = control_parameters[i].address;
        uint8_t value = server.pathArg(1).toInt() & 0xFF;
        if (value > control_parameters[i].max_value){
          value = control_parameters[i].max_value;
        }
        if (value < control_parameters[i].min_value){
          value = control_parameters[i].min_value;
        }
        
        Serial.println("Writing address:"+String(address)+" value:"+String(value));
        EEPROM.write(address, value);
        EEPROM.commit();
        control_parameters[i].data = value;
        found = true;
        break;
      }
    }

    if (found == true){
      server.send(200, "application/json", "{\"status\":\"ok\"}");  
    }
    else{
      server.send(404, "application/json", "{\"error\":\"cannot find parameter\"}");
    }
    
  });

  server.on("/getparam/{}", []() {
    String parameter = server.pathArg(0);
    Serial.println("Reading parameter:"+parameter);
    bool found = false;
    for (uint8_t i=0; i<control_variable_count; i++){
      if (control_parameters[i].name == parameter){
        uint8_t value = control_parameters[i].data;
        Serial.println("Value:"+String(value));
        found = true;
        server.send(200, "application/json", "{\"value\":"+String(value)+"}");
        break;
      }
    }
    if (found == false){
      server.send(404, "application/json", "{\"error\":\"cannot find parameter\"}");
    }
  });

  server.on("/version", []() {
    server.send(200, "application/json", "{\"version\":"+String(SW_VERSION)+"}");
  });

  server.on("/vallox_kpi/{}/{}", []() {
    digitalWrite(LED, !digitalRead(LED));
    // Start_pos and stop_pos valuesa are TICK values from startup. Increments every second. 32bit
    uint32_t start_pos = server.pathArg(0).toInt();
    uint32_t stop_pos = server.pathArg(1).toInt();
    String resp = "";
    resp += "{\"header\":[";
      resp += "\"timestamp\",";
      resp += "\"fresh_air_temp\",";
      resp += "\"dirty_air_temp\",";
      resp += "\"incoming_air_temp\",";
      resp += "\"outgoing_air_temp\",";
      resp += "\"heater_on\",";
      resp += "\"fan_speed\",";
      resp += "\"humidity\",";
      resp += "\"bathroom_temp\"";
    resp += "],\"log\":[";
    
    uint16_t readpos = current_kpi_position;
    bool first = true;
    for (uint16_t i = 0; i<KPI_LOG_SIZE; i++){
      if (kpi_log[readpos].fresh_air_temp != NULL){
        if (kpi_log[readpos].log_position >= start_pos and kpi_log[readpos].log_position <= stop_pos){
          if (first == false){
            resp += ",";
          }
          resp += "[";
          resp += String(kpi_log[readpos].log_position)+",";
          resp += String(kpi_log[readpos].fresh_air_temp)+",";
          resp += String(kpi_log[readpos].dirty_air_temp)+",";
          resp += String(kpi_log[readpos].incoming_air_temp)+",";
          resp += String(kpi_log[readpos].outgoing_air_temp)+",";
          resp += String(kpi_log[readpos].heater_on)+",";
          resp += String(kpi_log[readpos].fan_speed)+",";
          resp += String(kpi_log[readpos].humidity)+",";
          resp += String(kpi_log[readpos].bathroom_temp);
          resp += "]";
          first = false;
        }
      }
      if (readpos == 0){
        readpos = KPI_LOG_SIZE -1;
      }
      readpos -= 1;
    }
    resp += "]}";

    server.send(200, "application/json", resp);
  });

  //TODO api mistä voi kysyä ajanhetken?, ehkä myös login max alun ja lopun


  server.on("/setfan/{}", []() {
    uint8_t speed = server.pathArg(0).toInt();
    Serial.println("Setting fan speed to:"+String(speed));

    if (setFanSpeed(speed) == true){
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    }
    else{
      server.send(404, "application/json", "{\"error\":\"fan speed set error\"}");
    }
  });


  /*server.on("/users/{}", []() {
    String user = server.pathArg(0);
    server.send(200, "text/plain", "User: '" + user + "'");
  });
  
  server.on("/users/{}/devices/{}", []() {
    String user = server.pathArg(0);
    String device = server.pathArg(1);
    server.send(200, "text/plain", "User: '" + user + "' and Device: '" + device + "'");
  });*/

  server.begin();
  Serial.println("HTTP server started");

  for(;;){
     server.handleClient();
     delay(1); 
     //Serial.println("webrun");
  }
}


void valloxTaskMain(void * pvParameters){
  char c;
  String readString;
  uint16_t received = 0;
  bool packet_start = false;
  uint8_t packet_index = 0; //index to current position inside packet

  ValloxPacket packet;

  for(;;){
    while (Serial2.available()) {
        c = Serial2.read();
        if (c == 0x01 && packet_start == false){
          //Serial.println("Start packet found");
          packet_start = true;
          packet.system = c;
          packet_index += 1;
        }
        else if(packet_start == true){
          switch (packet_index){
            case 1:{
              packet.sender = c;
              packet_index ++;
              break;
            }
            case 2:{
              packet.receiver = c;
              packet_index ++;
              break;
            }
            case 3:{
              packet.variable = c;
              packet_index ++;
              break;
            }
            case 4:{
              packet.vardata = c;
              packet_index ++;
              break;
            }
            case 5:{
              packet.checksum = c;
              packet_start = false;
              packet_index = 0;
              //Serial.println("Validating packet end");
              if (validateValloxPacket(&packet) == true){
                parsePacket(&packet);
                total_packets ++;
              }
              else{
                Serial.println("Invalid packet received");
                total_invalid_packets ++;
              }
              valloxtask_stack = uxTaskGetStackHighWaterMark(NULL);
              break;
            }
          }
        }
        else{
          // Waiting packet start condition
        }
    }
    delayMicroseconds(10); 
  }
}


void valloxTaskMonitor(void * pvParameters){
  //int64_t tick = esp_timer_get_time();
  uint32_t last_count = total_packets-1;
  delay(10000);
  for(;;){
    if (total_packets > last_count){
      last_count = total_packets;
    }
    else{
      stuck_time ++;
      last_count = total_packets;
      if (stuck_time > 1){
         // Delete vallox task if it got stuck
         vTaskDelete(valloxTask);

         // Create new vallox task
         Serial.println("Starting Vallox task");
         xTaskCreatePinnedToCore(
                          valloxTaskMain,   /* Task function. */
                          "valloxTask",     /* name of task. */
                          2000,       /* Stack size of task */
                          NULL,        /* parameter of the task */
                          1,           /* priority of the task */
                          &valloxTask,      /* Task handle to keep track of created task */
                          1);          /* pin task to core 0 */                  
         delay(1000); 
         stuck_time = 0;
         vallox_restarts ++;
      }
    }
    delay(10000);
  }
  
}


void otaTaskMain(void * pvParameters){
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("OTA task started");
  
  for(;;){
     ArduinoOTA.handle();
     otatask_stack = uxTaskGetStackHighWaterMark(NULL);
     delay(100);
  }
}

void wlanWatchdogTaskMain(void * pvParameters){
  for(;;){
    digitalWrite(LED, !digitalRead(LED));
    delay(1000);
    wlantask_stack = uxTaskGetStackHighWaterMark(NULL);
    if (WiFi.waitForConnectResult() != WL_CONNECTED){
      Serial.println("WLAN is disconnected, reconnecting...");
      WiFi.mode(WIFI_STA);
      WiFi.setTxPower(WIFI_POWER_19_5dBm);
      WiFi.begin(ssid, password);
      while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(1000);
        ESP.restart();
      }
    }
  }
}

/*
void KPIloggerTaskMain(void * pvParameters){
  for(;;){
    ValloxKPIEntry entry;
    entry.log_position = ticker;
    entry.fresh_air_temp = read_global_variable("outside_temp");
    entry.dirty_air_temp = read_global_variable("dirty_air_temp");
    entry.incoming_air_temp = read_global_variable("temp_in");
    entry.outgoing_air_temp = read_global_variable("temp_out");
    entry.heater_on = read_global_variable("heater_percent");
    entry.fan_speed = read_global_variable("fresh_air_temp");
    entry.humidity = g_humidity;
    entry.bathroom_temp = g_temperature;   
    KPItick(&entry);
    delay(999);
    ticker ++;
  }
}*/

void rhMonitorMainTask(void * pvParameters){      
  // Get pointers to control variables
  IntParam *fan_boost_time = get_control_param_pointer(String("fan_boost_time"));
  IntParam *fan_boost_speed = get_control_param_pointer(String("fan_boost_speed"));
  IntParam *rh_boost_start = get_control_param_pointer(String("rh_boost_start"));
  IntParam *rh_boost_stop = get_control_param_pointer(String("rh_boost_stop"));
  IntParam *rh_hysteresis = get_control_param_pointer(String("rh_hysteresis"));
  IntParam *control_enabled = get_control_param_pointer(String("control_enabled"));
  IntParam *fan_base_speed = get_control_param_pointer(String("fan_base_speed"));
  IntParam *fan_base_speed_winter = get_control_param_pointer(String("fan_base_speed_winter"));
  IntParam *fan_boost_speed_winter = get_control_param_pointer(String("fan_boost_speed_winter"));
  IntParam *penalty_timer = get_control_param_pointer(String("penalty_timer"));
  IntParam *fan_min_boost_time = get_control_param_pointer(String("fan_min_boost_time"));

  bool average_ok = false;
  uint8_t num_of_average_samples = 0;
  uint8_t every_n_sample = 0;
  for(;;){
    // Update global variables
    float t_temp = read_temperature();
    if (t_temp > -50.0){
      g_temperature = t_temp;
    }
    g_humidity = read_humidity();

    // collect average samples before do any decissions
    if (average_ok == false){
      //if (num_of_average_samples == 0){ //Init humidity in case first sample
      
      g_avg_humidity = float(g_humidity);
      
      //}
      //g_avg_humidity = float(g_avg_humidity*float(AVERAGE_SAMPLES) + float(g_humidity))/float(AVERAGE_SAMPLES+1.0);

      //if (num_of_average_samples == AVERAGE_SAMPLES){
      average_ok = true;
      //}
    }
    else{
      // capture every nth sample to slow down average even more
      if (every_n_sample == 2){
        g_avg_humidity = float(g_avg_humidity*float(AVERAGE_SAMPLES) + float(g_humidity))/float(AVERAGE_SAMPLES+1.0);
        every_n_sample = 0;
      }
      every_n_sample ++;
    }

    float outside_temp = read_global_variable_float("outside_temp");
    if (outside_temp < 3.0){
      winter_mode = true;
    }
    else{
      winter_mode = false;
    }
      
    //Check master switch status
    if (control_enabled->data == 1 && average_ok == true){
      Serial.println("Humidity:"+String(g_humidity)+" start: "+String(rh_boost_start->data)+" stop:"+String(rh_boost_stop->data));
      if (cooldown_active == false){

        uint8_t set_boost_speed = fan_boost_speed->data;
        if (winter_mode == true){
          set_boost_speed = fan_boost_speed_winter->data;
        }

        // If current humidity has been increased over 10% over last average period, start boosting
        if (g_humidity > g_avg_humidity + 12 && boost_active == false){
          Serial.println("Boosting started, avg diff");
          boost_active = true;  
          setFanSpeed(set_boost_speed);
          delay(3000);
          //Send commands two times
          setFanSpeed(set_boost_speed);
          g_before_boost = g_avg_humidity; //Capture averaged value to memory
        }
              
        // Trigger boosting if over set threshold // This is absolute value and always trigger is it more humid than this
        if (g_humidity > rh_boost_start->data && boost_active == false){
          Serial.println("Boosting started");
          boost_active = true;  
          setFanSpeed(set_boost_speed);
          delay(3000);
          //Send commands two times
          setFanSpeed(set_boost_speed);
          g_before_boost = g_avg_humidity; //Capture averaged value to memory
        }

        // When boost mode is active check rules to deactivate it
        if (boost_active == true){
          boost_time ++; //Increment boost timer
          if (boost_time%10 == 0){
            Serial.println("boost_time:"+String(boost_time)+" penalty_time:"+String((uint16_t)penalty_timer->data*60));
          }
          
          // Shutdown boosting if it has lasted over fan boost time or humidity is below threshold
          // Boosting can be ended if it has been lasted over minimum boost time
          if (((boost_time >= (uint16_t)fan_boost_time->data*60 || g_humidity < rh_boost_stop->data) && boost_time > (uint16_t)fan_min_boost_time->data*60)
                || ((boost_time >= (uint16_t)fan_boost_time->data*60 || g_humidity <= g_before_boost + 8.0) && boost_time > (uint16_t)fan_min_boost_time->data*60)){
            Serial.println("Boosting end");
            
            //Default fan speed
            uint8_t fspeed = fan_base_speed->data;

            //If outside temperature is below 3 degrees, set winter fan speed
            if (winter_mode == true){
              fspeed = fan_base_speed_winter->data;
            }
            setFanSpeed(fspeed);
            delay(3000);
            setFanSpeed(fspeed);
            cooldown_active = true;
            boost_active = false;
            boost_time = 0;
            Serial.println("Cooldown activated");
          }
        }
      }
      else{
        cooldown_time ++; // Increase cooldown timer when true
        if (cooldown_time%10 == 0){
            Serial.println("cooldown_time:"+String(cooldown_time));
          }
        if (cooldown_time >= (uint16_t)penalty_timer->data*60){
          cooldown_time = 0;
          cooldown_active = false;
          Serial.println("Cooldown End");
        }
      }
    }

    delay(1000);
    if (read_global_variable("fan_speed") >= 6){
      boost_active = true;
    }
    rhtask_stack = uxTaskGetStackHighWaterMark(NULL);
  }
}


void setup(void) {
  pinMode(LED, OUTPUT);
  EEPROM.begin(EEPROM_SIZE);
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  

  Serial.println("Starting WebServer task");
  xTaskCreatePinnedToCore(
                    webTaskMain,   /* Task function. */
                    "webServer",     /* name of task. */
                    4000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &webTask,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  Serial.println("Starting Vallox task");
  xTaskCreatePinnedToCore(
                    valloxTaskMain,   /* Task function. */
                    "valloxTask",     /* name of task. */
                    2000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &valloxTask,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */                  
  delay(500); 

  Serial.println("Starting OTA task");
  xTaskCreatePinnedToCore(
                    otaTaskMain,   /* Task function. */
                    "otaTask",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &otaTask,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */        
  delay(500); 

  Serial.println("Starting Vallox monitor");
  xTaskCreatePinnedToCore(
                    valloxTaskMonitor,   /* Task function. */
                    "valloxTaskMon",     /* name of task. */
                    1000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &valloxTaskMon,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */                  
  delay(500); 
                    
  Serial.println("Starting WLAN monitor task");
  xTaskCreatePinnedToCore(
                    wlanWatchdogTaskMain,   /* Task function. */
                    "wlanWatchdogTask",     /* name of task. */
                    1000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &wlanWatchdogTask,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */        

  delay(500); 
/*                    
  Serial.println("Starting KPI logger task");
  xTaskCreatePinnedToCore(
                    KPIloggerTaskMain,  
                    "KPIloggerTask",    
                    1000,
                    NULL,
                    1,
                    &KPIloggerTask,
                    1);

  delay(500); 
*/                    
  Serial.println("Starting RH monitor task");
  xTaskCreatePinnedToCore(
                    rhMonitorMainTask,   /* Task function. */
                    "RHmonitor",     /* name of task. */
                    1500,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &RHmonitorTask,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */
   
  Serial.println("All tasks started");
}

void loop(void) {
  delay(1);
}
