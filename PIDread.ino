#include <SPI.h>
#include <mcp_can.h>

// MCP2515 CAN interface
#define CAN0_INT 2 // MCP2515 INT pin to digital pin 2
MCP_CAN CAN0(53);  // Digital pin 53 for CS

// MC33290 K-line interface (using Serial1: TX1 and RX1)
#define KLINE_TX 18
#define KLINE_RX 19

// Process functions
float processCoolantTemp(byte rawData) {
  return rawData * 0.75 - 48;
}
float processBoost(byte rawData) {
  return rawData * 10;
}
float processAFR(byte highByte, byte lowByte) {
  int rawData = 256 * highByte + lowByte;
  return rawData * 16.0 / 65536 * 14.7;
}
float processAFRexpected(byte highByte, byte lowByte) {
  int rawData = 256 * highByte + lowByte;
  return rawData * 16.0 / 65536 * 14.7;
}
float processMisfireCount(byte highByte, byte lowByte) {
  int rawData = 256 * highByte + lowByte;
  return rawData;
}
float processIntakeAirTemp(byte rawData) {
  return rawData * 0.75 - 48;
}

struct Condition {
  byte byte4;
  byte byte5;
  const char* name;
  float* data;
  float (*process)(byte highByte, byte lowByte);
  byte requestMessage[8];
};

float CoolantTemp = 0, Boost = 0, AFR = 0, AFRexpected = 0, MisfireCount = 0, IntakeAirTemp = 0;

Condition conditions[] = {
  {0x10, 0xD8, "CoolantTemp", &CoolantTemp, processCoolantTemp, {0xCD, 0x7A, 0xA6, 0x10, 0xD8, 0x01, 0x00, 0x00}},
  {0x12, 0x9D, "Boost", &Boost, processBoost, {0xCD, 0x7A, 0xA6, 0x12, 0x9D, 0x01, 0x00, 0x00}},
  {0x10, 0x34, "AFR", &AFR, processAFR, {0xCD, 0x7A, 0xA6, 0x10, 0x34, 0x01, 0x00, 0x00}},
  {0x12, 0xBB, "AFRexpected", &AFRexpected, processAFRexpected, {0xCD, 0x7A, 0xA6, 0x12, 0xBB, 0x01, 0x00, 0x00}},
  {0x10, 0xCA, "MisfireCount", &MisfireCount, processMisfireCount, {0xCD, 0x7A, 0xA6, 0x10, 0xCA, 0x01, 0x00, 0x00}},
  {0x10, 0xCE, "IntakeAirTemp", &IntakeAirTemp, processIntakeAirTemp, {0xCD, 0x7A, 0xA6, 0x10, 0xCE, 0x01, 0x00, 0x00}}
};

const int conditionCount = sizeof(conditions) / sizeof(conditions[0]);

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

void setup() {
  Serial.begin(115200);  // Start serial communication MCP2515
  Serial1.begin(10800);  // Start K-line communication

  Serial.println("Setup started.");

  // Send initial messages on K-line and Can-bus to activate OBD2 port diagnostics
  sendKeepaliveMessage();
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
  
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char buf[8]; 

 unsigned char canbusMessage[8] = {0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 CAN0.sendMsgBuf(0x000FFFFE, 1, 8, canbusMessage);

for (int i = 0; i < conditionCount; i++) {
    CAN0.sendMsgBuf(0x000FFFFE, 1, 8, conditions[i].requestMessage);
    if (CAN0.checkReceive() == CAN_MSGAVAIL) {
    CAN0.readMsgBuf(&rxId, &len, buf);
      if (rxId == 0x80800021 && buf[3] == conditions[i].byte4 && buf[4] == conditions[i].byte5) {
        *(conditions[i].data) = conditions[i].process(buf[5], buf[6]);
        Serial.print(conditions[i].name);
        Serial.print(": ");
        Serial.println(*(conditions[i].data));
      }
    }
  }
}    


    

