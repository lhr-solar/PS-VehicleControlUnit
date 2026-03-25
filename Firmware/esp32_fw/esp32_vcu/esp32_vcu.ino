// Transmitter

#include <esp_now.h>
#include <WiFi.h>

#define SEND_PERIOD 500

#define HEARTBEAT_LED 19
#define FLASH_LED 20
#define ERR_LED 21
#define BLE_LED 22
#define ADDRESS_LENGTH 6
#define ADDRESS_BUFFER_SIZE 10
#define START_MESSAGE_SIZE sizeof(startMessage)

// REPLACE WITH YOUR RECEIVER'S MAC ADDRESS
uint8_t receiverAddresses[ADDRESS_BUFFER_SIZE][ADDRESS_LENGTH] = {
{ 0x7C, 0x2C, 0x67, 0x5D, 0x3B, 0xDC }, // Ravi's esp32
{ 0x20, 0x6E, 0xF1, 0x11, 0x55, 0x9C }, //ESP32-C6 dev board
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
uint8_t numAddresses = 2;

uint8_t startMessage[] = "NEW ADDRESS:";


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
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  for(int i =0;i<numAddresses; i++){
    memcpy(peerInfo.peer_addr, receiverAddresses[i], 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      digitalWrite(ERR_LED, HIGH);
      Serial.println("Failed to add peer");
      return;
    }
    Serial.print("Successfully added receiver: ");
    for (int j = 0; j < ADDRESS_LENGTH; j++) {
      if (receiverAddresses[i][j] < 0x10) Serial.print("0"); // Manual padding for leading zero
      Serial.print(receiverAddresses[i][j], HEX);
      Serial.print(":");
    }
    Serial.println();
  }

  // if (esp_now_add_peer(&peerInfo) != ESP_OK) {
  //   Serial.println("Failed to add peer");
  //   digitalWrite(ERR_LED, HIGH);
  //   return;
  // }
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
    digitalWrite(ERR_LED, HIGH);
  }

  //check message for adding esp addresses
  if(bufIdx == START_MESSAGE_SIZE+ADDRESS_LENGTH){ // exact length of new address message
    uint8_t checkmessage = 1;
    for(int i =0;i<START_MESSAGE_SIZE; i++){
      if(startMessage[i]!=buffer[i]){
        checkmessage = 0;
        break;
      }
    }
    if(checkmessage){
      uint8_t newAddress[6];
      for(int i = START_MESSAGE_SIZE; i<START_MESSAGE_SIZE+ADDRESS_LENGTH; i++){
        newAddress[i-START_MESSAGE_SIZE] = buffer[i];
      }

      //check if it is already registered
      for(int i =0;i<numAddresses; i++){
        checkmessage = 0;
        for(int j =0;j<ADDRESS_LENGTH;j++){
          if(newAddress[j]!=receiverAddresses[i][j]){
            checkmessage = 1; // it is different, therefore add it to receiver addresses;
            break;
          }
        }
        if(checkmessage){ // add new address
          for(int j = 0;i <ADDRESS_LENGTH; j++){
            receiverAddresses[numAddresses][j] = newAddress[j];
          }
          esp_now_peer_info_t peerInfo = {};
          peerInfo.channel = 0;
          peerInfo.encrypt = false;
          memcpy(peerInfo.peer_addr, receiverAddresses[numAddresses], 6);
          if (esp_now_add_peer(&peerInfo) != ESP_OK){
            digitalWrite(ERR_LED, HIGH);
            Serial.println("Failed to add peer");
            // return;
          }else{
            Serial.print("Successfully added receiver: ");
            for (int j = 0; j < ADDRESS_LENGTH; j++) {
              if (receiverAddresses[numAddresses][j] < 0x10) Serial.print("0"); // Manual padding for leading zero
              Serial.print(receiverAddresses[numAddresses][j], HEX);
              Serial.print(":");
            }
            Serial.println();
          }
          numAddresses++;
          break;
        }
      }
    }
  }

  esp_err_t espNowResult = ESP_FAIL;
  // Send via ESP-NOW if we have data
  // char message[50];
  // sprintf(message, "Message #%d\r\n", messageNum);
  // const char* msgPtr = message;
  // messageNum++;
  // espNowResult = esp_now_send(receiverAddress, (uint8_t*)message, strlen(message) + 1);

  if (bufIdx > 0) {
    espNowResult = esp_now_send(0, buffer, bufIdx);
    bufIdx = 0;  // Reset buffer after attempt
  }


  // const char* message = "Hello World\n";
  // // Send the message
  // // (uint8_t*) casts the string to a byte pointer
  // // strlen(message) + 1 includes the null terminator '\0' if you want the receiver to treat it as a string
  // espNowResult = esp_now_send(receiverAddress, (uint8_t*)message, strlen(message) + 1);

  // turn on the bluetooth LED if we sucesfully transmitted data.
  digitalWrite(BLE_LED, espNowResult == ESP_OK ? HIGH : LOW);

  digitalWrite(HEARTBEAT_LED, !digitalRead(HEARTBEAT_LED));


  // delay
  delay(1000);
}