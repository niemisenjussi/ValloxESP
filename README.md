# ValloxESP
Vallox Digit 2 SE (2010) -ventilation machine protocol reader. Vallox uses RS-485 bus in communication between ventilation machine and control panel. Connection needs 4 wires A/B + 19VDC connection. ESP32 can be connected to in parallel with controller. 

Control panel will make query to machine every few seconds which returns information back to control panel. This arduino code will intercept that communication and collect all well known packets from ventilation machine to control panel.

## Features
- Collect all communication packets between controller and ventilation machine
- Parse all temperature and fan speed information
- Parse alarm flags
- Control Fan speed (1-8) using REST API
- Monitor pre-heater and after-heater usage
- Support for external 1-wire DS18B20 sensor
- Honeywell HIH-4030/31 humidity sensor support (Or any other linear model)

## API
```[GET] /vallox``` all Vallox protocol variables which were intercepted + RTOS task data
```[GET] /measure``` Humidity, external temperature data
```[GET] /setfan/<fanspeed>``` Method for setting FAN speed. This is GET request so it can be done using browser
```[GET] /getparam/<variable>``` Query for asking externally controller parameters
```[GET] /setparam/<variable>/<newvalue>``` Setter for parameters

Externally controlled parameters (default parameter range):
```
fan_boost_time //Maximum boost time in minutes, min=0, max=255, default=60
fan_boost_speed //Fan boost speed when boosting is active, min=3, max=8, default=4
fan_min_boost_time //Minimum boost time in minutes, min=0, max=255, default=10
rh_boost_start //Relative humidity when boosting is started, min=10%, max=100%, default=40%
rh_boost_stop //Relative humidity when boosting is stopped, min=10%, max=100%, default=30%
rh_hysteresis //Relative humidity hysteresis, min=0, max=20, default=5
control_enabled //Global flag if ESP controls fan speed min=0, max=1, default=1
fan_base_speed //Fan base speed, min=3, max=8, default=4
penalty_timer //Time waited after boosting ends in minutes min=0, max=255, default=15
fan_base_speed_winter //Fan speed in winter conditions, min=2, max=8, default=3
fan_boost_speed_winter //Fan boost speed in winter conditions, min=2, max=8, default=5
```


