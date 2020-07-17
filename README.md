# esp32-temp-monitor

A Temperature monitor and regulation system.

Designed so that I could monitor the temperature of a miniture fridge, shipping metrics back to a server, and turning the fridge on and off via a relay to ensure a precise constant temperature is maintained.  I am using this when hibernating tortoises.

Currently work is in progress

## Components
- ESP32
- Relay
- LCD Screen
- Temperature probe
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
- Integration with Amazon Alexa and Google Home
