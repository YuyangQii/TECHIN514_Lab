#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

const char* ssid = "UW MPSK";
const char* password = "7Ye6}mVzE&"; // Replace with your network password
#define DATABASE_URL "https://lab-5-9a467-default-rtdb.firebaseio.com/" // Replace with your database URL
#define API_KEY "AIzaSyACfz4T3He5Ne2lK25Qce4_Xe3wk7OkIJc" // Replace with your API key
#define STAGE_INTERVAL 12000 // 12 seconds each stage, defined in milliseconds


#define MAX_WIFI_RETRIES 5 // Maximum number of WiFi connection retries

const int uploadFrequencies[] = {500, 1000, 2000, 3000, 4000}; // 分别对应 2 Hz, 1 Hz, 0.5 Hz, 0.333 Hz, 0.25 Hz
const int numOfFrequencies = 5;
int uploadInterval = 0;

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// HC-SR04 Pins
const int trigPin = 2;
const int echoPin = 3;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Function prototypes
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // // First, we let the device run for 12 seconds without doing anything
  // Serial.println("Running for 12 seconds without doing anything...");
  unsigned long startTime = millis();
  // while (millis() - startTime < STAGE_INTERVAL)
  // {
  //   delay(100); // Delay between measurements
  // }

  // // Second, we start with the ultrasonic sensor only
  // Serial.println("Measuring distance for 12 seconds...");
  // startTime = millis();
  // while (millis() - startTime < STAGE_INTERVAL)
  // {
  //   measureDistance();
  //   delay(100); // Delay between measurements
  // }

  // // Now, turn on WiFi and keep measuring
  // Serial.println("Turning on WiFi and measuring for 12 seconds...");
  connectToWiFi();
  // startTime = millis();
  // while (millis() - startTime < STAGE_INTERVAL)
  // {
  //   measureDistance();
  //   delay(100); // Delay between measurements
  // }

  // Now, turn on Firebase and send data every 1 second with distance measurements
  Serial.println("Turning on Firebase and sending data every 0.5 second...");
  initFirebase();
  // startTime = millis();
 
 for (int i = 0; i < numOfFrequencies; i++) {
    // 设置当前频率
    uploadInterval = uploadFrequencies[i];
    Serial.print("当前频率: ");
    Serial.println(1000 / uploadInterval);

    unsigned long startMillis = millis();

    // 在当前频率上运行一段时间（例如12秒）
    while (millis() - startMillis < STAGE_INTERVAL) {
      // 检查是否是发送数据的时间
      if (millis() - sendDataPrevMillis > uploadInterval) {
        float distance = measureDistance();
        sendDataToFirebase(distance);
        sendDataPrevMillis = millis();
      }
    }
 }
  // Go to deep sleep for 12 seconds
  Serial.println("Going to deep sleep for 12 seconds...");
  WiFi.disconnect();
  esp_sleep_enable_timer_wakeup(STAGE_INTERVAL * 1000); // in microseconds
  esp_deep_sleep_start();
}

void loop(){
  // This is not used
}

float measureDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi()
{
  // Print the device's MAC address.
  Serial.println(WiFi.macAddress());
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wifiCnt++;
    if (wifiCnt > MAX_WIFI_RETRIES){
      Serial.println("WiFi connection failed");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase()
{
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

void sendDataToFirebase(float distance){
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > uploadInterval || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.pushFloat(&fbdo, "test/distance", distance)){
      Serial.println("PASSED");
      Serial.print("PATH: ");
      Serial.println(fbdo.dataPath());
      Serial.print("TYPE: " );
      Serial.println(fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.print("REASON: ");
      Serial.println(fbdo.errorReason());
    }
    count++;
  }
}