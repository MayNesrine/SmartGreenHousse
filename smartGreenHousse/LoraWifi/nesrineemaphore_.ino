#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

const int csPin = 5;
const int resetPin = 14;
const int irqPin = 2;
bool messageSentForFalseMode = false;
// WiFi and Firebase configuration
#define WIFI_SSID "JobGate"
#define WIFI_PASSWORD "JobGate2021"
#define API_KEY "AIzaSyA5pKm3m27xMD7wgkHoS_SwbPCrUgs6-Uk"
#define DATABASE_URL "https://serre-ab268-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
int i=0;
// Variables
bool mode;
bool mfan;
bool mlampe;
bool mpompe;
bool mservo;
bool signupOK;
byte msgCount = 0;
int interval = 2000;
long lastSendTime = 0;
String message = "{\"mode\":" + String(mode) + ",\"mfan\":" + String(mfan) + ",\"mlampe\":" + String(mlampe) + ",\"mpompe\":" + String(mpompe) + ",\"mservo\":" + String(mservo) + "}";
String incoming = "";
bool isReadingData = false;
bool isSendingData = false;

SemaphoreHandle_t xSemaphore = NULL;

void sendCommandFirebase(void* parameter);
void onReceive(void* parameter);
void sendMessage(String outgoing);
void processData(String data);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa init succeeded.");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println(WiFi.localIP());

  // Initialize Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign up succeeded.");
    signupOK = true;
  } else {
    Serial.printf("Firebase sign up error: %s\n", config.signer.signupError.message.c_str());
  }

  xSemaphore = xSemaphoreCreateMutex();
  
  // Tasks
  xTaskCreate(
    onReceive,
    "LoRaReceive",
    9000,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    sendCommandFirebase,
    "CommandFirebase",
    9000,
    NULL,
    1,
    NULL
  );
}

void loop() {
  //delay(1000);
}

void sendMessage(String outgoing) {
 
  LoRa.beginPacket();
  LoRa.print(outgoing);
  LoRa.endPacket();
  i++;
 /*  Serial.println("senddddddddddddddddddddddddddddddddddddddddddddd");
  msgCount++;*/
}

void onReceive(void* parameter) {
  Serial.println("task1");
    incoming = "";
  while (true) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      while (LoRa.available()) {
        incoming += (char)LoRa.read();
      }
      Serial.println("Message: " + incoming);
      xSemaphoreTake(xSemaphore, portMAX_DELAY);
      processData(incoming);
    
        delay(1000);
      xSemaphoreGive(xSemaphore);
    }
    delay(10); // Avoid busy loop
  }
}

void sendCommandFirebase(void* parameter) {
  Serial.println("task2");
  while (true) {
    if (Firebase.ready()) {
      xSemaphoreTake(xSemaphore, portMAX_DELAY);

      Firebase.RTDB.getBool(&fbdo, "/manActionneurs/mfan", &mfan);
      Firebase.RTDB.getBool(&fbdo, "/manActionneurs/mlampe", &mlampe);
      Firebase.RTDB.getBool(&fbdo, "/manActionneurs/mpompe", &mpompe);
      Firebase.RTDB.getBool(&fbdo, "/manActionneurs/mservo", &mservo);
      Firebase.RTDB.getBool(&fbdo, "/mode", &mode);

      StaticJsonDocument<200> doc;
      doc["mode"] = mode;
      doc["mfan"] = mfan;
      doc["mlampe"] = mlampe;
      doc["mpompe"] = mpompe;
      doc["mservo"] = mservo;
      doc["i"]=i;
      serializeJson(doc, message);
      Serial.println("---------------------------------------------");
      Serial.println(message);
  delay(1000);
    if (mode == true) {
        sendMessage(message);
        messageSentForFalseMode = false; // Reset the flag
      } else if (mode == false && !messageSentForFalseMode) {
        sendMessage(message);
        messageSentForFalseMode = true; // Set the flag to indicate message has been sent
      }
         xSemaphoreGive(xSemaphore);
      
    }
    delay(10); // Check Firebase every 10 seconds
  }
}

void processData(String data) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, data);

  int gaz = doc["gaz"];
  float temperature = doc["Temperature"];
  float hum = doc["hum"];
  int light = doc["light"];
  int rainState = doc["rainState"];
  int moistureValue = doc["moistureValue"];
  bool servoPos = bool(doc["servo"]);
  bool fan = bool(doc["fan"]);
  bool pompe = bool(doc["pompe"]);
  bool lampe = bool(doc["lampe"]);

  if (Firebase.ready()) {
    Firebase.RTDB.setInt(&fbdo, "/sensors/rainState", rainState);
    Firebase.RTDB.setInt(&fbdo, "/sensors/light", light);
    Firebase.RTDB.setBool(&fbdo, "/sensors/servoos", servoPos);
    Firebase.RTDB.setBool(&fbdo, "/sensors/fan", fan);
    Firebase.RTDB.setBool(&fbdo, "/sensors/pompe", pompe);
    Firebase.RTDB.setBool(&fbdo, "/sensors/lampe", lampe);
    Firebase.RTDB.setFloat(&fbdo, "/sensors/temperature", temperature);
    Firebase.RTDB.setFloat(&fbdo, "/sensors/hum", hum);
    Firebase.RTDB.setInt(&fbdo, "/sensors/gaz", gaz);
    Firebase.RTDB.setInt(&fbdo, "/sensors/moistureValue", moistureValue);
    Serial.println("Data sent to Firebase");
  }
}
