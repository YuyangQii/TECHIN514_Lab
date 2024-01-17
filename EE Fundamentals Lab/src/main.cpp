#include <Arduino.h>
const int analogPin = D0;

void setup(){
    Serial.begin(9600);
}

void loop(){
    int sensorValue = analogRead(analogPin);
    float voltage = sensorValue * (5.0/4095.0);

    Serial.print("analogRead:");
    Serial.print(sensorValue);
    Serial.print("Voltage:");
    Serial.println(voltage);

    delay(1000);
}