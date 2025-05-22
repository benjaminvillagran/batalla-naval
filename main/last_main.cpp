#include <Wire.h>
#include <BH1750.h>
#include "BluetoothSerial.h"

BH1750 lightMeter;
BluetoothSerial SerialBT;

const int relePin = 4;
int luxUmbral = 100;

void setup() {
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW); 

  Wire.begin(6, 7); 
  lightMeter.begin();

  Serial.begin(115200);
  SerialBT.begin("programatolaba"); 
  Serial.println("preparadopa");
  SerialBT.println("preparadopa");
}

void loop() {
  if (SerialBT.available()) {
    String input = SerialBT.readStringUntil('\n');
    int nuevoUmbral = input.toInt();
    if (nuevoUmbral > 0) {
      luxUmbral = nuevoUmbral;
      Serial.println("umbral nuevo: " + String(luxUmbral));
      SerialBT.println("umbral nuevo: " + String(luxUmbral));
    } else {
      Serial.println("esta mal el numero");
      SerialBT.println("esta mal el numero");
    }
  }

  float lux = lightMeter.readLightLevel();
  Serial.print("lux actual: ");
  Serial.print(lux);
  Serial.println(" lx");

  SerialBT.print("lux actual: ");
  SerialBT.print(lux);
  SerialBT.println(" lx");

  if (lux < luxUmbral) {
    digitalWrite(relePin, HIGH); 
  } else {
    digitalWrite(relePin, LOW);  
  }

  delay(100); 
}