#include <UIPEthernet.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <SPI.h>
#include "Debouncer.h"
#include "Substring.h"

#define transmitterGnd 8
#define transmitterVcc 9
#define transmitterPin 10
#define receiverInt 1 // receiver on interrupt 1 => that is pin #3
#define receiverVcc 4
#define statusLedVcc 13
#define CLIENT_ID "433MHzGateway"
#define rxTopic "/uhf-433/rx/"
#define txTopic "/uhf-433/tx/+"

byte mac[6] = {0xAE, 0xB2, 0x26, 0xE4, 0x4A, 0x5C}; 

void rxCallback(const char*);
void txCallback(char*, byte*, unsigned int);
void(*resetFunc) (void) = 0;

Debouncer debouncer = Debouncer(rxCallback, 1000);
RCSwitch transceiver = RCSwitch();
EthernetClient ethClient;
PubSubClient mqttClient;
IPAddress mqttServer(192, 168,  1, 110);

void setup() {
  Serial.begin(9600);
  // configure 433mHz transceiver
  pinMode(receiverVcc, OUTPUT);
  pinMode(transmitterGnd, OUTPUT);
  pinMode(transmitterVcc, OUTPUT);
  digitalWrite(transmitterGnd, LOW);
  digitalWrite(transmitterVcc, LOW);
  digitalWrite(receiverVcc, HIGH);
  transceiver.enableReceive(receiverInt);
  transceiver.enableTransmit(transmitterPin);
  // setup connection status led
  pinMode(statusLedVcc, OUTPUT);
  digitalWrite(statusLedVcc, LOW);
  // setup ethernet communication using DHCP
  if(Ethernet.begin(mac) == 0) {
    Serial.println(F("Ethernet configuration using DHCP failed"));
    Serial.println(F("restarting..."));
    resetFunc(); 
  }
  mqttClient.setClient(ethClient);
  mqttClient.setServer(mqttServer,1883);
  mqttClient.setCallback(txCallback);
  Serial.println(Ethernet.localIP());
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
    Serial.println(F("MQTT client connected"));
  }
  if (transceiver.available()) {
    debouncer.click(transceiver.getReceivedValue());
    transceiver.resetAvailable();
  }
  mqttClient.loop();
}

void reconnect() {
  digitalWrite(statusLedVcc, LOW);
  // Loop until we're reconnected
  for (int n=0;!mqttClient.connected()&&n<10;n++) {
    // will blink until the connection is established
    digitalWrite(statusLedVcc, HIGH);
    // Attempt to connect (takes ~500ms)
    if (mqttClient.connect(CLIENT_ID)) {
      // subscribe to transmitter topic
      mqttClient.subscribe(txTopic);
      return;
    }
    // blink while connecting
    digitalWrite(statusLedVcc, LOW);
    delay(500);
  }
  resetFunc(); 
}

void rxCallback(const char* id) {
  char topic[32];
  sprintf(topic, "%s%s", rxTopic, id); 
  Serial.print(F("Send data to topic:"));
  Serial.println(topic);
  if (!mqttClient.publish(topic, "TRIG")) {
     Serial.println(F("Failed to publish"));
  }
}

void txCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Substring substring = Substring(strlen(txTopic) - 1, strlen(topic));
  uint32_t value = to_uint32_t(substring.extract(topic));
  // switch off the receiver and enable transmitter
  digitalWrite(receiverVcc, LOW);
  digitalWrite(transmitterVcc, HIGH);
  // send the value obtained from topic name
  transceiver.send(value, 24);
  // switch off the transmitter and enable receiver
  digitalWrite(transmitterVcc, LOW);
  digitalWrite(receiverVcc, HIGH);
}

uint32_t to_uint32_t(char* str){
    uint32_t res = 0;
    for (int i = 0; str[i] != '\0'; ++i)
        res = res*10 + str[i] - '0';
    return res;
}
