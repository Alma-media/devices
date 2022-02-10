#include <Ethernet.h>
#include <EthernetBonjour.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <SPI.h>
#include "Debouncer.h"
#include "Substring.h"

#define SERVER_NAME "PUT-SERVER-NAME-HERE"
#define CLIENT_ID "UHFGateway-00000000"
#define receiverInt 1 // receiver on interrupt 1 => that is pin #3
#define receiverVcc 4
#define statusLedGnd 5
#define statusLedVcc 6
#define transmitterGnd 7
#define transmitterVcc 8
#define transmitterPin 9
#define rxTopic "/uhf-433-rx/00000000-0000-0000-0000-000000000000/"
#define txTopic "/uhf-433-tx/00000000-0000-0000-0000-000000000000/+"

byte mac[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }; 

void rxCallback(const char*);
void txCallback(char*, byte*, unsigned int);
void(*resetFunc) (void) = 0;

Debouncer debouncer = Debouncer(rxCallback, 1000);
RCSwitch transceiver = RCSwitch();
IPAddress serverAddr;
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

uint32_t to_uint32_t(char* str){
  uint32_t res = 0;
  for (int i = 0; str[i] != '\0'; ++i)
    res = res*10 + str[i] - '0';
  return res;
}

const char* ip_to_str(const uint8_t* ipAddr) {
  static char buf[16];
  sprintf(buf, "%d.%d.%d.%d\0", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
  return buf;
}

void nameFound(const char* name, const byte ipAddr[4]) {
  if (NULL != ipAddr) {
    Serial.println(ip_to_str(ipAddr));
    serverAddr = IPAddress(ipAddr);
    return;
  }
  resetFunc();
}

void resolveIP() {
  Ethernet.begin(mac); 
  EthernetBonjour.begin(CLIENT_ID);
  EthernetBonjour.setNameResolvedCallback(nameFound);
  while (NULL == serverAddr) {
    EthernetBonjour.resolveName(SERVER_NAME, 5000);
    while (EthernetBonjour.isResolvingName()) {
      EthernetBonjour.run();
    }
  }
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
  char topic[64];
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

void initPorts() {
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
  pinMode(statusLedGnd, OUTPUT);
  pinMode(statusLedVcc, OUTPUT);
  digitalWrite(statusLedGnd, LOW);
  digitalWrite(statusLedVcc, LOW);
}

void setup() {
  Serial.begin(9600);
  initPorts();
  // setup network
  resolveIP();
  mqttClient.setServer(serverAddr, 1883);
  mqttClient.setCallback(txCallback);
}

void loop() {
  if (!mqttClient.connected()) {
    resolveIP();                            // resolve IP address in case MQTT server has been restarted and got new IP
    mqttClient.setServer(serverAddr, 1883); // and assign a new IP
    reconnect();                            // reconnect to broker
  }
  if (transceiver.available()) {
    debouncer.click(transceiver.getReceivedValue());
    transceiver.resetAvailable();
  }
  mqttClient.loop();
  EthernetBonjour.run();
}