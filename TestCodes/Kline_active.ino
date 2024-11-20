// MC33290 K-line interface (using Serial1: TX1 and RX1)
#define KLINE_TX 18
#define KLINE_RX 19

unsigned long lastSendTime = 0; // Keep track of the last time the K-line message was sent
unsigned long sendDelay = 4000; // Delay between K-line messages in milliseconds

void setup() {
  Serial.begin(115200);
  Serial1.begin(10800);
}

void loop() {
  // Send periodic messages on K-line to keep CAN-bus relay active
  sendKLineMessage();
  delay(1000); 
}

void sendKLineMessage() {
  // Timer
  if ((millis() - lastSendTime) < sendDelay) { 
    return;
  }

  // Message for keeping the CAN-bus relay active
  unsigned char klineMessage[7] = {0x84, 0x40, 0x13, 0xB2, 0xF0, 0x03, 0x7C}; // K-line message

  Serial.print("Sending K-line message: ");
  for (int i = 0; i < 7; i++) {
    Serial.print(klineMessage[i], HEX);
    Serial.print(" ");
    Serial1.write(klineMessage[i]);
  }
  Serial.println();
  
  Serial.println("K-line message sent");
  lastSendTime = millis();
}
