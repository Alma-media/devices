#include <Ethernet.h>
#include <EthernetBonjour.h>
#include <PubSubClient.h>
#include <Wire.h>

// define SERVER_NAME, CLIENT_ID and PREFIX for every device
#define SERVER_NAME "server_name"
#define CLIENT_ID "8xSwitch-00000000"
#define PREFIX "00000000-0000-0000-0000-00000000000"

#define OFF 0b00000000
#define speakerGnd 5
#define speakerPin 6
#define statusGnd 7
#define statusPin 8

int swState = OFF;
int ledState = OFF;
// you can find this written on the board of some Arduino Ethernets or shields
byte mac[] = { 0xDE, 0xAE, 0xCC, 0xEF, 0xFE, 0xED }; 

IPAddress serverAddr;
EthernetClient ethClient;
PubSubClient client(ethClient);

void(* resetFunc) (void) = 0;

void setupPorts() {
  Wire.begin();
  Wire.beginTransmission(0x20);
  Wire.write(0x01);
  Wire.write(0x00); // set entire PORT B to output
  Wire.endTransmission();

  writeB((byte)OFF);

  // piezo
  pinMode(speakerGnd, OUTPUT);
  pinMode(speakerPin, OUTPUT);
  digitalWrite(speakerGnd, LOW);
  digitalWrite(speakerPin, LOW);

  // connection status led
  pinMode(statusGnd, OUTPUT);
  pinMode(statusPin, OUTPUT);
  digitalWrite(statusGnd, LOW);
  digitalWrite(statusPin, LOW);
}

void playON() {
  tone(speakerPin, 392, 35);
  delay(35);
  tone(speakerPin, 784, 35);
  delay(35);
  tone(speakerPin, 1568, 35);
  delay(35);
  noTone(speakerPin);
}

void playOFF() {
  tone(speakerPin, 1568, 35);
  delay(35);
  tone(speakerPin, 784, 35);
  delay(35);
  tone(speakerPin , 392, 35);
  delay(35);
  noTone(speakerPin);
}

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

void resolveIP() {
  Ethernet.begin(mac); 
  EthernetBonjour.begin(CLIENT_ID);
  EthernetBonjour.setNameResolvedCallback(nameFound);
  
  while (NULL == serverAddr) {
    digitalWrite(statusPin, HIGH);

    EthernetBonjour.resolveName(SERVER_NAME, 5000);

    while (EthernetBonjour.isResolvingName()) {
      EthernetBonjour.run();
    }
    delay(500);
    digitalWrite(statusPin, LOW);
    delay(500);
  }
}

void registerCallbacks() {
  for (int i=0;i<=7;i++) {
    char topic[43];
    sprintf(topic, "/led/%s%d", PREFIX, (byte)i);
    // subscribe relays to their topics
    client.subscribe(topic);
    // register relays to the orchestrator
    client.publish("/ready/%s%d", PREFIX, (byte)i);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char command[length];
  for (int i=0;i<length;i++) {
    command[i] = (char)payload[i];
  }

  if (strlen(command) >= 2 && command[0] == 'O' && command[1] == 'N') {
    if (bitRead(ledState, (topic[strlen(topic)-1]-'0')) == 1) {
      return;
    }
    playON();
    ledState = ledState^1<<(topic[strlen(topic)-1]-'0');
    writeB((byte)ledState);
  }

  if (strlen(command) >= 3 && command[0] == 'O' && command[1] == 'F' && command[2] == 'F') {
    if (bitRead(ledState, (topic[strlen(topic)-1]-'0')) == 0) {
      return;
    }
    playOFF();
    ledState = ledState^1<<(topic[strlen(topic)-1]-'0');
    writeB((byte)ledState);
  }
}


void reconnect() {
  // Loop until we're reconnected
  for (int n=0;!client.connected()&&n<5;n++) {
    // will blink until the connection is established
    digitalWrite(statusPin, HIGH);
    
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      registerCallbacks();
      // status LED to HIGH
      break;
    }
    delay(500);
    digitalWrite(statusPin, LOW);
    delay(500);
  }
}

void nameFound(const char* name, const byte ipAddr[4]) {
  if (NULL != ipAddr) {
    serverAddr = IPAddress(ipAddr);
    return;
  }
  // reboot when timed out
  resetFunc(); 
}

const char* ip_to_str(const uint8_t* ipAddr) {
  static char buf[16];
  return buf;
}

void setup () {
  setupPorts();
  resolveIP();
  client.setServer(serverAddr, 1883);
  client.setCallback(callback);
  reconnect();
}

byte inputs=0;

void loop () {
  inputs = readA();
 
  for (int i=0;i<8;i++) {
    if (bitRead(inputs, i) != 0 && bitRead(inputs, i) != bitRead(swState, i)) {
      char topic[43];
      sprintf(topic, "/switch/%s%d", PREFIX, (byte)i);
      client.publish(topic, "TRIG");
    }
  }

  swState = inputs;
  if (!client.connected()) {
    resetFunc(); 
  }
  client.loop();
  EthernetBonjour.run();
}
