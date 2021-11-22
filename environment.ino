
#include <OneWire.h>
#include <DallasTemperature.h>

// Humidity sensor low pass filter on PCB 30k / 3.8nF => 1400Hz
// Humidity sensor model: Honeywell HIH-4030/31 series
#define HUMIDITY_SENSE_PIN 34 //Humidity measurement pin
#define VOLTAGE_DIVIDER 2.0 //there is 30K/30K dividers => 1:2 voltage on port
#define HUMIDITY_0 650 //Sensor voltage when 0% RH
#define HUMIDITY_100 3900 //Sensor voltage when 100% RH

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;   


OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);


float read_temperature(void){
  sensors.requestTemperatures(); 
  float t = sensors.getTempCByIndex(0); 
  return t;
}

uint8_t read_humidity(void){
  float portValue = analogRead(HUMIDITY_SENSE_PIN);
  uint16_t voltage = uint16_t((3300.0/4096.0*portValue)*VOLTAGE_DIVIDER);
  voltage = (voltage + uint16_t((3300.0/4096.0*portValue)*VOLTAGE_DIVIDER))/2;
  if (voltage < HUMIDITY_0){
    Serial.println("Humidity voltage below min value:"+String(voltage));
    voltage = HUMIDITY_0;
  }
  g_humidity = map(voltage, HUMIDITY_0, HUMIDITY_100, 0, 100);
  
  return g_humidity;
}
