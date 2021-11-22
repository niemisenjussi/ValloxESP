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
- `[GET] /vallox` all Vallox protocol variables which were intercepted + RTOS task data
- `[GET] /measure` Humidity, external temperature data
- `[GET] /setfan/<fanspeed>` Method for setting FAN speed. This is GET request so it can be done using browser
- `[GET] /getparam/<variable>` Query for asking externally controller parameters
- `[GET] /setparam/<variable>/<newvalue>` Setter for parameters

Externally controlled parameters (default parameter range):
- `fan_boost_time` Maximum boost time in minutes, min=0, max=255, default=60
- `fan_boost_speed` Fan boost speed when boosting is active, min=3, max=8, default=4
- `fan_min_boost_time` Minimum boost time in minutes, min=0, max=255, default=10
- `rh_boost_start` Relative humidity when boosting is started, min=10%, max=100%, default=40%
- `rh_boost_stop` Relative humidity when boosting is stopped, min=10%, max=100%, default=30%
- `rh_hysteresis` Relative humidity hysteresis, min=0, max=20, default=5
- `control_enabled` Global flag if ESP controls fan speed min=0, max=1, default=1
- `fan_base_speed` Fan base speed, min=3, max=8, default=4
- `penalty_timer` Time waited after boosting ends in minutes min=0, max=255, default=15
- `fan_base_speed_winter` Fan speed in winter conditions, min=2, max=8, default=3
- `fan_boost_speed_winter` Fan boost speed in winter conditions, min=2, max=8, default=5

Example `/setparam/fan_boost_speed_winter/6` will change winter boost speed to 6.


# Example queries
- Asking current environment information
`/measure`
```
{ 
    "temperature":23.94,
    "humidity":24,
    "before_boost_humidity":18.14,
    "avg_humidity":25.58
}
```

- Reading all Vallox variables:
`/vallox`

```
{
  "41":{"desc":"fan_speed","value":3.00},
  "42":{"desc":"max_rh","value":-6.37},
  "46":{"desc":"ma_volt","value":0.00},
  "50":{"desc":"outside_temp","value":-4.40},
  "51":{"desc":"dirty_air_temp","value":8.00},
  "52":{"desc":"temp_out","value":22.40},
  "53":{"desc":"temp_in","value":18.70},
  "54":{"desc":"error_code","value":0.00},
  "85":{"desc":"heater_on_percent","value":0.00},
  "86":{"desc":"heater_off_percent","value":0.00},
  "87":{"desc":"target_temp","value":0.00},
  "109":{"desc":"flags2","value":0.00},
  "111":{"desc":"flags4","value":0.00},
  "112":{"desc":"flags5","value":0.00},
  "113":{"desc":"flags6","value":0.00},
  "121":{"desc":"fireplace_boost_timer","value":0.00},
  "163":{"desc":"lamps","value":9.00},
  "164":{"desc":"after_heater_set_temp","value":0.00},
  "165":{"desc":"max_fan_speed","value":0.00},
  "166":{"desc":"service_interval","value":0.00},
  "167":{"desc":"preheater_set_temp","value":0.00},
  "168":{"desc":"incoming_air_stop_temp","value":0.00},
  "169":{"desc":"base_fan_speed","value":0.00},
  "170":{"desc":"time_to_service","value":0.00},
  "175":{"desc":"passthrough_temp","value":0.00},
  "176":{"desc":"dc_fan_in_set_speed","value":0.00},
  "177":{"desc":"dc_fan_out_set_speed","value":0.00},
  "178":{"desc":"ice_protection_temp_hyst","value":0.00},
  "241":{"desc":"power_button","value":1.00},
  "242":{"desc":"CO2_button","value":0.00},
  "243":{"desc":"RH_button","value":0.00},
  "244":{"desc":"heater_button","value":1.00},
  "245":{"desc":"filter_led","value":0.00},
  "246":{"desc":"heater_led","value":0.00},
  "247":{"desc":"error_led","value":0.00},
  "248":{"desc":"service_led","value":0.00},
  "total_packets":13857,
  "total_invalid_packets":29,
  "humidity":24,
  "before_boost_humidity":18.14,
  "avg_humidity":25.35,
  "webtask":1584,
  "valloxtask":1420,
  "otatask":8652,
  "bathroom_temp":23.94,
  "wlantask":264,
  "rhtask":108,
  "stuck_time":0,
  "vallox_restarts":1,
  "boost_time":0,
  "outside_temp":-4.40,
  "cooldown_time":0,
  "boost_active":false,
  "cooldown_active":false,
  "winter_mode":true
}
```
