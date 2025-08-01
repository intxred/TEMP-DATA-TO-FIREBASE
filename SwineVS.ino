#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <HX711_ADC.h>
#include <ThingSpeak.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// WiFi Credentials
const char* WIFI_SSID = "Kia";
const char* WIFI_PASSWORD = "Martypogi@1230";

// ThingSpeak Credentials
const unsigned long HX711_CHANNEL_ID = 2823326;
const char* HX711_TS_API_KEY = "8ATD1SSVL5798X4S";
WiFiClient client;

// Firebase Credentials
#define FIREBASE_HOST "tempsensor-39a21-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_API_KEY "AIzaSyDhK45HHDlpRbzjmA0s_iCPeaXzD9VXSxA"
#define FIREBASE_USER_EMAIL "kalabasangmgamonghe@gmail.com"
#define FIREBASE_USER_PASS "tristanjared14"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// HX711 Pins
const int HX711_dout = 4;
const int HX711_sck = 5;

// HX711 Setup
HX711_ADC LoadCell(HX711_dout, HX711_sck);
const float calibrationFactor = 696.0;
const float scaleFactor = 1.0;
const float conversionFactor = 0.03433;

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected.");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);

  // Initialize Firebase
  config.api_key = FIREBASE_API_KEY;
  auth.user.email = FIREBASE_USER_EMAIL;
  auth.user.password = FIREBASE_USER_PASS;
  config.database_url = FIREBASE_HOST;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize Load Cell
  Serial.println("Initializing Load Cell...");
  LoadCell.begin();
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("❌ Timeout! Check wiring.");
    while (1);
  } else {
    LoadCell.setCalFactor(calibrationFactor);
    Serial.println("✅ Load Cell Ready.");
    delay(2000);
  }
}

void loop() {
  if (LoadCell.update()) {
    float rawWeight = LoadCell.getData();
    float weightInGrams = rawWeight * scaleFactor;
    float convertedWeight = weightInGrams * conversionFactor;

    Serial.print("Load cell output (grams): ");
    Serial.println(weightInGrams, 2);
    Serial.print("Converted weight: ");
    Serial.println(convertedWeight, 2);

    // Send to Firebase
    if (Firebase.RTDB.setFloat(&fbdo, "/weight/converted", convertedWeight)) {
      Serial.println("✅ Firebase updated successfully.");
    } else {
      Serial.print("❌ Firebase error: ");
      Serial.println(fbdo.errorReason());
    }

    // Send to ThingSpeak
    int tsStatus = sendToThingSpeak(convertedWeight);
    if (tsStatus == 200) {
      Serial.println("✅ ThingSpeak updated successfully.");
    } else {
      Serial.print("❌ ThingSpeak error: ");
      Serial.println(tsStatus);
    }
  }

  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') {
      LoadCell.tareNoDelay();
      Serial.println("Tare requested...");
    }
  }

  if (LoadCell.getTareStatus()) {
    Serial.println("✅ Tare complete.");
    delay(2000);
  }

  delay(1000);  // **Updated delay to 1 second**
}

int sendToThingSpeak(float weight) {
  ThingSpeak.setField(1, weight);
  int status = ThingSpeak.writeFields(HX711_CHANNEL_ID, HX711_TS_API_KEY);
  return status;
}
