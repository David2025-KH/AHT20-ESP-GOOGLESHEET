#include <WiFi.h>              // Wi-Fi library for ESP32
#include <Wire.h>              // I²C communication library
#include <Adafruit_AHTX0.h>    // Library for AHT20 temperature/humidity sensor
#include <HTTPClient.h>        // HTTP library for ESP32

// ===== Wi-Fi Settings =====
const char* ssid     = "David1";
const char* password = "David2@24";

// ===== Google Sheets Settings =====
String Web_App_URL = "https://script.google.com/macros/s/AKfycbz98Yw7wroqw12Ru61KMUILIboZJ4lwdcQMqUsEeVG2B_ASyc0fJZNdl_246xruzsCM/exec";

bool googleSheetAvailable = false;

unsigned long lastGoogleSheetUploadTime = 0;
const unsigned long googleSheetUploadInterval = 15000; // 15s upload interval

// ===== Wi-Fi Control Variables =====
unsigned long lastReconnectAttempt = 0;
unsigned long reconnectInterval = 30000; // retry every 30s if Wi-Fi lost

// ===== Sensor Read Timing =====
unsigned long lastSensorReadTime = 0;
const unsigned long sensorReadInterval = 2000; // read every 2 seconds

// ===== Sensor Object =====
Adafruit_AHTX0 aht;
sensors_event_t ahtTemp, ahtHumidity;

// ===== Wi-Fi Connect Function =====
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Wi-Fi connects in the background; status is checked in loop()
}

// ===== Google Sheets Upload Function =====
void uploadToGoogleSheets() {
  String Send_Data_URL = Web_App_URL + "?sts=write";

  // --- Append AHT20 data ---
  Send_Data_URL += "&aht20_temp.temperature=" + String(ahtTemp.temperature);
  Send_Data_URL += "&aht20_humidity.relative_humidity=" + String(ahtHumidity.relative_humidity);

  Serial.println("\n-------------");
  Serial.println("Sending data to Google Spreadsheet...");
  Serial.print("URL: ");
  Serial.println(Send_Data_URL);

  HTTPClient http;
  http.begin(Send_Data_URL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  if (httpCode <= 0) {
    Serial.println("❌ Upload failed (no server response). Will retry on next interval.");
    googleSheetAvailable = false;
    Serial.println("Last Google Sheets upload failed.");  // <-- only print here
  } else if (httpCode != 200) {
    Serial.print("⚠ Unexpected HTTP response code: ");
    Serial.println(httpCode);
    googleSheetAvailable = false;
    Serial.println("Last Google Sheets upload failed.");  // <-- only print here
  } else {
    String payload = http.getString();
    Serial.println("Payload: " + payload);
    googleSheetAvailable = true;
    Serial.println("Last Google Sheets upload successful.");  // <-- only print here
  }

  http.end();
  Serial.println("-------------");
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Wire.begin();

  // --- Initialize AHT20 sensor ---
  if (!aht.begin()) {
    Serial.println("Could not find AHT20 sensor!");
    while (1) delay(10); // halt if sensor not found
  }
  Serial.println("AHT20 found!");

  connectWiFi(); // start Wi-Fi connection (non-blocking)
}

// ===== Main Loop =====
void loop() {
  unsigned long now = millis();

  // --- Read AHT20 sensor every sensorReadInterval ---
  if (now - lastSensorReadTime >= sensorReadInterval) {
    lastSensorReadTime = now;
    aht.getEvent(&ahtHumidity, &ahtTemp);

    // --- Print temperature and humidity on separate lines ---
    Serial.print("Temperature: ");
    Serial.print(ahtTemp.temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(ahtHumidity.relative_humidity);
    Serial.println(" %");
  }

  // --- Upload to Google Sheets every googleSheetUploadInterval ---
  if (now - lastGoogleSheetUploadTime >= googleSheetUploadInterval) {
    lastGoogleSheetUploadTime = now;
    uploadToGoogleSheets(); // prints status inside function
  }

  // --- Wi-Fi reconnect management ---
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastReconnectAttempt > reconnectInterval) {
      Serial.println("Retrying WiFi (non-blocking)...");
      connectWiFi();
      lastReconnectAttempt = now;
    }
  }
}
