#include <SPI.h>
#include <mcp_can.h>

// MCP2515 CAN interface
#define CAN0_INT 2 // MCP2515 INT pin to digital pin 2
MCP_CAN CAN0(53);  // Digital pin 53 for CS

// MC33290 K-line interface (using Serial1: TX1 and RX1)
#define KLINE_TX 18
#define KLINE_RX 19

// Data request messages
unsigned char reqCoolantTemp[8] = {0xCD, 0x7A, 0xA6, 0x10, 0xD8, 0x01, 0x00, 0x00};
unsigned char reqBoost[8] = {0xCD, 0x7A, 0xA6, 0x12, 0x9D, 0x01, 0x00, 0x00};
unsigned char reqAFR[8] = {};
unsigned char reqMisfireCount[8] = {0xCD, 0x7A, 0xA6, 0x10, 0xCA, 0x01, 0x00, 0x00};
unsigned char reqIntakeAirTemp[8] = {0xCD, 0x7A, 0xA6, 0x10, 0xCE, 0x01, 0x00, 0x00};

// Data return messages
long CoolantTemp, Boost, AFR, MisfireCount, IntakeAirTemp;

// Timer variables
unsigned long lastSendTime1 = 0; // K-line message last send time
unsigned long sendDelay = 3000; // Delay between K-line + Canbus messages
unsigned long lastSendTime = 0; // Canbus message last send time
bool waitForReply = false;

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
}

void sendDiagnosticsMessage() {
  if ((millis() - lastSendTime) < sendDelay) { //timer
    return;
  }
  unsigned char canbusMessage[8] = {0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Canbus message for diagnostics
  // Serial.print("Sending Canbus message... ")
  CAN0.sendMsgBuf(0x000FFFFE, 1, 8, canbusMessage);
  lastSendTime = millis();
}

void setup() {
  Serial.begin(115200);  // Start serial communication MCP2515
  Serial1.begin(10800);  // Start K-line communication

  Serial.println("Setup started.");

  // Send initial messages on K-line and Can-bus to activate OBD2 port diagnostics
  sendKeepaliveMessage();
  sendDiagnosticsMessage();
  delay(1000); 
  Serial.println("Initial K-line and Can-bus message sent.");

  // Initialize MCP2515
  if (CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("Error Initializing MCP2515...");
    while (1);  // Hang if MCP2515 initialization fails
  }

  //pinMode(CAN0_INT, INPUT);
  CAN0.setMode(MCP_NORMAL);  // Set MCP2515 to normal mode
  Serial.println("MCP2515 in Normal mode.");
}

void loop() {

  sendKeepaliveMessage();    // Send K-line message if the delay period has passed
  sendDiagnosticsMessage();  // Send K-line message if the delay period has passed
  
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char buf[8]; 

// unsigned char canbusMessage[8] = {0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// CAN0.sendMsgBuf(0x000FFFFE, 1, 8, canbusMessage);

CAN0.sendMsgBuf(0x000FFFFE, 1, 8, reqCoolantTemp);
CAN0.sendMsgBuf(0x000FFFFE, 1, 8, reqBoost);
CAN0.sendMsgBuf(0x000FFFFE, 1, 8, reqAFR);
CAN0.sendMsgBuf(0x000FFFFE, 1, 8, reqMisfireCount);
CAN0.sendMsgBuf(0x000FFFFE, 1, 8, reqIntakeAirTemp);

  while (CAN_MSGAVAIL == CAN0.checkReceive())
  {
    CAN0.readMsgBuf(&rxId, &len, buf);
    //SerialUSB.println(INCOMING.id,HEX);
    if (rxId == 0x80800021) {     
      if ((buf[3] == 0x10) && (buf[4] == 0xD8)) {
        CoolantTemp = buf[5];
      Serial.print("Coolant temperature, C: ");
      CoolantTemp = (CoolantTemp * 0.75 - 48);
      Serial.print(CoolantTemp);
      Serial.println();
      }

      if ((buf[3] == 0x12) && (buf[4] == 0x9D)) {
       Boost = buf[5];
      Serial.print("Boost, mbar: ");
      Boost = (Boost * 10);
      Serial.print(Boost);
      Serial.println();  
      }
/*
      if ((buf[3] == 0x) && (buf[4] == 0x)) {
       AFR = buf[5];
      Serial.print("AFR: ");
      AFR = ();
      Serial.print(AFR);
      Serial.println();  
      } */
/*
      if ((buf[3] == 0x) && (buf[4] == 0x)) {
       MisfireCount = buf[5];
      Serial.print("Misfire count: ");
      MisfireCount = ();
      Serial.print(MisfireCount);
      Serial.println();  
      } */

      if ((buf[3] == 0x10) && (buf[4] == 0xCE)) {
       IntakeAirTemp = buf[5];
      Serial.print("Intake air temperature, C: ");
      IntakeAirTemp = (IntakeAirTemp * 0.75 - 48);
      Serial.print(IntakeAirTemp);
      Serial.println();  
      }
    }
  }
}
