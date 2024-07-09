/*
 * Harmonic oscillator for ESP32-based microcontroller to generate a sine wave and six additional harmonics,
 * controllable via a digital encoder. Displays resulting waveforms on a 128x64 Adafruit_SSD1305-based display,
 * with selectable base frequency and musical scales.
 *
 * Author: Tyler Reckart (tyler.reckart@gmail.com)
 * Copyright 2024
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * See http://creativecommons.org/licenses/MIT/ for more information.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <RotaryEncoder.h>
#include "display.h"
#include "dac.h"

// Define the rotary encoder pins
#define ENCODER_PIN_A 32
#define ENCODER_PIN_B 33
#define ENCODER_BUTTON_PIN 34

// Define the ADC input pins for CV
#define CV_PIN_1 34
#define CV_PIN_2 35
#define CV_PIN_3 36
#define CV_PIN_4 39

// Create rotary encoder instance
RotaryEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, RotaryEncoder::LatchMode::FOUR3);

// Harmonic control variables
int harmonicIndex = 0;
float harmonicAmplitudes[7] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float harmonicPanning[7] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5}; // 0.0 is full left, 1.0 is full right
float baseFrequency = 440.0; // Base frequency set to A4 (440 Hz)
int baseFrequencyIndex = 1;  // Default to 440 Hz

// Timer for the sine wave generation
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Waveform parameters
const int sampleRate = 1000; // Adjust as needed
const int numSamples = 256;
int sampleIndex = 0;

// Sine wave table
float sineTable[numSamples];

// Menu and scale settings
enum MenuMode { SCALE_MENU, FREQUENCY_MENU, HARMONIC_MENU, MODULATION_MENU, PANNING_MENU, CV_MENU, AMPLITUDE_MENU, WAVEFORM_MENU, PARTICLE_DISPLAY, XY_DISPLAY, RIPPLE_DISPLAY, OSCILLOSCOPE_DISPLAY, DEFAULT_VIEW };
MenuMode currentMenu = DEFAULT_VIEW;
int menuIndex = 0;
bool inMenu = false;
bool inPopupMenu = false;

const char* scaleNames[] = {"Major", "Minor", "Natural Harmonic", "Pentatonic"};
const float baseFrequencies[] = {220.0, 440.0, 880.0, 1760.0};
const char* waveformNames[] = {"Sine", "Saw", "Triangle", "Pulse"};

// Modulation matrix
float modulationMatrix[7][7] = {0}; // 7 harmonics modulating each other

// CV input assignments
enum CVMode { NONE, LIN_FM, EXP_FM, AMPLITUDE, PITCH_1V_OCT };
CVMode cvAssignments[4] = {NONE, NONE, NONE, NONE};

// Base waveform type
enum WaveformType { SINE, SAW, TRIANGLE, PULSE };
WaveformType currentWaveform = SINE;

// XY Oscilloscope settings
bool xySwapped = false;
float xyBiasX = 0.0;
float xyBiasY = 0.0;

// Timer interrupt service routine to generate waveforms
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);

  float leftSample = 0.0;
  float rightSample = 0.0;
  float stereoSample = 0.0;
  float waveSamples[7] = {0.0};

  // Read CV inputs
  float cvValues[4];
  cvValues[0] = analogRead(CV_PIN_1) / 4095.0;
  cvValues[1] = analogRead(CV_PIN_2) / 4095.0;
  cvValues[2] = analogRead(CV_PIN_3) / 4095.0;
  cvValues[3] = analogRead(CV_PIN_4) / 4095.0;

  // Calculate the sample value for each harmonic
  for (int i = 0; i < 7; ++i) {
    // Apply modulation from other harmonics
    float modulatedFrequency = baseFrequency * (i + 1);
    for (int j = 0; j < 7; ++j) {
      modulatedFrequency += modulationMatrix[j][i] * harmonicAmplitudes[j];
    }

    // Apply CV inputs
    for (int cvIndex = 0; cvIndex < 4; ++cvIndex) {
      switch (cvAssignments[cvIndex]) {
        case LIN_FM:
          modulatedFrequency += cvValues[cvIndex] * baseFrequency;
          break;
        case EXP_FM:
          modulatedFrequency *= pow(2, cvValues[cvIndex]);
          break;
        case AMPLITUDE:
          harmonicAmplitudes[i] *= cvValues[cvIndex];
          break;
        case PITCH_1V_OCT:
          modulatedFrequency *= pow(2, cvValues[cvIndex] - 1); // Assuming 1V/oct
          break;
        case NONE:
        default:
          break;
      }
    }

    // Generate the base waveform sample
    float harmonicSample;
    switch (currentWaveform) {
      case SINE:
        harmonicSample = harmonicAmplitudes[i] * sin(2.0 * PI * (sampleIndex * modulatedFrequency / sampleRate));
        break;
      case SAW:
        harmonicSample = harmonicAmplitudes[i] * (2.0 * (float)(sampleIndex % numSamples) / numSamples - 1.0);
        break;
      case TRIANGLE:
        harmonicSample = harmonicAmplitudes[i] * (2.0 * abs(2.0 * (float)(sampleIndex % numSamples) / numSamples - 1.0) - 1.0);
        break;
      case PULSE:
        harmonicSample = harmonicAmplitudes[i] * ((sampleIndex % numSamples) < (numSamples / 2) ? 1.0 : -1.0);
        break;
    }

    float pan = harmonicPanning[i];
    leftSample += harmonicSample * (1.0 - pan);
    rightSample += harmonicSample * pan;
    stereoSample += harmonicSample; // Mixed for stereo output
    waveSamples[i] = harmonicSample; // Individual wave output
  }

  // Output the sample values to the DACs
  outputToDACs(leftSample, rightSample, stereoSample, waveSamples);

  // Increment the sample index
  sampleIndex = (sampleIndex + 1) % numSamples;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  Serial.begin(115200);

  // Initialize the DACs
  initDACs();

  // Initialize the display
  initDisplay();

  // Generate the sine wave table
  for (int i = 0; i < numSamples; ++i) {
    sineTable[i] = sin(2.0 * PI * i / numSamples);
  }

  // Initialize the rotary encoder
  encoder.begin();
  pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);

  // Initialize CV input pins
  pinMode(CV_PIN_1, INPUT);
  pinMode(CV_PIN_2, INPUT);
  pinMode(CV_PIN_3, INPUT);
  pinMode(CV_PIN_4, INPUT);

  // Set up the timer interrupt for sine wave generation
  timer = timerBegin(0, 80, true); // Timer 0, prescaler 80, count up
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000 / sampleRate, true); // 1 second / sampleRate
  timerAlarmEnable(timer);
}

// Quantize the harmonics based on the selected musical scale
void quantizeHarmonics() {
  // Define the scales as arrays of frequency ratios
  float majorScale[] = {1.0, 1.122, 1.26, 1.335, 1.5, 1.682, 1.888};
  float minorScale[] = {1.0, 1.122, 1.189, 1.335, 1.5, 1.587, 1.782};
  float naturalHarmonicScale[] = {1.0, 1.125, 1.25, 1.375, 1.5, 1.625, 1.75};
  float pentatonicScale[] = {1.0, 1.125, 1.25, 1.5, 1.75, 2.0, 2.25};

  float* selectedScale;

  // Select the appropriate scale based on the current menu
  switch (currentMenu) {
    case SCALE_MENU:
      selectedScale = majorScale;
      break;
    case MINOR:
      selectedScale = minorScale;
      break;
    case NATURAL_HARMONIC:
      selectedScale = naturalHarmonicScale;
      break;
    case PENTATONIC:
      selectedScale = pentatonicScale;
      break;
  }

  // Apply the selected scale to the harmonic amplitudes
  for (int i = 0; i < 7; ++i) {
    harmonicAmplitudes[i] = selectedScale[i];
  }
}

void loop() {
  encoder.tick();

  if (inMenu) {
    // Navigate the menu
    static int lastMenuPos = 0;
    int newMenuPos = encoder.getPosition();
    if (newMenuPos != lastMenuPos) {
      menuIndex = (menuIndex + (newMenuPos - lastMenuPos)) % 12; // Updated to 12 for new menu items
      if (menuIndex < 0) menuIndex += 12;
      lastMenuPos = newMenuPos;
      drawMenu();
    }

    // Select menu item with encoder button
    static unsigned long lastButtonPress = 0;
    if (digitalRead(ENCODER_BUTTON_PIN) == LOW && millis() - lastButtonPress > 300) {
      inMenu = false;
      inPopupMenu = true;
      lastButtonPress = millis();
    }
  } else if (inPopupMenu) {
    // Handle popup menu interactions
    // Implementation for each mode's options goes here
  } else {
    // Default view: loop through display modes
    static int displayModeIndex = 0;
    static int lastPos = 0;
    int newPos = encoder.getPosition();
    if (newPos != lastPos) {
      displayModeIndex = (displayModeIndex + (newPos - lastPos)) % 12; // Number of display modes
      if (displayModeIndex < 0) displayModeIndex += 12;
      lastPos = newPos;

      // Redraw the selected display mode
      switch (displayModeIndex) {
        case PARTICLE_DISPLAY:
          drawParticles();
          break;
        case XY_DISPLAY:
          drawXYOscilloscope();
          break;
        case RIPPLE_DISPLAY:
          drawRippleEffect();
          break;
        case OSCILLOSCOPE_DISPLAY:
          drawWaveformOscilloscope();
          break;
        default:
          drawWaveforms();
          break;
      }
    }

    // Change to popup menu mode with long press of encoder button
    static unsigned long lastButtonPress = 0;
    if (digitalRead(ENCODER_BUTTON_PIN) == LOW && millis() - lastButtonPress > 1000) {
      inMenu = true;
      drawMenu();
      lastButtonPress = millis();
    }
  }

  delay(100); // Update display every 100 ms
}
