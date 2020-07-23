# esp32-temp-monitor

An ESP32 based temperature monitoring, regulation and alerting system. Configurable via a web interface, with statistics and metric logging.

Designed so that I could monitor the temperature of a mini fridge, shipping metrics back to a server, and turning the fridge on and off via a relay to ensure a precise constant temperature is maintained.  I am using this when hibernating tortoises. 

I plan to use this to monitor and control a mini fridge used to hibernate tortoises, but this could be used for other purposes like:
- monitoring a green house and using it to open and close shutters
- rather than using a temperature sensor, use an air quality sensor and control the opening and closing of window, or turning on or off an air purifier
- monitoring your house temperature and controlling central heating

Work is in progress

## Components
- ESP32
- Relay
- LCD Screen
- Multiple DS18B20 Temperature probes
- LED

## Features
- LCD Screen to display current temperature
- Web server to allow remote monitoring, configuration and control
- Editing and editing of configuration via webpage, api and uploaded file
- OTA Updates of firmware
- Metrics shipping via TICK stack
- Logging via syslog
- Warning LED
- Remote control via webapi to allow integration with automation

## Possible Further Features
- To open and close fridge door
- To control a fan to control air circulation
- Integration with Amazon Alexa and Google Home, to allow you to monitor the status, or even have it initiate commands to other devices managed by Alexa or Google Home
- Integation with IFTT
- Integration with pushover
- Integration with twitter
- Integration with twilio
- Integration with users own webhooks
