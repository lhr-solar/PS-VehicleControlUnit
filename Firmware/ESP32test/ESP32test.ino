void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
}

void loop() {
  //print Hello world in every 2 seconds
  Serial.print("Hello World !!! \n");
  delay(2000); 
}