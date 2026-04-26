/*
 * ============================================================
 *  Smart Environmental Monitoring System (SEMS)
 *  MGM University, Aurangabad
 *  Department: Computer Science & IT (GYPC)
 *
 *  Team:
 *    Shaikh Faisal     (Roll No: A0231086)
 *    Bhushan Agnihotri (Roll No: A0231129)
 *    Rupesh Deglur     (Roll No: A0231073)
 *
 *  Guide: Prof. Pratibha P. Patil
 *         ETC Dept., JNEC, MGM University
 *
 *  Hardware:
 *    - Arduino UNO
 *    - DHT22  : Temperature & Humidity Sensor (Digital Pin 7)
 *    - MQ-135 : Air Quality / Gas Sensor      (Analog Pin A0)
 *    - Rain Sensor                            (Digital Pin 8)
 *    - LDR    : Light Intensity Sensor        (Analog Pin A1)
 *    - 16x2 LCD Display (I2C)                (SDA A4, SCL A5)
 *    - Buzzer                                 (Digital Pin 9)
 *    - ESP8266 Wi-Fi Module                   (Serial: TX/RX)
 *
 *  Cloud Platform: Blynk / ThingSpeak
 *  Academic Year: 2026-2027
 * ============================================================
 */

// ── Libraries ──────────────────────────────────────────────
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// ── Pin Definitions ─────────────────────────────────────────
#define DHT_PIN       7       // DHT22 data pin
#define DHT_TYPE      DHT22   // Sensor type
#define MQ135_PIN     A0      // MQ-135 analog pin
#define RAIN_PIN      8       // Rain sensor digital pin
#define LDR_PIN       A1      // LDR analog pin
#define BUZZER_PIN    9       // Buzzer digital pin
#define ESP_TX        10      // ESP8266 TX (SoftwareSerial)
#define ESP_RX        11      // ESP8266 RX (SoftwareSerial)

// ── Threshold Values ────────────────────────────────────────
#define TEMP_MAX      35.0    // Max safe temperature (°C)
#define TEMP_MIN      10.0    // Min safe temperature (°C)
#define HUMIDITY_MAX  80.0    // Max safe humidity (%)
#define AQ_THRESHOLD  400     // Air quality alert threshold (PPM)
#define LDR_THRESHOLD 300     // Low light threshold

// ── Object Initialization ───────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27
SoftwareSerial espSerial(ESP_RX, ESP_TX);

// ── Blynk / ThingSpeak Config ───────────────────────────────
// Replace with your actual credentials
String WIFI_SSID     = "YOUR_WIFI_SSID";
String WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
String API_KEY       = "YOUR_THINGSPEAK_API_KEY"; // ThingSpeak Write API Key

// ── Global Variables ────────────────────────────────────────
float temperature  = 0.0;
float humidity     = 0.0;
int   airQuality   = 0;
bool  rainDetected = false;
int   lightLevel   = 0;
bool  alertActive  = false;

unsigned long lastUpload   = 0;
unsigned long lastDisplay  = 0;
const long UPLOAD_INTERVAL  = 15000; // Upload every 15 seconds
const long DISPLAY_INTERVAL = 3000;  // Switch LCD screen every 3 seconds
int displayScreen = 0;

// ============================================================
void setup() {
  Serial.begin(9600);
  espSerial.begin(115200);

  // Initialize pins
  pinMode(RAIN_PIN,   INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize DHT sensor
  dht.begin();

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  SEMS System  ");
  lcd.setCursor(0, 1);
  lcd.print(" Initializing..");
  delay(2000);
  lcd.clear();

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  connectWiFi();

  Serial.println("SEMS Ready!");
}

// ============================================================
void loop() {
  readSensors();
  checkAlerts();
  updateDisplay();

  // Upload to cloud every 15 seconds
  if (millis() - lastUpload >= UPLOAD_INTERVAL) {
    uploadToCloud();
    lastUpload = millis();
  }

  delay(500);
}

// ============================================================
//  Read all sensor values
// ============================================================
void readSensors() {
  // DHT22 — Temperature & Humidity
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity    = h;

  // MQ-135 — Air Quality (raw analog value, approx PPM)
  airQuality = analogRead(MQ135_PIN);

  // Rain Sensor — LOW means rain detected
  rainDetected = (digitalRead(RAIN_PIN) == LOW);

  // LDR — Light Intensity
  lightLevel = analogRead(LDR_PIN);

  // Print to Serial Monitor for debugging
  Serial.println("──────────────────────");
  Serial.print("Temperature : "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidity    : "); Serial.print(humidity);    Serial.println(" %");
  Serial.print("Air Quality : "); Serial.print(airQuality);  Serial.println(" (raw)");
  Serial.print("Rain        : "); Serial.println(rainDetected ? "Detected" : "No Rain");
  Serial.print("Light Level : "); Serial.println(lightLevel);
}

// ============================================================
//  Check thresholds and trigger alerts
// ============================================================
void checkAlerts() {
  alertActive = false;

  if (temperature > TEMP_MAX || temperature < TEMP_MIN) {
    alertActive = true;
    Serial.println("ALERT: Abnormal Temperature!");
  }
  if (humidity > HUMIDITY_MAX) {
    alertActive = true;
    Serial.println("ALERT: High Humidity!");
  }
  if (airQuality > AQ_THRESHOLD) {
    alertActive = true;
    Serial.println("ALERT: Poor Air Quality!");
  }
  if (rainDetected) {
    alertActive = true;
    Serial.println("ALERT: Rain Detected!");
  }

  // Activate or deactivate buzzer
  if (alertActive) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ============================================================
//  Update LCD display (rotates between screens every 3 sec)
// ============================================================
void updateDisplay() {
  if (millis() - lastDisplay < DISPLAY_INTERVAL) return;
  lastDisplay = millis();
  lcd.clear();

  switch (displayScreen) {
    case 0:
      // Screen 1: Temperature & Humidity
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print(" C");
      lcd.setCursor(0, 1);
      lcd.print("Humidity: ");
      lcd.print(humidity, 1);
      lcd.print("%");
      break;

    case 1:
      // Screen 2: Air Quality
      lcd.setCursor(0, 0);
      lcd.print("Air Quality:");
      lcd.setCursor(0, 1);
      if (airQuality < 200) {
        lcd.print("Good (");
      } else if (airQuality < 400) {
        lcd.print("Moderate (");
      } else {
        lcd.print("Poor (");
      }
      lcd.print(airQuality);
      lcd.print(")");
      break;

    case 2:
      // Screen 3: Rain & Light
      lcd.setCursor(0, 0);
      lcd.print("Rain: ");
      lcd.print(rainDetected ? "Detected" : "No Rain");
      lcd.setCursor(0, 1);
      lcd.print("Light: ");
      lcd.print(lightLevel < LDR_THRESHOLD ? "Low" : "Normal");
      break;

    case 3:
      // Screen 4: Alert Status
      lcd.setCursor(0, 0);
      lcd.print("Status:");
      lcd.setCursor(0, 1);
      lcd.print(alertActive ? "!! ALERT ACTIVE" : "All Normal OK");
      break;
  }

  displayScreen = (displayScreen + 1) % 4;
}

// ============================================================
//  Upload sensor data to ThingSpeak via ESP8266
// ============================================================
void uploadToCloud() {
  Serial.println("Uploading to ThingSpeak...");

  String data = "field1=";
  data += String(temperature, 1);
  data += "&field2=";
  data += String(humidity, 1);
  data += "&field3=";
  data += String(airQuality);
  data += "&field4=";
  data += String(rainDetected ? 1 : 0);
  data += "&field5=";
  data += String(lightLevel);

  // Send AT commands to ESP8266
  sendATCommand("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80", 3000);

  String httpRequest = "GET /update?api_key=" + API_KEY + "&" + data + " HTTP/1.1\r\n";
  httpRequest += "Host: api.thingspeak.com\r\n";
  httpRequest += "Connection: close\r\n\r\n";

  sendATCommand("AT+CIPSEND=" + String(httpRequest.length()), 2000);
  espSerial.print(httpRequest);
  delay(2000);

  sendATCommand("AT+CIPCLOSE", 1000);
  Serial.println("Upload complete.");
}

// ============================================================
//  Connect Arduino to Wi-Fi via ESP8266
// ============================================================
void connectWiFi() {
  sendATCommand("AT", 1000);
  sendATCommand("AT+CWMODE=1", 1000);
  sendATCommand("AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASSWORD + "\"", 8000);
  sendATCommand("AT+CIPMUX=0", 1000);
  Serial.println("Wi-Fi Connected!");
}

// ============================================================
//  Send AT command to ESP8266 and wait for response
// ============================================================
void sendATCommand(String command, int timeout) {
  espSerial.println(command);
  long start = millis();
  while (millis() - start < timeout) {
    if (espSerial.available()) {
      Serial.write(espSerial.read());
    }
  }
}

// ============================================================
//  Get air quality label string
// ============================================================
String getAirQualityLabel() {
  if (airQuality < 200) return "Good";
  else if (airQuality < 400) return "Moderate";
  else return "Poor";
}
