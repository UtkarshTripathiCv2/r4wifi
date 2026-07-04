#include "Wire.h"
#include <MPU6050_light.h>
#include "WiFiS3.h"

MPU6050 mpu(Wire);

// --- Wi-Fi SoftAP Settings ---
const char ssid[] = "R4_Robot_Car";
const char pass[] = "12345678";
int status = WL_IDLE_STATUS;
WiFiServer server(80);

// --- L298N Motor Pins ---
const int enA = 9; const int in1 = 8; const int in2 = 7;
const int in3 = 5; const int in4 = 4; const int enB = 3;
const int motorSpeed = 180; 

// Forward declarations so the compiler knows these exist right away
void stopMotors();
void driveForward();
void driveBackward();
void turnLeft();
void turnRight();
void sendHTML(WiFiClient &client);
void sendOK(WiFiClient &client);

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
  // Update the sensor data
  mpu.update();
  float pitch = mpu.getAngleY();

  // Handle Web Dashboard clients
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Send the HTML Web Page
            sendHTML(client);
            break;
          } else {
            // --- Parse the HTTP Request for Commands ---
            if (currentLine.startsWith("GET /data ")) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Connection: close\n");
              client.print("{\"pitch\":"); client.print(pitch); client.println("}");
              break;
            } 
            else if (currentLine.startsWith("GET /F ")) { driveForward(); sendOK(client); break; }
            else if (currentLine.startsWith("GET /B ")) { driveBackward(); sendOK(client); break; }
            else if (currentLine.startsWith("GET /L ")) { turnLeft(); sendOK(client); break; }
            else if (currentLine.startsWith("GET /R ")) { turnRight(); sendOK(client); break; }
            else if (currentLine.startsWith("GET /S ")) { stopMotors(); sendOK(client); break; }
            
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

// ==========================================
//          WEB SERVER HELPER FUNCTIONS
// ==========================================

void sendOK(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Connection: close\n");
}

void sendHTML(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close\n");
  
  client.println("<!DOCTYPE HTML><html><head><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no'>");
  client.println("<style>");
  client.println("body { font-family: sans-serif; text-align: center; background: #222; color: #fff; margin: 0; padding: 20px; touch-action: manipulation; }");
  client.println(".mpu-box { font-size: 24px; padding: 20px; background: #333; border-radius: 10px; margin: 20px auto; max-width: 300px; }");
  client.println(".val { color: #00ffcc; font-weight: bold; }");
  client.println(".grid { display: grid; grid-template-columns: 80px 80px 80px; gap: 15px; justify-content: center; margin-top: 40px; }");
  client.println(".btn { background: #007bff; color: white; border: none; font-size: 24px; font-weight: bold; border-radius: 15px; width: 80px; height: 80px; user-select: none; }");
  client.println(".btn:active { background: #00ffcc; color: #000; }");
  client.println("</style></head><body>");
  
  client.println("<h2>R4 Remote Control</h2>");
  client.println("<div class='mpu-box'>Pitch Angle:<br><span id='pitch' class='val'>0</span>&deg;</div>");
  
  client.println("<div class='grid'>");
  client.println("<div></div><button id='F' class='btn'>&#8593;</button><div></div>");
  client.println("<button id='L' class='btn'>&#8592;</button><button id='B' class='btn'>&#8595;</button><button id='R' class='btn'>&#8594;</button>");
  client.println("</div>");

  client.println("<script>");
  client.println("function send(c) { fetch('/' + c); }");
  client.println("function bind(id, cmd) {");
  client.println("  let b = document.getElementById(id); let active = false;");
  client.println("  let start = (e) => { e.preventDefault(); if(!active) { send(cmd); active = true; } };");
  client.println("  let end = (e) => { e.preventDefault(); if(active) { send('S'); active = false; } };");
  client.println("  b.addEventListener('mousedown', start); b.addEventListener('touchstart', start);");
  client.println("  b.addEventListener('mouseup', end); b.addEventListener('mouseleave', end); b.addEventListener('touchend', end);");
  client.println("}");
  client.println("bind('F', 'F'); bind('B', 'B'); bind('L', 'L'); bind('R', 'R');");
  client.println("setInterval(() => {");
  client.println("  fetch('/data').then(r => r.json()).then(d => { document.getElementById('pitch').innerText = d.pitch.toFixed(1); });");
  client.println("}, 400);");
  client.println("</script></body></html>");
}

// ==========================================
//          MOTOR CONTROL FUNCTIONS
// ==========================================

void driveForward() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW); analogWrite(enA, motorSpeed);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW); analogWrite(enB, motorSpeed);
}

void driveBackward() {
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH); analogWrite(enA, motorSpeed);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH); analogWrite(enB, motorSpeed);
}

void turnLeft() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW); analogWrite(enA, motorSpeed);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH); analogWrite(enB, motorSpeed);
}

void turnRight() {
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH); analogWrite(enA, motorSpeed);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW); analogWrite(enB, motorSpeed);
}

void stopMotors() {
  digitalWrite(in1, LOW); digitalWrite(in2, LOW); analogWrite(enA, 0);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW); analogWrite(enB, 0);
}
