#include "Wire.h"
#include <MPU6050_light.h>
#include "WiFiS3.h"

MPU6050 mpu(Wire);

// --- Wi-Fi SoftAP Settings ---
const char ssid[] = "SAD_Car";
const char pass[] = "12345678";
int status = WL_IDLE_STATUS;
WiFiServer server(80);

// --- L298N Motor Pins ---
const int enA = 9; const int in1 = 8; const int in2 = 7;
const int in3 = 5; const int in4 = 4; const int enB = 3;
const int motorSpeed = 180; 
;'
\'
String carState = "STOPPED";

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // 1. Setup Motor Pins
  pinMode(enA, OUTPUT); pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT); pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
  stopMotors();

  // 2. Setup MPU6050
  Wire.begin();
  byte mpuStatus = mpu.begin();
  if (mpuStatus != 0) {
    Serial.println("Could not connect to MPU6050.");
    while (1) { delay(10); }
  }
  Serial.println("Calibrating MPU6050... Keep flat!");
  delay(1000);
  mpu.calcOffsets(true, true);

  // 3. Start SoftAP Mode
  Serial.println("Starting Access Point...");
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating Access Point failed");
    while (1);
  }
  
  server.begin();
  Serial.print("AP Created! SSID: "); Serial.println(ssid);
  Serial.print("Dashboard URL: http://"); Serial.println(WiFi.localIP());
}

void loop() {
  mpu.update();
  float pitch = mpu.getAngleY();

  // Run the standard tilt automation logic
  if (pitch > 15.0) {
    driveForward();
    carState = "DRIVING FORWARD";
  } else if (pitch < -15.0) {
    driveBackward();
    carState = "DRIVING BACKWARD";
  } else {
    stopMotors();
    carState = "STOPPED";
  }

  // Handle Web Dashboard clients
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Send standard HTTP headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            
            // Build the HTML/JS Dashboard
            client.println("<!DOCTYPE HTML><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>");
            client.println("<style>body{font-family:Arial; text-align:center; background:#222; color:#fff;}");
            client.println(".box{background:#333; padding:20px; margin:20px auto; max-width:400px; border-radius:10px; box-shadow:0 4px 8px rgba(0,0,0,0.5);}");
            client.println("h1{color:#00ffcc;} .val{font-size:30px; font-weight:bold; color:#ffcc00;}</style>");
            
            // JavaScript snippet to fetch new readings every 200ms dynamically without reloading
            client.println("<script>setInterval(function(){");
            client.println("fetch('/data').then(response => response.json()).then(data => {");
            client.println("document.getElementById('pitch').innerHTML = data.pitch + '&deg;';");
            client.println("document.getElementById('state').innerHTML = data.state;});");
            client.println("}, 200);</script></head>");
            
            // Body Structure
            client.println("<body><h1>R4 Car Dashboard</h1>");
            client.println("<div class='box'><h3>MPU6050 Pitch Angle</h3><div id='pitch' class='val'>0&deg;</div></div>");
            client.println("<div class='box'><h3>Car Status</h3><div id='state' class='val'>STOPPED</div></div>");
            client.println("</body></html>");
            break;
          } else {
            // If client is requesting JSON data stream
            if (currentLine.startsWith("GET /data")) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Connection: close");
              client.println();
              client.print("{\"pitch\":"); client.print(pitch);
              client.print(",\"state\":\""); client.print(carState); client.println("\"}");
              break;
            }
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}

// --- Motor Control Driving Functions ---
void driveForward() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW); analogWrite(enA, motorSpeed);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW); analogWrite(enB, motorSpeed);
}

void driveBackward() {
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH); analogWrite(enA, motorSpeed);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH); analogWrite(enB, motorSpeed);
}

void stopMotors() {
  digitalWrite(in1, LOW); digitalWrite(in2, LOW); analogWrite(enA, 0);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW); analogWrite(enB, 0);
}
