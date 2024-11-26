#include <TVout.h>
#include <fontALL.h>
#include <SPI.h>
#include <math.h>

enum display_mode_name {RTI_RGB, RTI_PAL, RTI_NTSC, RTI_OFF};
const char display_modes[] = {0x40, 0x45, 0x4C, 0x46};
const char brightness_levels[] = {0x20, 0x61, 0x62, 0x23, 0x64, 0x25, 0x26, 0x67, 0x68, 0x29, 0x2A, 0x2C, 0x6B, 0x6D, 0x6E, 0x2F};

//int current_display_mode = RTI_OFF;
int current_display_mode = RTI_NTSC;
bool send_brightness = true;
char current_brightness_level = 13;

//delay between bytes, ms
const int rti_delay = 10;
int once = 1;

TVout TV;
const int radius = 40;
const int arcThickness = 1;
const int centerX = 50; // Center of the screen
const int centerY = 80; 
const int startAngle = 150; // Start of the arc (degrees)
const int endAngle = 390;   // End of the arc (degrees)
int temperature = 25;       // Example temperature value

// Function to convert degrees to radians
float toRadians(float degrees) {
  return degrees * (PI / 180.0);
}

// Draw a thick arc by layering multiple outlines
void drawArc(int x, int y, int r, int start, int end, int thickness) {
  for (int t = 0; t < thickness; t++) {
    for (int angle = start; angle < end; angle++) {
      int x0 = x + cos(toRadians(angle)) * (r + t);
      int y0 = y - sin(toRadians(angle)) * (r + t); // Negative y due to screen coordinates
      TV.set_pixel(x0, y0, 1);
    }
  }
}

void drawGauge() {
  TV.clear_screen();

  // Draw the arc outline
  drawArc(centerX, centerY, radius, startAngle, endAngle, arcThickness);

  // Draw "C" below the temperature
 // TV.print(centerX - 2, centerY + 8, "C");
}

void setup() {
  // put your setup code here, to run once:
TV.begin(_NTSC, 720, 480);
Serial.begin(2400);
TV.clear_screen();
drawGauge();
}

void loop() {
   rtiWrite(display_modes[current_display_mode]);
  
  if (send_brightness)
    rtiWrite(brightness_levels[current_brightness_level]);
  else
    rtiWrite(0x40);
    
    rtiWrite(0x83);
  // put your main code here, to run repeatedly:
//TV.clear_screen();
 TV.select_font(font8x8); // Small font for compact display
 TV.print(centerX - 8, centerY - 8, String(temperature).c_str()); // Offset for alignment
//delay(50);
temperature = random(1000, 1800); // Simulate temperature fluctuation
//TV.print();
//TV.bitmap(10, 10, Pi);
//delay(60);
}

void rtiWrite(char byte) {
  Serial.print(byte);
  delay(rti_delay);
}
