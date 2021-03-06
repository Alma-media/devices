#include <Ethernet.h>
#include <EthernetBonjour.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>

#define OFF 0b11111111
#define CLIENT_ID "8xRelayModule-00000000"
#define SERVER_NAME "PUT-SERVER-NAME-HERE"
#define PREFIX "00000000-0000-0000-0000-000000000000"

void(* resetFunc) (void) = 0;
// initial state (everything is OFF)
int state = OFF;
// define MAC-address here (must be unique for every device)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 

IPAddress serverAddr;
EthernetClient ethClient;
PubSubClient client(ethClient);

void setPort(byte a) {
  Wire.beginTransmission(0x20);
  Wire.write(0x12);
  Wire.write(a);
  Wire.endTransmission();
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
    state = state^1<<(topic[strlen(topic)-1]-'0');
    setPort((byte)state);
  }

  if (strlen(command) >= 3 && command[0] == 'O' && command[1] == 'F' && command[2] == 'F') {
    Serial.println("SWITCH OFF");
    if (bitRead(state, (topic[strlen(topic)-1]-'0')) == 1) {
      return;
    }
    state = state^1<<(topic[strlen(topic)-1]-'0');
    setPort((byte)state);
  }
}

void registerCallbacks() {
  for (int i=0;i<=7;i++) {
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
  // status LED to LOW
  digitalWrite(7, LOW);
  // Loop until we're reconnected
  for (int n=0;!client.connected()&&n<5;n++) {
    Serial.println(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      registerCallbacks();
      // status LED to HIGH
      digitalWrite(7, HIGH);
      break;
    }
    
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in a second");
    delay(1000);
  }
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

void setup() {
  Serial.begin(9600);
  // connection status led
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  // switch off status led
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  // init MCP
  Wire.begin();
  Wire.beginTransmission(0x20);
  Wire.write(0x00); 
  Wire.write(0x00); 
  Wire.endTransmission();
  // switch off all relays
  setPort((byte)OFF);
  resolveIP();
  client.setServer(serverAddr, 1883);
  client.setCallback(callback);
}

void nameFound(const char* name, const byte ipAddr[4]) {
  if (NULL != ipAddr) {
    Serial.println(ip_to_str(ipAddr));
    serverAddr = IPAddress(ipAddr);
    return;
  }
  Serial.println("cannot resolve server name. rebooting...");
  delay(1000);
  // reboot when timed out
  resetFunc(); 
}

void loop() {
  if (!client.connected()) {
    resolveIP();                        // resolve IP address in case MQTT server has been restarted and got new IP
    client.setServer(serverAddr, 1883); // and assign a new IP
    reconnect();                        // reconnect to broker
  }
  client.loop();
  EthernetBonjour.run();
}