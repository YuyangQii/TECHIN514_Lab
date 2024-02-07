#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* ssid = "UW MPSK";
const char* password = "7Ye6}mVzE&"; // Replace with your network password
#define DATABASE_URL "https://lab-5-9a467-default-rtdb.firebaseio.com/" // Replace with your database URL
#define API_KEY "AIzaSyAGzuJiwC-9zZG87uGnA9gT9OrdZ2P3hR4" // Replace with your API key
#define MAX_WIFI_RETRIES 5 // Maximum number of WiFi connection retries
#define STAGE_INTERVAL 5000 // 12 seconds each stage
#define MAX_WIFI_RETRIES 5 // Maximum number of WiFi connection retries

int uploadInterval = 3000; // 3 seconds each upload
unsigned long lastUploadTime = 0;
float lastDistance = 0.0;
bool isWiFiConnected = false;

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
void enterDeepSleep();

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Measure distance and enter deep sleep if no significant change for 30 seconds
  float currentDistance = measureDistance();
  if (currentDistance > 50) {
    enterDeepSleep(); // No significant change, enter deep sleep
  }
  lastDistance = currentDistance;
  delay(1000); // Check every 10 seconds

  // Connect to WiFi and Firebase
  connectToWiFi();
  initFirebase();
}

void loop() {
  float currentDistance = measureDistance();
  unsigned long currentMillis = millis();

  // Send data to Firebase only if there's a significant change in distance
  if (abs(currentDistance - lastDistance) >= 5 && currentMillis - lastUploadTime > uploadInterval) {
    sendDataToFirebase(currentDistance);
    lastUploadTime = currentMillis;
  }
  if(currentDistance > 50){
    enterDeepSleep();
  }

  lastDistance = currentDistance;

  // Implement your stages or other logic as needed
  delay(100);
}

float measureDistance() {
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

void connectToWiFi() {
  if (!isWiFiConnected) {
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi");
    int wifiRetryCount = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
      wifiRetryCount++;
      if (wifiRetryCount > MAX_WIFI_RETRIES) {
        Serial.println("Failed to connect to WiFi. Going to deep sleep.");
        enterDeepSleep();
      }
    }
    isWiFiConnected = true;
    Serial.println("Connected to WiFi");
  }
}

void initFirebase() {
  if (!signupOK) {
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    if (Firebase.signUp(&config, &auth, "", "")) {
      signupOK = true;
    } else {
      Serial.printf("Firebase signup failed: %s\n", config.signer.signupError.message.c_str());
      enterDeepSleep();
    }
    Firebase.begin(&config, &auth);
  }
}

void sendDataToFirebase(float distance) {
  if (Firebase.ready() && signupOK && (millis() - lastUploadTime > uploadInterval)) {
    lastUploadTime = millis();
    if (Firebase.RTDB.pushFloat(&fbdo, "test/distance", distance)) {
      Serial.println("Data sent to Firebase");
    } else {
      Serial.println("Failed to send data to Firebase. Error: " + fbdo.errorReason());
    }
  }
}

void enterDeepSleep() {
  Serial.println("Entering deep sleep for 12 seconds");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  isWiFiConnected = false;
  esp_sleep_enable_timer_wakeup(STAGE_INTERVAL * 1000ULL); // 12 seconds deep sleep
  esp_deep_sleep_start();
}