// Isaac Olson OLED Tachometer
// Must be connected to vehicle ground to function correctly
// Board compatibility may vary, tested on Arduino Nano Every
// Pin 6 is the signal pin

/* OPTIONS */
const unsigned long resetTime = 100;  // in millis low long after the last signal do you want the gauge to start decaying its value
const double decayRate = .8;          // Adjust this value for the rate of decay (0.9 means 10% decay per loop )
const int numReadings = 10;           // This is how 'stable' the rpm value is, adjust this to smooth out values
const int maxRPM = 7999;              // Adjust the maximum rpm the display will read, default 7999, recomended max 9999
const int showShiftInd = 5500;        // The value the onboard LED and shift indicator will turn on
double multiplier = 300000.0;         // 300000.0 for 2 signals per rev, 600000.0 for 1 signal per rev, 1200000.0 for 0.5 signal per rev

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int barWidth;
volatile unsigned long ignCount = 0;
unsigned long lastTime = 0;
int displayRefresh = 1;
unsigned long readings[numReadings];
unsigned long total = 0;
int rollingAvgGra = 1;
double rollingAvgDis;
int rpm = 0;
int indexAVG = 0;
unsigned long lastInputTime = 0;
unsigned long currentTime = 0;
unsigned long timeDiff = 0;
unsigned long elapsedTime = 0;

void isr() {
  ignCount++;
  lastInputTime = millis();
}

void update() {
  currentTime = millis();
  timeDiff = currentTime - lastTime;
  if (ignCount > 1) {
    rpm = (multiplier) / timeDiff * ignCount;
    rpm = constrain(rpm, 0, maxRPM);
    ignCount = 0;
    updateRollingAverage(rpm);
    displayRefresh = 1;
    lastTime = millis();
  }
}

void updateRollingAverage(double rpm) {
  total -= readings[indexAVG];
  readings[indexAVG] = rpm;
  total += rpm;
  indexAVG = (indexAVG + 1) % numReadings;
  rollingAvgGra = total / numReadings;
  rollingAvgDis = rollingAvgGra / 1000.0;
}

void displayGraph() {
  barWidth = map(rollingAvgGra, 0, maxRPM, 1, SCREEN_WIDTH);
  display.fillRect(0, 0, barWidth, 15, SSD1306_WHITE);
}

void displayChar() {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(2, 20);
  display.print("RPM");
  if (rollingAvgGra > showShiftInd) {
    display.setTextSize(4);
    display.setCursor(2, 32);
    display.print(rollingAvgDis, 1);
    display.setTextSize(3);
    display.print(" ");
    display.setTextSize(6);
    display.print("^");
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    display.setTextSize(4);
    display.setCursor(2, 32);
    display.print(rollingAvgDis, 3);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void setup() {
  delay(100);
  //Serial.begin(115200);
  Wire.begin();
  Wire.beginTransmission(SCREEN_ADDRESS);
  pinMode(6, INPUT_PULLDOWN);
  pinMode(LED_BUILTIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(6), isr, RISING);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
  }
}

void loop() {
  elapsedTime = millis() - lastInputTime;
  if (elapsedTime > resetTime && rollingAvgGra > 0) {
    rpm *= decayRate;
    updateRollingAverage(rpm);
    displayRefresh = 1;
  }
  update();
  if (displayRefresh == 1) {
    displayRefresh = 0;
    display.clearDisplay();
    displayChar();
    displayGraph();
    display.display();
    // Serial.println(rollingAvgGra, 0);
  }
}
