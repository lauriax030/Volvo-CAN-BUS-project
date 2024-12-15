#include <SPI.h>
#include <mcp_can.h>
#include <TVout.h>
#include <fontALL.h>
#include "Initial.h"

TVout TV;

// HighSpeed Canbus MCP2515
#define CAN0_INT 2 // INT pin to digital pin 2
MCP_CAN CAN0(53);  // Digital pin 53 for CS

// LowSpeed Canbus MCP2515
#define CAN1_INT 3
MCP_CAN CAN1(49);

// MC33290 K-line interface
#define KLINE_TX 18
#define KLINE_RX 19

// Display modes and brightness levels
enum display_mode_name {RTI_RGB, RTI_PAL, RTI_NTSC, RTI_OFF};
const char display_modes[] = {0x40, 0x45, 0x4C, 0x46};
const char brightness_levels[] = {0x20, 0x61, 0x62, 0x23, 0x64, 0x25, 0x26, 0x67, 0x68, 0x29, 0x2A, 0x2C, 0x6B, 0x6D, 0x6E, 0x2F};
int current_display_mode = RTI_NTSC;  //Display mode
char current_brightness_level = 13;   //Screen brightness

// Define button IDs
const uint8_t ENTER[] = {0x60, 0x7f};
const uint8_t BACK[] = {0x50, 0x7f};


// Timer variables
unsigned long lastSendTime1 = 0; // K-line message last send time
unsigned long sendDelay = 3000; // Delay between K-line + Canbus messages
unsigned long lastSendTime = 0; // Canbus message last send time
unsigned long startTime = 0;          //Start time for counting
const int rti_delay = 100;            //RTI message delay
const int durationInitial = 3000;     //Initial image duration

// Check conditions
bool send_brightness = true;
bool initialDisplayed = true; 
bool clearedScreen = false;  
bool waitForReply = false;
bool runDisplay = false;

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
float processMisfire1 (byte highbyte, byte lowByte) {
  int rawData =256 * highByte + lowByte;
  return rawData;
}
float processMisfire2 (byte highbyte, byte lowByte) {
  int rawData =256 * highByte + lowByte;
  return rawData;
}
float processMisfire3 (byte highbyte, byte lowByte) {
  int rawData =256 * highByte + lowByte;
  return rawData;
}
float processMisfire4 (byte highbyte, byte lowByte) {
  int rawData =256 * highByte + lowByte;
  return rawData;
}
float processMisfire5 (byte highbyte, byte lowByte) {
  int rawData =256 * highByte + lowByte;
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

float CoolantTemp = 0, Boost = 0, AFR = 0, AFRexpected = 0, MisfireCount = 0, Misfire1 = 0, Misfire2 = 0, Misfire3 = 0, Misfire4 = 0, Misfire5 = 0, IntakeAirTemp = 0;

Condition conditions[] = {
  {0x10, 0xD8, "CoolantTemp", &CoolantTemp, processCoolantTemp, {0xCD, 0x7A, 0xA6, 0x10, 0xD8, 0x01, 0x00, 0x00}},
  {0x12, 0x9D, "Boost", &Boost, processBoost, {0xCD, 0x7A, 0xA6, 0x12, 0x9D, 0x01, 0x00, 0x00}},
  {0x10, 0x34, "AFR", &AFR, processAFR, {0xCD, 0x7A, 0xA6, 0x10, 0x34, 0x01, 0x00, 0x00}},
  {0x12, 0xBB, "AFRexpected", &AFRexpected, processAFRexpected, {0xCD, 0x7A, 0xA6, 0x12, 0xBB, 0x01, 0x00, 0x00}},
  {0x10, 0xCA, "MisfireCount", &MisfireCount, processMisfireCount, {0xCD, 0x7A, 0xA6, 0x10, 0xCA, 0x01, 0x00, 0x00}},
  {0x10, 0x31, "Misfire1", &Misfire1, processMisfire1, {0xCD, 0x7A, 0xA6, 0x10, 0x31, 0x01, 0x00, 0x00}},
  {0x10, 0x3A, "Misfire2", &Misfire2, processMisfire2, {0xCD, 0x7A, 0xA6, 0x10, 0x3A, 0x01, 0x00, 0x00}},
  {0x10, 0x3C, "Misfire3", &Misfire3, processMisfire3, {0xCD, 0x7A, 0xA6, 0x10, 0x4A, 0x01, 0x00, 0x00}},
  {0x10, 0x45, "Misfire4", &Misfire4, processMisfire4, {0xCD, 0x7A, 0xA6, 0x10, 0x3C, 0x01, 0x00, 0x00}},
  {0x10, 0x4A, "Misfire5", &Misfire5, processMisfire5, {0xCD, 0x7A, 0xA6, 0x10, 0x45, 0x01, 0x00, 0x00}},
  {0x10, 0xCE, "IntakeAirTemp", &IntakeAirTemp, processIntakeAirTemp, {0xCD, 0x7A, 0xA6, 0x10, 0xCE, 0x01, 0x00, 0x00}}
};

const int conditionCount = sizeof(conditions) / sizeof(conditions[0]);

// Function to write data to RTI display
void rtiWrite(char byte) {
  Serial.print(byte);
  delay(rti_delay);
}

void dataLayout() {   //Data information
  TV.clear_screen();  
  TV.select_font(font8x8);
  TV.print(5, 0, "Coolant Temp C");
  TV.draw_line(0, 15, TV.hres() - 1, 15, WHITE);
  TV.print(5, 22, "Intake Air Temp C");
  TV.draw_line(0, 37, TV.hres() - 1, 37, WHITE);
  TV.print(5, 44, "Boost Pressure Bar");
  TV.draw_line(0, 59, TV.hres() - 1, 59, WHITE);
  TV.print(5, 66, "Requested AFR");
  TV.draw_line(0, 81, TV.hres() - 1, 81, WHITE);
  TV.print(5, 88, "Current AFR");
  TV.draw_line(0, 103, TV.hres() - 1, 103, WHITE);
  TV.draw_line(0, 104, TV.hres() - 1, 104, WHITE);
  TV.print(5, 135, "Misfire Counter 1 - 5 cyl");
}

void data() {           //Displayed current data
  TV.select_font(font8x8);
  char buffer[50];
  TV.print(200, 0, String(CoolantTemp).c_str());
  TV.print(200, 22, String(IntakeAirTemp).c_str());
  TV.print(200, 44, String(Boost).c_str());
  TV.print(200, 66, String(AFR).c_str());
  TV.print(200, 88, String(AFRexpected).c_str());
  sprintf(buffer, "%d    %d    %d    %d    %d", Misfire1, Misfire2, Misfire3, Misfire4, Misfire5);
  TV.print(30, 152, buffer);
}

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

  TV.begin(_NTSC, 240, 160); // Initialise TVout
  Serial.begin(2400);   // Start RTI serial communication
  Serial1.begin(10800);  // Start K-line communication MC33290
  //Serial2.begin(115200);  // Start serial communication MCP2515

  // Send initial messages on K-line and Can-bus to activate OBD2 port diagnostics
  sendKeepaliveMessage();
  delay(1000); 
  Serial.println("Initial K-line and Can-bus message sent.");

  Serial.println("Setup started.");

  // Initialize MCP2515
  if (CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("High-speed Canbus Initialized Successfully!");
  } else {
    Serial.println("Error Initializing High-speed Canbus...");
    while (1);  // Hang if MCP2515 initialization fails
  }
  CAN0.setMode(MCP_NORMAL);
  Serial.println("MCP2515 in Normal mode.");

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
      if (buf[6] == ENTER[0] && buf[7] == ENTER[1]) {
        //Serial.println("Enter Button Pressed");
        runDisplay = true;
      }
      else if (buf[6] == BACK[0] && buf[7] == BACK[1]) {
        //Serial.println("Back Button Pressed");
        runDisplay = false;
      }
    }
  }
  if (runDisplay) {
  //Serial.println("Display opens...");

  // Send RTI commands
  rtiWrite(display_modes[current_display_mode]);
  if (send_brightness)
    rtiWrite(brightness_levels[current_brightness_level]);
  else
    rtiWrite(0x40);
  rtiWrite(0x83);

  if (initialDisplayed) {
    // Show the initial picture for 5 seconds
    if (millis() - startTime <= durationInitial) {
      TV.bitmap(30, 0, Initial);
    }
    else {
      initialDisplayed = false;
    }
  }
  else {
    // Clear the screen once after transitioning
    if (!clearedScreen) {
      TV.clear_screen();
      dataLayout();
      clearedScreen = true; // Ensure the screen is only cleared once
    }
     unsigned char canbusMessage[8] = {0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 CAN0.sendMsgBuf(0x000FFFFE, 1, 8, canbusMessage);

for (int i = 0; i < conditionCount - 1; i++) {
    CAN0.sendMsgBuf(0x000FFFFE, 1, 8, conditions[i].requestMessage);
    if (CAN0.checkReceive() == CAN_MSGAVAIL) {
    CAN0.readMsgBuf(&rxId, &len, buf);
      if (rxId == 0x80800021 && buf[3] == conditions[i].byte4 && buf[4] == conditions[i].byte5) {
        *(conditions[i].data) = conditions[i].process(buf[5], buf[6]);
        //Serial.print(conditions[i].name);
        //Serial.print(": ");
        //Serial.println(*(conditions[i].data));
      }
    }
  }
    data();
  }
}
else {
  //Serial.println("Display off...");
  startTime = 0;
}
}
