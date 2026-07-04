#include "Wire.h"
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

void setup() {
  Serial.begin(115200);
  
  // Wait for R4 Native USB
  while (!Serial) { 
    delay(10); 
  }

  Serial.println("Booting up...");
  Wire.begin();
  
  // Initialize the sensor
  byte status = mpu.begin();
  Serial.print("MPU6050 status: ");
  Serial.println(status);
  
  if (status != 0) {
    Serial.println("Could not connect to MPU6050. Check wiring.");
    while (1) { delay(10); }
  }
  
  Serial.println("Calculating offsets... DO NOT MOVE THE SENSOR!");
  delay(1000);
  
  // This automatically calibrates the sensor for better accuracy
  mpu.calcOffsets(true, true); 
  Serial.println("Done! Starting loop...\n");
}

void loop() {
  // This line fetches and calculates the new data
  mpu.update();
  
  // Print the pre-calculated Roll and Pitch
  Serial.print("Roll (X-Angle): ");
  Serial.print(mpu.getAngleX());
  Serial.print("  |  ");
  Serial.print("Pitch (Y-Angle): ");
  Serial.println(mpu.getAngleY());
  
  delay(20); 
}
