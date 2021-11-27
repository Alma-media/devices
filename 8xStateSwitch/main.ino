#include <Ethernet.h>
#include <EthernetBonjour.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>

#define SERVER_NAME "PUT-SERVER-NAME-HERE"
#define CLIENT_ID "8xSwitch-00000000"
#define PREFIX "00000000-0000-0000-0000-00000000000"
#define OFF 0b00000000

int swState = OFF;
int ledState = OFF;
// you can find this written on the board of some Arduino Ethernets or shields
byte mac[] = { 0xDE, 0xAD, 0xBC, 0xEF, 0xFE, 0xED }; 

IPAddress serverAddr;
EthernetClient ethClient;
PubSubClient client(ethClient);

byte readA() {
  Wire.beginTransmission(0x20);
  Wire.write(0x12);           // address PORT A
  Wire.endTransmission();
  Wire.requestFrom(0x20, 1);  // request one byte of data
  return Wire.read();         // store incoming byte into "input"
}

void writeB(byte state) {
  Wire.beginTransmission(0x20);
  Wire.write(0x13);           // address PORT B
  Wire.write(state);          // write state
  Wire.endTransmission();
}

void registerCallbacks() {
  for (int i=0;i<=7;i++) {
    char topic[43];
    sprintf(topic, "/led/%s%d", PREFIX, (byte)i);
    Serial.println(topic);
    // subscribe relays to their topics
    client.subscribe(topic);
    // register relays to the orchestrator
    client.publish("/ready/%s%d", PREFIX, (byte)i);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[");
  Serial.print(topic);
  Serial.print("] ");

  char command[length];
  for (int i=0;i<length;i++) {
    command[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strlen(command) >= 2 && command[0] == 'O' && command[1] == 'N') {
    Serial.println("SWITCH ON");
    if (bitRead(ledState, (topic[strlen(topic)-1]-'0')) == 1) {
      return;
    }
    ledState = ledState^1<<(topic[strlen(topic)-1]-'0');
    writeB((byte)ledState);
  }

  if (strlen(command) >= 3 && command[0] == 'O' && command[1] == 'F' && command[2] == 'F') {
    Serial.println("SWITCH OFF");
    if (bitRead(ledState, (topic[strlen(topic)-1]-'0')) == 0) {
      return;
    }
    ledState = ledState^1<<(topic[strlen(topic)-1]-'0');
    writeB((byte)ledState);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      // ... and resubscribe
      registerCallbacks();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void nameFound(const char* name, const byte ipAddr[4]) {
  if (NULL != ipAddr) {
    Serial.println(ip_to_str(ipAddr));
    serverAddr = IPAddress(ipAddr);
    return;
  }
  Serial.println(F("Resolving timed out."));
}

const char* ip_to_str(const uint8_t* ipAddr) {
  static char buf[16];
  return buf;
}

void setup () {
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(0x20);
  Wire.write(0x01);
  Wire.write(0x00); // set entire PORT B to output
  Wire.endTransmission();

  writeB((byte)OFF);

  Ethernet.begin(mac); 
  EthernetBonjour.begin(CLIENT_ID);
  EthernetBonjour.setNameResolvedCallback(nameFound);

  Serial.println(F("Resolving broker IP ..."));

  while (NULL == serverAddr) {
    EthernetBonjour.resolveName(SERVER_NAME, 5000);
    while (EthernetBonjour.isResolvingName()) {
      EthernetBonjour.run();
    }
  }
  client.setServer(serverAddr, 1883);
  client.setCallback(callback);
}

byte inputs=0;

void loop () {
  inputs = readA();
 
  for (int i=0;i<8;i++) {
    if (bitRead(inputs, i) != 0 && bitRead(inputs, i) != bitRead(swState, i)) {
      Serial.print("switch state: ");
      Serial.println(i);
      char topic[43];
      sprintf(topic, "/switch/%s%d", PREFIX, (byte)i);
      client.publish(topic, "TRIG");
    }
  }

  swState = inputs;
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  EthernetBonjour.run();
}
