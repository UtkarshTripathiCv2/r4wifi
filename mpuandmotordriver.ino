#include "Wire.h"
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

// --- L298N Motor A (Right Side) Pins ---
const int enA = 9;
const int in1 = 8;
const int in2 = 7;

// --- L298N Motor B (Left Side) Pins ---
const int in3 = 5;
const int in4 = 4;
const int enB = 3;

// --- Motor Speed (0 to 255) ---
const int motorSpeed = 180; 

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // 1. Setup Motor Driver Pins
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  // Ensure motors are completely off at startup
  stopMotors();

  // 2. Setup MPU6050
  Serial.println("Booting up...");
  Wire.begin();
  
  byte status = mpu.begin();
  if (status != 0) {
    Serial.println("Could not connect to MPU6050. Check wiring.");
    while (1) { delay(10); }
  }
  
  Serial.println("Calculating offsets... DO NOT MOVE THE SENSOR!");
  delay(1000);
  mpu.calcOffsets(true, true); 
  Serial.println("Done! Ready to drive.\n");
}

void loop() {
  // Fetch the latest sensor data
  mpu.update();
  
  // Get the pitch (tilt forward/backward)
  float pitch = mpu.getAngleY(); 

  Serial.print("Pitch Angle: ");
  Serial.println(pitch);

  // --- Tilt Logic ---
  
  if (pitch > 15.0) {
    // Tilted forward more than 15 degrees
    driveForward();
    Serial.println("Action: DRIVING FORWARD");
  } 
  else if (pitch < -15.0) {
    // Tilted backward more than -15 degrees
    driveBackward();
    Serial.println("Action: DRIVING BACKWARD");
  } 
  else {
    // Sensor is relatively flat (between -15 and 15 degrees)
    stopMotors();
    Serial.println("Action: STOPPED");
  }
  
  delay(50); // Slight delay for stability
}


// ==========================================
//          MOTOR CONTROL FUNCTIONS
// ==========================================

void driveForward() {
  // Turn on Motor A forward
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, motorSpeed);

  // Turn on Motor B forward
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enB, motorSpeed);
}

void driveBackward() {
  // Turn on Motor A reverse
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  analogWrite(enA, motorSpeed); 

  // Turn on Motor B reverse
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enB, motorSpeed); 
}

void stopMotors() {
  // Cut power to both motors
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(enA, 0);

  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enB, 0);
}
