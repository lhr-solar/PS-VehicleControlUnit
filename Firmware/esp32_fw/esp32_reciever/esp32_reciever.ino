// Receiver

#include <esp_now.h>
#include <WiFi.h>

#define LED 18

// Callback function when data is received
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  (void)info;
  if (incomingData == NULL || len <= 0) {
    return;
  }
  Serial.write(incomingData, (size_t)len);

  digitalWrite(LED, !digitalRead(LED));
}

void setup() {
  pinMode(LED, OUTPUT);

  Serial.begin(115200); 
  delay(500);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  Serial.println("MAC Address: "+ WiFi.macAddress());
 
  // Register callback to handle incoming packets
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receiver Ready. Waiting for data...");
}

void loop() {
  // Receiver just waits for the callback
}