#include <WiFi.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Update.h>

// Cấu hình Wi-Fi
const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";

// URL của GitHub API để kiểm tra firmware
const char* github_api_url = "https://api.github.com/repos/TBach17112003/OTA_Update_Project/releases/latest";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Kích hoạt Basic OTA
  setupOTA();

  // Kiểm tra và cập nhật firmware qua HTTP Client (GitHub Releases)
  checkForUpdates();
}

void loop() {
  ArduinoOTA.handle(); // Xử lý OTA trực tiếp
}

void setupOTA() {
  ArduinoOTA.setHostname("ESP32-OTA");
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating via ArduinoOTA...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpdate complete!");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}

void checkForUpdates() {
  HTTPClient http;
  http.begin(github_api_url);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);

    // Lấy URL của firmware từ JSON
    String firmware_url = doc["assets"][0]["browser_download_url"];
    Serial.println("Firmware URL: " + firmware_url);

    if (!firmware_url.isEmpty()) {
      performOTAUpdate(firmware_url);
    }
  } else {
    Serial.println("Failed to fetch release info.");
  }

  http.end();
}

void performOTAUpdate(const String& firmware_url) {
  HTTPClient http;
  http.begin(firmware_url);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* client = http.getStreamPtr();
    if (Update.begin(UPDATE_SIZE_UNKNOWN)) {
      size_t written = Update.writeStream(*client);
      if (written > 0) {
        Serial.println("Firmware update written successfully.");
      } else {
        Serial.println("Failed to write firmware update.");
      }

      if (Update.end()) {
        Serial.println("Update complete. Restarting...");
        ESP.restart();
      } else {
        Serial.println("Update failed.");
      }
    } else {
      Serial.println("Not enough space for firmware update.");
    }
  } else {
    Serial.println("Failed to download firmware.");
  }

  http.end();
}
