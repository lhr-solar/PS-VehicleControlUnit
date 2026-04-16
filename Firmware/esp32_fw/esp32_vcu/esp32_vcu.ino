// Transmitter

#include <esp_now.h>
#include <WiFi.h>

#define SEND_PERIOD 5

#define HEARTBEAT_LED 19
#define FLASH_LED 20
#define ERR_LED 21
#define BLE_LED 22

// REPLACE WITH YOUR RECEIVER'S MAC ADDRESS
// uint8_t receiverAddress[] = { 0x7C, 0x2C, 0x67, 0x5D, 0x3B, 0xDC }; // Ravi's esp32
uint8_t receiverAddress[] = { 0x20, 0x6E, 0xF1, 0x11, 0x55, 0x9C }; //ESP32-C6 dev board


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
    digitalWrite(ERR_LED, HIGH);
    return;
  }

  // Register the receiver (peer)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    digitalWrite(ERR_LED, HIGH);
    return;
  }
}

void loop() {
  static uint8_t frameBuf[MAX_PACKET_SIZE];
  static size_t frameLen = 0;
  esp_err_t espNowResult = ESP_OK;
  bool sawError = false;

  /*
   * Forward one complete SLCAN line per ESP-NOW packet so downstream readers
   * (e.g. Electron serial parser) never see split frames.
   */
  while (Serial0.available()) {
    const int raw = Serial0.read();
    if (raw < 0) {
      break;
    }
    const uint8_t c = (uint8_t)raw;
    Serial.write(c);  // Mirror to USB for debug

    if (frameLen >= MAX_PACKET_SIZE) {
      frameLen = 0;
      sawError = true;
      continue;
    }

    frameBuf[frameLen++] = c;

    if (c == '\r' || c == '\n') {
      if (frameLen > 0) {
        espNowResult = esp_now_send(receiverAddress, frameBuf, frameLen);
        if (espNowResult != ESP_OK) {
          sawError = true;
        }
      }
      frameLen = 0;
    }
  }

  if (sawError) {
    digitalWrite(ERR_LED, HIGH);
  } else {
    digitalWrite(ERR_LED, LOW);
  }


  // const char* message = "Hello World\n";
  // // Send the message
  // // (uint8_t*) casts the string to a byte pointer
  // // strlen(message) + 1 includes the null terminator '\0' if you want the receiver to treat it as a string
  // espNowResult = esp_now_send(receiverAddress, (uint8_t*)message, strlen(message) + 1);

  // turn on the bluetooth LED if we sucesfully transmitted data.
  digitalWrite(BLE_LED, espNowResult == ESP_OK ? HIGH : LOW);

  digitalWrite(HEARTBEAT_LED, !digitalRead(HEARTBEAT_LED));


  delay(SEND_PERIOD);
}