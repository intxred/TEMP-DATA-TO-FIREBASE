#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// ========================
// WiFi Credentials
const char* ssid = "Kia";
const char* password = "Martypogi@1230";

// ========================
// ThingSpeak Settings for MLX90614
const unsigned long MLX90614_CHANNEL_ID = 2856554;
const char* MLX90614_TS_API_KEY = "YEVUFY2UIT1FW77K";
WiFiClient mlxClient;

// ========================
// MLX90614 Temperature Sensor
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// ========================
// Firebase Settings
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
const char* FIREBASE_HOST = "tempsensor-39a21-default-rtdb.asia-southeast1.firebasedatabase.app";
const char* FIREBASE_API_KEY = "AIzaSyDhK45HHDlpRbzjmA0s_iCPeaXzD9VXSxA";
const char* FIREBASE_USER_EMAIL = "kalabasangmgamonghe@gmail.com";
const char* FIREBASE_USER_PASS  = "tristanjared14";

// ========================
// Timing Variables
unsigned long lastThingSpeakUpdate = 0;
unsigned long lastFirebaseUpdate = 0;
const unsigned long thingspeakInterval = 15000; // 15 seconds
const unsigned long firebaseInterval = 5000;    // 5 seconds

void setup() {
  Serial.begin(115200);
  delay(10);

  // Disable deep sleep
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize ThingSpeak Client
  ThingSpeak.begin(mlxClient);

  // Initialize MLX90614 Sensor
  Wire.begin(14, 12);
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX90614 sensor. Check wiring.");
    while (1);
  }
  Serial.println("MLX90614 sensor initialized.");

  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.api_key = FIREBASE_API_KEY;
  auth.user.email = FIREBASE_USER_EMAIL;
  auth.user.password = FIREBASE_USER_PASS;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  float ambientTemp = mlx.readAmbientTempC();
  float objectTemp  = mlx.readObjectTempC();
  bool outOfRange = (objectTemp < 26.0) || (objectTemp > 30.0);

  Serial.print("Object Temperature: ");
  Serial.println(objectTemp);

  unsigned long currentMillis = millis();

  // Send data to ThingSpeak every 15 seconds
  if (currentMillis - lastThingSpeakUpdate >= thingspeakInterval) {
    Serial.print("Sending data to ThingSpeak... ");
    ThingSpeak.setField(1, objectTemp);
    int tsStatus = ThingSpeak.writeFields(MLX90614_CHANNEL_ID, MLX90614_TS_API_KEY);
    if (tsStatus == 200) {
      Serial.println("✅ Data sent successfully.");
    } else {
      Serial.print("❌ Failed to send data, error code: ");
      Serial.println(tsStatus);
    }
    lastThingSpeakUpdate = currentMillis;
    yield();
  }

  // Upload data to Firebase every 5 seconds
  if (currentMillis - lastFirebaseUpdate >= firebaseInterval) {
    Firebase.RTDB.setFloat(&fbdo, "/temperature/ambient", ambientTemp);
    Firebase.RTDB.setFloat(&fbdo, "/temperature/object", objectTemp);
    Firebase.RTDB.setBool(&fbdo, "/temperature/outOfRange", outOfRange);
    lastFirebaseUpdate = currentMillis;
    yield();
  }
}
