#include <UIPEthernet.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <SPI.h>
#include "Debouncer.h"

#define receiverPin 4
#define statusGnd 7
#define statusPin 8
#define CLIENT_ID "433MHzGateway"

byte mac[6] = {0xAE, 0xB2, 0x26, 0xE4, 0x4A, 0x5C}; 

void rxCallback(const char*);

void(* resetFunc) (void) = 0;

Debouncer debouncer = Debouncer(rxCallback, 1000);
RCSwitch receiver = RCSwitch();
EthernetClient ethClient;
PubSubClient mqttClient;
IPAddress mqttServer(192, 168,  1, 110);

void setup() {
  Serial.begin(9600);
  // enable 433mHz receiver power pin  
  pinMode(receiverPin, OUTPUT);
  digitalWrite(receiverPin, HIGH);
  // setup connection status led
  pinMode(statusGnd, OUTPUT);
  pinMode(statusPin, OUTPUT);
  digitalWrite(statusGnd, LOW);
  digitalWrite(statusPin, LOW);
  // receiver on interrupt 1 => that is pin #3
  receiver.enableReceive(1);  
  // setup ethernet communication using DHCP
  if(Ethernet.begin(mac) == 0) {
    Serial.println(F("Ethernet configuration using DHCP failed"));
    Serial.println(F("restarting..."));
    
    resetFunc(); 
  }

  mqttClient.setClient(ethClient);
  mqttClient.setServer(mqttServer,1883);
  
  Serial.println(Ethernet.localIP());
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
    
    Serial.println(F("MQTT client connected"));
  }
    
  if (receiver.available()) {
    debouncer.click(receiver.getReceivedValue());
    receiver.resetAvailable();
  }

  mqttClient.loop();
}

void reconnect() {
  digitalWrite(statusPin, LOW);
  // Loop until we're reconnected
  for (int n=0;!mqttClient.connected()&&n<10;n++) {
    // will blink until the connection is established
    digitalWrite(statusPin, HIGH);
    // Attempt to connect (takes ~500ms)
    if (mqttClient.connect(CLIENT_ID)) {      
      return;
    }
    // blink while connecting
    digitalWrite(statusPin, LOW);
    delay(500);
  }

  resetFunc(); 
}

void rxCallback(const char* id) {
  char topic[32];
  sprintf(topic, "/uhf-433/%s", id); 

  Serial.print(F("Send data to topic:"));
  Serial.println(topic);
     
  if (!mqttClient.publish(topic, "TRIG")) {
     Serial.println(F("Failed to publish"));
  }
}
