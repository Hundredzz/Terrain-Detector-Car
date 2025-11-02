#include <WiFiS3.h>
#include <MQTTClient.h>

unsigned long lastRoughTime = 0; // เวลาที่ตรวจพบพื้นผิวที่ไม่เรียบครั้งล่าสุด
const unsigned long SMOOTH_IGNORE_DURATION = 2000; // ดีเลย์ให้อสดงคำว่า Rough 2 วินาที
const int trigPin1 = 4, trigPin2 = 9;
const int echoPin1 = 3, echoPin2 = 8;
int prev = -1; // เก็บระยะทางจากพื้นถึงตัวรถตอนเริ่มทำงาน
long duration1, duration2; // ตัวแปรรับค่าจาก Ultra Sonic
double distanceCm1, distanceCm2; // ระยะทางหลังจากคำนวน
const int AP = 5;   // A+
const int AM = 6;   // A-
const int BP = 10;  // B+
const int BM = 11;  // B-

String motorState = "STOPPED";
unsigned long motorStartTime = 0; // เวลาที่มอเตอร์เริ่มทำงานแต่ละครั้ง
const unsigned long motorDuration = 2000; // กดหนึ่งครั้ง มอเตอร์จะทำงาน 2 seconds

const char WIFI_SSID[] = "";     // CHANGE TO YOUR WIFI SSID
const char WIFI_PASSWORD[] = ""; // CHANGE TO YOUR WIFI PASSWORD

const char MQTT_BROKER_ADRRESS[] = "mqtt-dashboard.com";  // CHANGE TO MQTT BROKER'S ADDRESS
//const char MQTT_BROKER_ADRRESS[] = "192.168.0.11";  // CHANGE TO MQTT BROKER'S IP ADDRESS
const int MQTT_PORT = 1883;
const char MQTT_CLIENT_ID[] = "TD-car-14564564";  // Client_ID
const char MQTT_USERNAME[] = "";              // CHANGE IT IF REQUIRED, empty if not required
const char MQTT_PASSWORD[] = "";              // CHANGE IT IF REQUIRED, empty if not required

// The MQTT topics that Arduino should publish/subscribe
const char PUBLISH_TOPIC[] = "TD-car/remote";       // CHANGE IT AS YOU DESIRE
const char SUBSCRIBE_TOPIC[] = "TD-car/car";  // CHANGE IT AS YOU DESIRE

const int PUBLISH_INTERVAL = 500;  // 500 milliseconds

WiFiClient network;
MQTTClient mqtt = MQTTClient(256);

unsigned long lastPublishTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    Serial.print("Arduino UNO R4 - Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // print your board's IP address:
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  connectToMQTT();
}

void loop() {
  mqtt.loop();
  handleMotor(); // ฟังก์ชันที่ทใช้ขับเคลื่อนมอเตอร์
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);

  duration1 = pulseIn(echoPin1, HIGH);
  distanceCm1 = duration1 * 0.034 / 2;

  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
    
  // Set the trigPin HIGH for 10 microseconds
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  // Read the echoPin, return the sound wave travel time in microseconds
  duration2 = pulseIn(echoPin2, HIGH);

  // Calculate the distance in cm and inches
  distanceCm2 = duration2 * 0.034 / 2;

  if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
    sendToMQTT();
    lastPublishTime = millis();
  }
}

void handleMotor() {
  if (motorState == "STOPPED" ) {
    return;
  }

  unsigned long now = millis(); // นับเวลาปัจจุบัน
  if (now - motorStartTime < motorDuration) {
      if(motorState == "UP"){ // เคลื่อนที่ไปข้างหน้า
        if(distanceCm2 <= 5){ // ถ้ามีสิ่งกีดขวางในระยะ 5 เซน มอเตอร์จะหยุด
          analogWrite(AP, 0);
          analogWrite(BM, 0);
          return;
        }
        analogWrite(AP, 255);
        analogWrite(AM, 0);
        analogWrite(BP, 0);
        analogWrite(BM, 200);
      }
      else if(motorState == "DOWN"){ // เคลื่อนที่ไปข้างหลัง
        analogWrite(AP, 0);
        analogWrite(AM, 255);
        analogWrite(BP, 200);
        analogWrite(BM, 0);
      }
      else if(motorState == "LEFT"){ // หันซ้าย
        analogWrite(AP, 200);
        analogWrite(AM, 0);
        analogWrite(BP, 200);
        analogWrite(BM, 0);
      }
      else if(motorState == "RIGHT"){ // หันขวา
        analogWrite(AP, 0);
        analogWrite(AM, 200);
        analogWrite(BP, 0);
        analogWrite(BM, 255);
      }
    } else {
      // หยุดมอเตอร์หลังหมดเวลา
      analogWrite(AP, 0);
      analogWrite(AM, 0);
      analogWrite(BP, 0);
      analogWrite(BM, 0);
      motorState = "STOPPED";
    }
}

void connectToMQTT() {
  // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageReceived);

  Serial.print("Arduino UNO R4 - Connecting to MQTT broker");

  while (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if (!mqtt.connected()) {
    Serial.println("Arduino UNO R4 - MQTT broker Timeout!");
    return;
  }

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("Arduino UNO R4 - Subscribed to the topic: ");
  else
    Serial.print("Arduino UNO R4 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("Arduino UNO R4 - MQTT broker Connected!");
}

void sendToMQTT() {
// inside sendToMQTT()

if(prev == -1){ // ถ้ายังไม่มีระยะทางจากพื้นถึงตัวรถ
  prev = distanceCm1;
}

unsigned long now = millis(); // นับเวลาเพื่อเช็ก delay rough
String val_str;

if(distanceCm2 <= 5){
  val_str = "block";
}
else if(abs(distanceCm1 - prev) >= 1){
  val_str = "rough";
  lastRoughTime = now; // reset smooth timer
}
else if(now - lastRoughTime >= SMOOTH_IGNORE_DURATION){ // จะแสดงผล smooth หลังครบดีเลย์และพื้นผิวเรียบ
  val_str = "smooth";
}

char messageBuffer[10];
val_str.toCharArray(messageBuffer, 10);
mqtt.publish(PUBLISH_TOPIC, messageBuffer);

Serial.println("Arduino UNO R4 - sent to MQTT:");
Serial.print("- topic: ");
Serial.println(PUBLISH_TOPIC);
Serial.print("- payload:");
Serial.println(messageBuffer);
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Arduino UNO R4 - received from MQTT:");
  Serial.println("- topic: " + topic);
  Serial.println("- payload:");
  Serial.println(payload);

  if(payload == "up"){
    upPress();
  }else if(payload == "down"){
    downPress();
  }else if(payload == "left"){
    leftPress();
  }else if(payload == "right"){
    rightPress();
  }
}

// ฟังก์ชันที่ใช้เปลี่ยน state moter และเริ่มนับเวลา
void upPress()   { motorState = "UP";    motorStartTime = millis(); }
void downPress() { motorState = "DOWN";  motorStartTime = millis(); }
void rightPress() { motorState = "LEFT";  motorStartTime = millis(); }
void leftPress(){ motorState = "RIGHT"; motorStartTime = millis(); }