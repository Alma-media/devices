# 4 x Relay module

## Hardware
- Arduino Nano
- 4 x Arduino Relay Module
- W5500 Ethernet Shield

## Libraries
- SPI + Ethernet (Required by W5500)
- EthernetBonjour (MDNS)
- PubSubClient (MQTT)

## Commands
In order to control the device use one of the 4 topics (UUID must be provided while uploading the sketch).
### Topics
`/relay/<UUID>`
### Payload
- switch on
```
ON
```
- switch off
```
OFF
``` 
## Configuration
- unique MAC address
- unique device ID (PREFIX and CLIENT_ID)
- name of the server (same for all smart devices in the private network)

![4 x Relay module](./4xRelayModule.jpg)