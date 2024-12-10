  #include <SPI.h>
  #include <mcp_can.h>

  // HighSpeed Canbus
  #define CAN0_INT 2 // MCP2515 INT pin to digital pin 2
  MCP_CAN CAN0(53);  // Digital pin 53 for CS

  // LowSpeed Canbus
  #define CAN1_INT 3
  MCP_CAN CAN1(49);

  // MC33290 K-line interface (using Serial1: TX1 and RX1)
  #define KLINE_TX 18
  #define KLINE_RX 19

// Define button IDs
const uint8_t UP[] = {0x48, 0x7f};
const uint8_t DOWN[] = {0x44, 0x7f};
const uint8_t LEFT[] = {0x42, 0x7f};
const uint8_t RIGHT[] = {0x41, 0x7f};
const uint8_t ENTER[] = {0x60, 0x7f};
const uint8_t BACK[] = {0x50, 0x7f};


  // Timer variables
  unsigned long lastSendTime1 = 0; // K-line message last send time
  unsigned long sendDelay = 3000; // Delay between K-line + Canbus messages
  unsigned long lastSendTime = 0; // Canbus message last send time
  bool waitForReply = false;
  bool runDisplay = false;
  bool lastRunDisplay = false;

  void sendKeepaliveMessage() {
  if ((millis() - lastSendTime1) < sendDelay) { //timer
    return; 
  }
  unsigned char klineMessage[7] = {0x84, 0x40, 0x13, 0xB2, 0xF0, 0x03, 0x7C}; // K-line message for Canbus relay
 // Serial.print("Sending K-line message... ");
  for (int i = 0; i < 7; i++) {
    Serial1.write(klineMessage[i]);
  }
  lastSendTime1 = millis(); // Update the time
  // Serial.println("K-line message sent.");
}

  void setup() {
    Serial.begin(115200);  // Start serial communication MCP2515
    Serial1.begin(10800);  // Start K-line communication
      // Send initial messages on K-line and Can-bus to activate OBD2 port diagnostics
  sendKeepaliveMessage();
  delay(1000); 
  Serial.println("Initial K-line and Can-bus message sent.");

    Serial.println("Setup started.");
/*
    // Initialize MCP2515
    if (CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK) {
      Serial.println("High-speed Canbus Initialized Successfully!");
    } else {
      Serial.println("Error Initializing High-speed Canbus...");
      while (1);  // Hang if MCP2515 initialization fails
    }
    CAN0.setMode(MCP_NORMAL);
    Serial.println("MCP2515 in Normal mode.");*/

    if (CAN1.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) {
      Serial.println("Low-speed Canbus Initialized Successfully!");
    } else {
      Serial.println("Error Initializing Low-speed Canbus...");
      while (1);  // Hang if MCP2515 initialization fails
    }
    CAN1.setMode(MCP_NORMAL);
    Serial.println("MCP2515 in Normal mode.");
  }

void loop() {
 sendKeepaliveMessage(); 

  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char buf[8];

  if (CAN1.checkReceive() == CAN_MSGAVAIL) {
    CAN1.readMsgBuf(&rxId, &len, buf);

    uint32_t getId = rxId & 0x00FFFFFF;
    
    if (getId == 0x200066 || getId == 0x400066 || getId == 0x404066) { // Check if message ID matches the button message ID
      // Check the last two bytes for button press
      if (buf[6] == UP[0] && buf[7] == UP[1]) {
        Serial.println("Up Button Pressed");
      }
      else if (buf[6] == DOWN[0] && buf[7] == DOWN[1]) {
        Serial.println("Down Button Pressed");
      }
      else if (buf[6] == LEFT[0] && buf[7] == LEFT[1]) {
        Serial.println("Left Button Pressed");
      }
      else if (buf[6] == RIGHT[0] && buf[7] == RIGHT[1]) {
        Serial.println("Right Button Pressed");
      }
      else if (buf[6] == ENTER[0] && buf[7] == ENTER[1]) {
        Serial.println("Enter Button Pressed");
        runDisplay = true;
      }
      else if (buf[6] == BACK[0] && buf[7] == BACK[1]) {
        Serial.println("Back Button Pressed");
        runDisplay = false;
      }
    }
  }
if (runDisplay != lastRunDisplay) {
  if (runDisplay) {
  Serial.println("Display opens...");
}
else {
  Serial.println("Display off...");
}
lastRunDisplay = runDisplay;
}
}    
