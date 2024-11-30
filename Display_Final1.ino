#include <TVout.h>
#include <fontALL.h>
#include <SPI.h>
#include "Initial.h"

// TVout instance
TVout TV;

// Display modes and brightness levels
enum display_mode_name {RTI_RGB, RTI_PAL, RTI_NTSC, RTI_OFF};
const char display_modes[] = {0x40, 0x45, 0x4C, 0x46};
const char brightness_levels[] = {0x20, 0x61, 0x62, 0x23, 0x64, 0x25, 0x26, 0x67, 0x68, 0x29, 0x2A, 0x2C, 0x6B, 0x6D, 0x6E, 0x2F};

// Settings
int current_display_mode = RTI_NTSC;  //Display mode
const int rti_delay = 100;            //RTI message delay
const int durationInitial = 3000;     //Initial image duration
char current_brightness_level = 13;   //Screen brightness
unsigned long startTime = 0;          //Start time for counting
bool send_brightness = true;
bool initialDisplayed = true; 
bool clearedScreen = false;  

int temperature = 0;           // Demo test temperature

void setup() {
  TV.begin(_NTSC, 240, 160); // Initialize TVout
  Serial.begin(2400);        // Initialize serial communication
}

void loop() {
  // Send RTI commands
  rtiWrite(display_modes[current_display_mode]);
  if (send_brightness)
    rtiWrite(brightness_levels[current_brightness_level]);
  else
    rtiWrite(0x40);
  rtiWrite(0x83);

  temperature = random(40, 90);

  if (initialDisplayed) {
    // Show the initial picture for 5 seconds
    if (millis() - startTime <= durationInitial) {
      TV.bitmap(30, 0, Initial);
    } else {
      // Transition after 5 seconds
      initialDisplayed = false;
    }
  } else {
    // Clear the screen once after transitioning
    if (!clearedScreen) {
      TV.clear_screen();
      dataLayout();
      clearedScreen = true; // Ensure the screen is only cleared once
    }
    data();
  }  
}

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
  TV.print(30, 152, "1    2    3    4    5");
}

void data() {           //Displayed current data
  TV.select_font(font8x8);
  TV.print(200, 0, String(temperature).c_str());
  TV.print(200, 22, String(temperature).c_str());
  TV.print(200, 44, String(temperature).c_str());
  TV.print(200, 66, String(temperature).c_str());
  TV.print(200, 88, String(temperature).c_str());
}
