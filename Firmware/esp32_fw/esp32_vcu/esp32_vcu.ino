// Transmitter

#include <esp_now.h>
#include <WiFi.h>

#define SEND_PERIOD 500

#define HEARTBEAT_LED 19
#define FLASH_LED 20
#define ERR_LED 21
#define BLE_LED 22

// REPLACE WITH YOUR RECEIVER'S MAC ADDRESS
uint8_t receiverAddress[] = { 0x7C, 0x2C, 0x67, 0x5D, 0x3B, 0xDC };

#define BAUDRATE 115200
#define MAX_PACKET_SIZE 250  // ESP-NOW limit is 250 bytes

void setup() {
  Serial.begin(BAUDRATE);   // Computer output
  Serial0.begin(BAUDRATE);  // UART input (GPIO 16/17)

  WiFi.mode(WIFI_STA);  // Must be in station mode for ESP-NOW

  pinMode(HEARTBEAT_LED, OUTPUT);
  pinMode(BLE_LED, OUTPUT);
  pinMode(ERR_LED, OUTPUT);
  pinMode(FLASH_LED, OUTPUT);

  // default set all pins low
  digitalWrite(HEARTBEAT_LED, LOW);
  digitalWrite(BLE_LED, LOW);
  digitalWrite(ERR_LED, LOW);
  digitalWrite(FLASH_LED, LOW);

  Serial.write("ESP32 starting up");
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the receiver (peer)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  static uint8_t buffer[MAX_PACKET_SIZE];
  static int bufIdx = 0;

  // Read UART into local buffer
  while (Serial0.available() && bufIdx < MAX_PACKET_SIZE) {
    uint8_t c = Serial0.read();
    buffer[bufIdx] = c;
    bufIdx = (bufIdx + 1) % MAX_PACKET_SIZE;
    Serial.write(c);  // Mirror to USB
  }

  // Send via ESP-NOW if we have data
  if (bufIdx > 0) {
    esp_err_t result = esp_now_send(receiverAddress, buffer, bufIdx);
    bufIdx = 0;  // Reset buffer after attempt
  }

  const char* message = "Hello World\n";
  Serial.write("balls");
  Serial0.write("fuck you Ravi");
  // Send the message
  // (uint8_t*) casts the string to a byte pointer
  // strlen(message) + 1 includes the null terminator '\0' if you want the receiver to treat it as a string
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t*)message, strlen(message) + 1);

  digitalWrite(HEARTBEAT_LED, !digitalRead(HEARTBEAT_LED));
  // digitalWrite(FLASH_LED, !digitalRead(FLASH_LED));
  // digitalWrite(ERR_LED, !digitalRead(ERR_LED));
  // digitalWrite(BLE_LED, !digitalRead(BLE_LED));


  // delay 500 m
  delay(500);
}