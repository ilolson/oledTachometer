// Isaac Olson Addressable LED Tachometer
// Must be connected to vehicle ground to function correctly
// Board compatibility may vary, Originally designed to work on the raspberry pi pico
// Pin 6 is the signal pin
// Pin 4 is the brightness cycle pin

#include <FastLED.h>
#define NUM_LEDS 8
#define DATA_PIN 0
CRGB leds[NUM_LEDS];

/* OPTIONS */
const unsigned long resetTime = 100;  // in milliseconds low long after the last signal do you want the gauge to decay its value
const double decayRate = 0.998;       // Adjust this value for the rate of decay (0.995 means 0.5% decay per loop )
const int numReadings = 10;           // This is how 'stable' the rpm value is, adjust this to smooth out values
double multiplier = 30000.0;          // 30000.0 for 2 signals per rev, 60000.0 for 1 signal per rev, 120000.0 for 0.5 signal per rev
const int redline = 5;                // 5 is a 6000RPM redline, this determines when the gauge will turn all previous LEDS red (redline rpm / 1000) - 1,
const int maxRPM = 7999;              // Adjust the maximum rpm the display will read, default 7999, reccomended value 7999

volatile unsigned long ignCount = 0;
unsigned long lastTime = 0;
unsigned long currentTime = 0;
unsigned long timeDiff = 0;
int displayRefresh = 1;
unsigned long readings[numReadings];
unsigned long total = 0;
double rollingAvgGra = 7999;
double rpm = 7999;
int indexAVG = 0;
unsigned long lastInputTime = 0;
int brightnessState = 0;
double rollingAvgDis = 0;
unsigned long elapsedTime;
void changeBrightness() {
  if (digitalRead(4) == LOW) {
    if (brightnessState == 5) {
      brightnessState = 0;
    } else {
      brightnessState++;
    }
    if (brightnessState == 0) {
      FastLED.setBrightness(10);
    } else if (brightnessState == 1) {
      FastLED.setBrightness(25);
    } else if (brightnessState == 2) {
      FastLED.setBrightness(50);
    } else if (brightnessState == 3) {
      FastLED.setBrightness(100);
    } else if (brightnessState == 4) {
      FastLED.setBrightness(200);
    } else if (brightnessState == 5) {
      FastLED.setBrightness(3);
    }
    delay(333);
  }
}

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

void setup() {
  delay(100);
  Serial.begin(9600);
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.show();
  FastLED.setBrightness(10);
  pinMode(6, INPUT_PULLDOWN);
  pinMode(4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(6), isr, FALLING);
}

void loop() {

  changeBrightness();
  elapsedTime = millis() - lastInputTime;
  if (elapsedTime > resetTime && rollingAvgGra > 0) {
    rpm *= decayRate;
    updateRollingAverage(rpm);
    displayRefresh = 1;
  }
  update();
  if (displayRefresh == 1) {
    displayRefresh = 0;
    int krpm = rollingAvgGra / 1000;
    for (int i = 0; i <= 7; i++)
      if (i < krpm) {
        if (krpm > redline) {
          leds[i] = CRGB::Red;
        } else {

          // change color here

          // leds[i] = CRGB::Red;
          leds[i] = CRGB::Green;
          // leds[i] = CRGB::Blue;
          // leds[i] = CRGB::Purple;
          // leds[i] = CRGB::Aqua;
          // leds[i] = CRGB::Black;
          // leds[i] = CRGB::White;
        }
      } else {
        leds[i] = CRGB::Black;
      }
    double hrpm = fmod(rollingAvgGra, 1000.0);

    // continue changing color here

    // int hue = map(hrpm,0,1000,80,255); // red
    int hue = map(hrpm, 0, 1000, 255, 80);  // green
    // int hue = map(hrpm,0,1000,0,170); // blue
    // int hue = map(hrpm,0,1000,0,215); // Purple
    // int hue = map(hrpm,0,1000,0,145); // Aqua
    // int hue = map(hrpm, 0, 1000, 0, 255);  // Black

    leds[krpm].setHue(hue);
    FastLED.show();
  }
}
