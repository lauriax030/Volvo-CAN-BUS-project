#include <TVout.h>
#include <fontALL.h>
#include <SPI.h>
#include <math.h>

#define DEG2RAD 0.0174532925

// TVout instance
TVout TV;

// Display modes and brightness levels
enum display_mode_name {RTI_RGB, RTI_PAL, RTI_NTSC, RTI_OFF};
const char display_modes[] = {0x40, 0x45, 0x4C, 0x46};
const char brightness_levels[] = {0x20, 0x61, 0x62, 0x23, 0x64, 0x25, 0x26, 0x67, 0x68, 0x29, 0x2A, 0x2C, 0x6B, 0x6D, 0x6E, 0x2F};

// Settings
int current_display_mode = RTI_NTSC;
bool send_brightness = true;
char current_brightness_level = 13;
const int rti_delay = 10;

// Arc and temperature settings
const int maxTemperature = 80; // Maximum temperature
int temperature = 0;           // Current temperature
int previousTemperature = 0;   // Track previous temperature
const int start_angle = -120;  // Arc start angle
const int arcThickness = 5;    // Arc thickness
const int arcRadiusX = 40;     // Arc horizontal radius
const int arcRadiusY = 40;     // Arc vertical radius

void setup() {
  TV.begin(_NTSC, 720, 480); // Initialize TVout
  Serial.begin(2400);        // Initialize serial communication
  TV.clear_screen();         // Clear the screen
}

void loop() {
  // Send RTI commands
  rtiWrite(display_modes[current_display_mode]);
  if (send_brightness)
    rtiWrite(brightness_levels[current_brightness_level]);
  else
    rtiWrite(0x40);
  rtiWrite(0x83);

  temperature = random(0, maxTemperature);

  // Efficiently update the arc (draw and erase only necessary parts)
  updateArc(60, 60, 40, 40, 5, -120, previousTemperature, temperature);

  // Update displayed temperature value
  TV.select_font(font8x8);
  TV.print(50, 50, String(temperature).c_str());

  // Save current temperature for the next loop
  previousTemperature = temperature;
}


// Function to write data to RTI display
void rtiWrite(char byte) {
  Serial.print(byte);
  delay(rti_delay);
}

// Function to draw or erase an arc
int fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, uint8_t color) {
  byte seg = 3; // Segment angle size in degrees

  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x0 = sx * (rx - w) + x;
  uint16_t y0 = sy * (ry - w) + y;
  uint16_t x1 = sx * rx + x;
  uint16_t y1 = sy * ry + y;

  for (int i = 0; i < seg_count; i++) {
    float angle = (start_angle + i * seg - 90) * DEG2RAD;
    float next_angle = (start_angle + (i + 1) * seg - 90) * DEG2RAD;

    int x2 = cos(next_angle) * (rx - w) + x;
    int y2 = sin(next_angle) * (ry - w) + y;
    int x3 = cos(next_angle) * rx + x;
    int y3 = sin(next_angle) * ry + y;

    // Fill the two triangles that make up the arc segment
    fillTriangleTV(x0, y0, x1, y1, x2, y2, color);
    fillTriangleTV(x1, y1, x2, y2, x3, y3, color);

    // Update start points for the next segment
    x0 = x2;
    y0 = y2;
    x1 = x3;
    y1 = y3;
  }

  return seg_count;
}


// Function to fill a triangle on the TV screen
void fillTriangleTV(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color) {
  // Sort vertices by y-coordinate (y0 <= y1 <= y2)
  if (y0 > y1) { swap(x0, x1); swap(y0, y1); }
  if (y1 > y2) { swap(x1, x2); swap(y1, y2); }
  if (y0 > y1) { swap(x0, x1); swap(y0, y1); }

  // Compute edge slopes
  float dx1 = (y1 - y0) > 0 ? (float)(x1 - x0) / (y1 - y0) : 0;
  float dx2 = (y2 - y0) > 0 ? (float)(x2 - x0) / (y2 - y0) : 0;
  float dx3 = (y2 - y1) > 0 ? (float)(x2 - x1) / (y2 - y1) : 0;

  float xStart = x0, xEnd = x0;

  // Draw first segment
  for (int y = y0; y < y1; y++) {
    TV.draw_line((int)xStart, y, (int)xEnd, y, color);
    xStart += dx1;
    xEnd += dx2;
  }

  // Adjust for second segment
  xStart = x1;

  // Draw second segment
  for (int y = y1; y <= y2; y++) {
    TV.draw_line((int)xStart, y, (int)xEnd, y, color);
    xStart += dx3;
    xEnd += dx2;
  }
}

// Helper function to swap two integers
void swap(int &a, int &b) {
  int temp = a;
  a = b;
  b = temp;
}

void updateArc(int x, int y, int rx, int ry, int w, int start_angle, int previous_temp, int current_temp) {
  // Convert temperatures to segment counts
  int previous_segments = map(previous_temp, 0, maxTemperature, 0, 80); // Convert temperature to segments
  int current_segments = map(current_temp, 0, maxTemperature, 0, 80);   // Convert temperature to segments

  if (current_segments > previous_segments) {
    // Draw new segments (only the difference)
    fillArc(x, y, start_angle + previous_segments * 3, current_segments - previous_segments, rx, ry, w, WHITE);
  } else if (current_segments < previous_segments) {
    // Erase excess segments (only the difference)
    fillArc(x, y, start_angle + current_segments * 3, previous_segments - current_segments, rx, ry, w, BLACK);
  }
}
