#include <Ethernet.h>
#include <EthernetBonjour.h>
#include <PubSubClient.h>
#include <SPI.h>

#define OFF 0b1111;
#define CLIENT_ID "4xRelayModule-00000000"
#define PREFIX "00000000-0000-0000-0000-000000000000"
#define SERVER_NAME "PUT-SERVER-NAME-HERE"

int state = OFF;
// define MAC-address here (must be unique for every device)
byte mac[] = { 0xCE, 0xAD, 0xCE, 0xEF, 0xFE, 0xED }; 

IPAddress serverAddr;
EthernetClient ethClient;
PubSubClient client(ethClient);

void writeState(int newState) {
  for (int i=0;i<4;i++){
    if (bitRead(state, i) == bitRead(newState, i)) {
      continue;
    }
    digitalWrite(i+3, bitRead(newState, i));
  }
  state = newState;
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
    if (bitRead(state, (topic[strlen(topic)-1]-'0')) == 0) {
      return;
    }
    writeState(state^1<<(topic[strlen(topic)-1]-'0'));
  }

  if (strlen(command) >= 3 && command[0] == 'O' && command[1] == 'F' && command[2] == 'F') {
    Serial.println("SWITCH OFF");
    if (bitRead(state, (topic[strlen(topic)-1]-'0')) == 1) {
      return;
    } 
    writeState(state^1<<(topic[strlen(topic)-1]-'0'));
  }
}

void setup()
{  
  Serial.begin(9600);

  // RELAYS
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  // connection status LED
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  // switch everything OFF on startup 
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  // status LED 
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);

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

void registerCallbacks() {
  for (int i=0;i<=3;i++) {
    char topic[43];
    sprintf(topic, "/relay/%s", channel((byte)i));
    Serial.println(topic);
    // subscribe relays to their topics
    client.subscribe(topic);
    // register relays to the orchestrator
    client.publish("/ready", channel((byte)i));
  }
}

void reconnect() {
  // disable status LED since the connection is lost
  digitalWrite(7, LOW);
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
  // enable status LED when connected to the broker
  digitalWrite(7, HIGH);
}

const char* ip_to_str(const uint8_t* ipAddr) {
  static char buf[16];
  sprintf(buf, "%d.%d.%d.%d\0", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
  return buf;
}

const char* channel(byte num){
  static char buf[36];
  sprintf(buf,"%s%i",PREFIX,num);
  return buf;
}

void nameFound(const char* name, const byte ipAddr[4]) {
  if (NULL != ipAddr) {
    Serial.println(ip_to_str(ipAddr));
    serverAddr = IPAddress(ipAddr);
    return;
  }
  Serial.println(F("Resolving timed out."));
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  EthernetBonjour.run();
}