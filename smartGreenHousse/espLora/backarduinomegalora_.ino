
#include <SPI.h>  // include libraries
#include <LoRa.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <Servo.h>

const int csPin = 53;      // LoRa radio chip select
const int resetPin = 5;    // LoRa radio reset
const int irqPin = 2;      // change for your board; must be a hardware interrupt pin
String outgoing;           // outgoing message

#define SERVO_PIN 38
//pin relay fan
#define FAN_PIN 3
//pin relay pompe
#define Pompe_PIN 4
//pin relay lampe
#define lampe_PIN 6
#define AO_PINL A15  // Light sensor pin
#define AO_PING A13  // Gas sensor pin
#define DHTPIN A14    // DHT sensor pin
#define DHTTYPE DHT22
#define DRain_PIN 7   // Rain sensor power pin
#define RAIN_PIN A8   // Rain sensor data pin
#define SOIL_PIN A12  // Soil moisture sensor pin
Servo servo;

  String incoming = ""; 
int moistureValue=0 ;
bool etat = false;
//etat fan
bool etat_fan = false;
//etat pompe
bool etat_pompe = false;
//etat lampe
bool etat_lampe = false;
bool mode = false;
//etat manuel
bool mfan = false;
bool mlampe = false;
bool mpompe = false;
bool mservo = false;
// LoRa pin definitions
int val;
int rain_state;
int rainState = 0;
int lightValue = 0;
int gasValue = 0;
float humidity = 0;
float temperature = 0;
String message = "";
DHT dht(DHTPIN, DHTTYPE);
//fonction

void readCapteur();
void automatique();
void manuel();
void onReceive(int);
void fenetreControl();
void ventilateur();
void irrigation();
void eclairage();
void processDataJson();
void sendMessage(String outgoing);
void setup() {
  Serial.begin(115200);  // initialize serial
  while (!Serial)
    ;

  Serial.println("LoRa Duplex with callback");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);  // set CS, reset, IRQ pin
//LoRa.setGain(2);

  pinMode(AO_PINL, INPUT);
  pinMode(AO_PING, INPUT);
  pinMode(DRain_PIN, INPUT);
   pinMode(RAIN_PIN, INPUT);
  // Initialize DHT sensor

  dht.begin();
  //servo
  servo.attach(SERVO_PIN);
  //relay fan
  pinMode(FAN_PIN, OUTPUT);
  //relay pompe
  pinMode(Pompe_PIN, OUTPUT);
  //relay lampe
  pinMode(lampe_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);
  digitalWrite(lampe_PIN, HIGH);
  digitalWrite(Pompe_PIN, HIGH);
  
  servo.write(0);
  if (!LoRa.begin(433E6)) {  // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);  // if failed, do nothing
  }
  LoRa.setSyncWord(0xF3);
  LoRa.onReceive(onReceive);
  LoRa.receive();
  
  Serial.println("LoRa init succeeded.");
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;
  incoming = "";
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
  delay(1000);
  Serial.println("Message: " + incoming);
  processDataJson(incoming); // Process data immediately after receiving
}

void loop() {
  readCapteur();
  sendMessage(message);
  Serial.println("Sending " + message);
  delay(1000); // Reduce delay if possible
  LoRa.receive();
   Serial.println("Message: " + incoming);
  if (mode) {
    manuel();
  } else {
    automatique();
  }
}


void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
                        // increment message ID
}


void readCapteur() {
  lightValue = analogRead(AO_PINL);
  gasValue = analogRead(AO_PING);

  humidity = dht.readHumidity();
    temperature = dht.readTemperature();
   if (isnan( humidity) || isnan( temperature)) {
     temperature =0;
      humidity =0;
  }
val=digitalRead(DRain_PIN);
  delay(10);  // Short delay for power to stabilize
  if(val){rainState=0;}
  else{
    rainState = analogRead(RAIN_PIN);}
  moistureValue = analogRead(SOIL_PIN);
  message = "{\"rainState\":" + String(rainState) + ",\"light\":" + String(lightValue) + ",\"servo\":" + String(etat) + ",\"fan\":" + String(etat_fan) + ",\"pompe\":" + String(etat_pompe) + ",\"lampe\":" + String(etat_lampe) + ",\"Temperature\":" + String(temperature) + ",\"hum\":" + String(humidity) + ",\"gaz\":" + String(gasValue) + ",\"moistureValue \": " + String(moistureValue) + "}";
  delay(1000);
}
void automatique() {
  Serial.println("mode auto");
  fenetreControl();
  ventilateur();
  irrigation();
  eclairage();
}
void manuel() {
  Serial.println("mode manuel");
  delay(1000);
  if (mfan == true) {
    digitalWrite(FAN_PIN, LOW);
  } else {
    digitalWrite(FAN_PIN, HIGH);
  }
  if (mlampe == true) {
    digitalWrite(lampe_PIN, LOW);  //closed
  } else {
    digitalWrite(lampe_PIN,HIGH );
  }
  if (mpompe) {
    digitalWrite(Pompe_PIN,LOW);
  } else {

    digitalWrite(Pompe_PIN,  HIGH);
  }
  if (mservo == 0) {
    servo.write(90);
  } else {
    servo.write(0);
  }
  // delay(1000);
}
void fenetreControl() {
  if ((lightValue > 400) || (val == 0)) {
    etat = false;
    // Serial.println("Porte fermé!");
    servo.write(90);
  } else {
    etat = true;
    //  Serial.println("Porte ouvert!");
    servo.write(0);
  }
}
void ventilateur() {
  if (temperature > 19) {
    etat_fan = true;
    digitalWrite(FAN_PIN,LOW );
    Serial.println("activer la Ventilateur ");
  } else {
    etat_fan = false;
    digitalWrite(FAN_PIN, HIGH);
    Serial.println("désactiver Ventilateur ");
  }
}
void irrigation() {
  if (moistureValue <= 530) {
    etat_pompe = true;
    digitalWrite(Pompe_PIN, LOW);
    //Serial.println("turn pump ON");
  } else {
    etat_pompe = false;
    digitalWrite(Pompe_PIN,HIGH );
    //Serial.println(" turn pump OFF");
  }
}
void eclairage() {
  if (lightValue > 600) {
    etat_lampe = true;
    digitalWrite(lampe_PIN,LOW);
  } else {
    etat_lampe = false;
    digitalWrite(lampe_PIN,  HIGH);
  }
}

void processDataJson(String data) {
  // Parse JSON data
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, data);
  // Extract values
  mode = bool(doc["mode"]);
  mfan = bool(doc["mfan"]);
  mlampe = bool(doc["mlampe"]);
  mpompe = bool(doc["mpompe"]);
  mservo = bool(doc["mservo"]);
}

