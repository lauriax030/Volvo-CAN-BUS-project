#include <SPI.h>
#include <mcp_can.h>

// MCP2515 CAN interface
#define CAN0_INT 2 // Assuming MCP2515 INT pin is connected to digital pin 2
MCP_CAN CAN0(53);  // Assuming you're using digital pin 53 for CS

// MC33290 K-line interface (using Serial1: TX1 and RX1)
#define KLINE_TX 18
#define KLINE_RX 19

unsigned long lastSendTime = 0; // Keep track of the last time the K-line message was sent
unsigned long sendDelay = 3000; // Delay between K-line messages in milliseconds
unsigned long lastRequestTime = 0; // Track the last time CAN request was sent
// unsigned long requestDelay = 1000; // Delay between CAN requests in milliseconds
bool waitForReply = false;

void sendKLineMessage() {
  // Timer to manage delay
  if ((millis() - lastSendTime) < sendDelay) {
    return;  // Exit if the delay time hasn't passed yet
  }

  // Message for keeping the CAN-bus relay active
  unsigned char klineMessage[7] = {0x84, 0x40, 0x13, 0xB2, 0xF0, 0x03, 0x7C}; // K-line message
  Serial.print("Sending K-line message: ");
  for (int i = 0; i < 7; i++) {
    Serial.print(klineMessage[i], HEX);
    Serial.print(" ");
    Serial1.write(klineMessage[i]);  // Send each byte of the K-line message
  }
  Serial.println();  // Print a newline after sending the message

  lastSendTime = millis(); // Update the time when K-line message was last sent
}

void setup() {
  Serial.begin(115200);  // Start serial communication for debugging
  Serial1.begin(10800);  // Start K-line communication

  Serial.println("Setup started.");

  // Send initial message on K-line to activate CAN-bus relay
  sendKLineMessage();
  delay(1000); // Small delay to make sure K-line message is sent initially
  Serial.println("Initial K-line message sent.");

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
  // Handle K-line messaging independently from CAN messaging
  sendKLineMessage();  // Send K-line message if the delay period has passed
  
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char buf[8];

  if (CAN_MSGAVAIL == CAN0.checkReceive()) {
    CAN0.readMsgBuf(&rxId, &len, buf);

    Serial.print("Message ID: ");    
    Serial.println(rxId, HEX);

    Serial.print("Data: ");
    for (int i = 0; i < len; i++) {
      Serial.print(buf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    CAN0.setMode(MCP_NORMAL);
  }

  delay(10);

  }
