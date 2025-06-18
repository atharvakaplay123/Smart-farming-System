#include "secrets.ino"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

unsigned long sendDataPrevMillis = 0;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

#include "DHT.h"
#define DHTPIN 13
DHT dht(DHTPIN, DHT11);

const int soil_pin = 34;
const int water_lvl_pin = 32;
const int pump_pin = 33;
const int led = 25;
float water_lvl_per;
float moisture_percentage;
int pump = 0;

void setup() {
  Serial.begin(115200);
  pinMode(soil_pin, INPUT);
  pinMode(water_lvl_pin, INPUT);
  pinMode(pump_pin, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(led, HIGH);
    Serial.print(".");
    delay(300);
    
  }
  digitalWrite(led, LOW);
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  config.timeout.serverResponse = 2 * 1000;
  config.cert.data = NULL;
  auth.user.email = "";  // Leave email and password empty
  auth.user.password = "";
  Firebase.begin(&config, &auth);
  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);
  Firebase.setDoubleDigits(5);


  Serial.println(F("DHTxx test!"));
  dht.begin();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(led, HIGH);
      Serial.print(".");
      delay(300);
    }
    digitalWrite(led, LOW);
  }
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    digitalWrite(led, HIGH);
    return;
  } else {
    digitalWrite(led, LOW);
  }
  moisture_percentage = 100 - ((analogRead(soil_pin) / 4095.00) * 100.00);
  if (analogRead(water_lvl_pin) > 0 && analogRead(water_lvl_pin) < 1200) {
    water_lvl_per = map(analogRead(water_lvl_pin), 0, 1200, 0, 5);
  } else if (analogRead(water_lvl_pin) > 1200 && analogRead(water_lvl_pin) < 3500) {
    water_lvl_per = map(analogRead(water_lvl_pin), 1200, 3500, 5, 50);
  } else if (analogRead(water_lvl_pin) > 3500 && analogRead(water_lvl_pin) < 4095) {
    water_lvl_per = map(analogRead(water_lvl_pin), 3500, 4095, 50, 100);
  }
  // Serial.print(analogRead(water_lvl_pin));
  // Serial.print("  ");
  // Serial.println(water_lvl_per);
  // delay(300);
  // Firebase.ready() should be called repeatedly to handle authentication tasks.
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 250 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    String State;
    if (Firebase.RTDB.getString(&fbdo, "/smart_farming/motor", &State)) {
      State = State.substring(2, State.length() - 2);
      Serial.println("Motor State: " + State);
      if (State == "ON") {
        digitalWrite(pump_pin, 1);
        Serial.println("pump ON");
      } else if (State == "OFF") {
        digitalWrite(pump_pin, 0);
        Serial.println("pump OFF");
      }
    } else {
      Serial.println(fbdo.errorReason().c_str());
    }
  }
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 250 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setFloat(&fbdo, "/smart_farming/Humidity", h)) {
      Serial.println("Humidity");
    } else {
      Serial.println(fbdo.errorReason().c_str());
    }
  }
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 250 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setFloat(&fbdo, "/smart_farming/Temperature", t)) {
      Serial.println("Temperature");
    } else {
      Serial.println(fbdo.errorReason().c_str());
    }
  }
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 250 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setFloat(&fbdo, "/smart_farming/Soil_Moisture", moisture_percentage)) {
      Serial.println("Soil_Moisture");
    } else {
      Serial.println(fbdo.errorReason().c_str());
    }
  }
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 250 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setFloat(&fbdo, "/smart_farming/Water_lvl", water_lvl_per)) {
      Serial.println("Water_lvl");
    } else {
      Serial.println(fbdo.errorReason().c_str());
    }
  }
  if (!Firebase.ready()) {
    digitalWrite(led, HIGH);
    Serial.println("Firebase not ready!");
    Serial.print("Wi-Fi Status: ");
    Serial.println(WiFi.status());
    Serial.print("Firebase Error: ");
    Serial.println(fbdo.errorReason().c_str());
    delay(1000);
  } else {
    digitalWrite(led, LOW);
  }
}