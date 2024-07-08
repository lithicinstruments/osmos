/*
 * File: display.cpp
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

#include "display.h"

// Create SSD1305 display instance
Adafruit_SSD1305 display(128, 64, &Wire, OLED_RESET);

extern const char* scaleNames[];
extern const char* waveformNames[];
extern const float harmonicAmplitudes[];
extern const float baseFrequency;
extern const MenuMode currentMenu;
extern const int harmonicIndex;
extern const int menuIndex;

void initDisplay() {
  display.begin(SSD1305_SWITCHCAPVCC, OLED_ADDRESS);
  display.display(); // Initialize with a blank display
  delay(1000); // Pause for 1 second
  display.clearDisplay();
}

void drawWaveforms() {
  display.clearDisplay();

  // Draw the combined waveform
  for (int x = 0; x < 128; ++x) {
    float sample = 0.0;
    for (int i = 0; i < 7; ++i) {
      sample += harmonicAmplitudes[i] * sin(2.0 * PI * (i + 1) * x / 128.0);
    }
    int y = 32 + (int)(sample * 16); // Center at 32, scale to 16 pixels
    display.drawPixel(x, y, WHITE);
  }

  // Display harmonic amplitudes and the current scale
  for (int i = 0; i < 7; ++i) {
    display.setCursor(0, i * 8);
    display.print("H");
    display.print(i + 1);
    display.print(": ");
    display.print(harmonicAmplitudes[i], 1);
    if (i == harmonicIndex) {
      display.print(" <-");
    }
  }

  display.setCursor(0, 56);
  display.print("Scale: ");
  display.print(scaleNames[currentMenu]);
  display.setCursor(64, 56);
  display.print("Freq: ");
  display.print(baseFrequency, 1);
  display.display();
}

void drawAmplitudeBars() {
  display.clearDisplay();
  
  for (int i = 0; i < 7; ++i) {
    int barHeight = (int)(harmonicAmplitudes[i] * 64);
    display.fillRect(i * 18, 64 - barHeight, 16, barHeight, WHITE);
    display.setCursor(i * 18, 64 - barHeight - 8);
    display.print(i + 1);
  }
  display.display();
}

void drawMenu() {
  display.clearDisplay();
  display.setCursor(0, 0);
  
  if (currentMenu == SCALE_MENU) {
    display.print("Select Scale:");
    for (int i = 0; i < 4; ++i) {
      display.setCursor(0, (i + 1) * 8);
      display.print(scaleNames[i]);
      if (i == menuIndex) {
        display.print(" <-");
      }
    }
  } else if (currentMenu == FREQUENCY_MENU) {
    display.print("Select Base Freq:");
    for (int i = 0; i < 4; ++i) {
      display.setCursor(0, (i + 1) * 8);
      display.print(baseFrequencies[i], 1);
      if (i == menuIndex) {
        display.print(" <-");
      }
    }
  } else if (currentMenu == MODULATION_MENU) {
    display.print("Modulate H");
    display.print(harmonicIndex + 1);
    display.print(" with:");
    for (int i = 0; i < 7; ++i) {
      display.setCursor(0, (i + 1) * 8);
      display.print("H");
      display.print(i + 1);
      display.print(": ");
      display.print(modulationMatrix[i][harmonicIndex], 1);
      if (i == menuIndex) {
        display.print(" <-");
      }
    }
  } else if (currentMenu == PANNING_MENU) {
    display.print("Panning H");
    display.print(harmonicIndex + 1);
    for (int i = 0; i < 7; ++i) {
      display.setCursor(0, (i + 1) * 8);
      display.print("H");
      display.print(i + 1);
      display.print(": ");
      display.print(harmonicPanning[i], 1);
      if (i == menuIndex) {
        display.print(" <-");
      }
    }
  } else if (currentMenu == CV_MENU) {
    display.print("CV Assignments:");
    for (int i = 0; i < 4; ++i) {
      display.setCursor(0, (i + 1) * 8);
      display.print("CV");
      display.print(i + 1);
      display.print(": ");
      switch (cvAssignments[i]) {
        case NONE:
          display.print("None");
          break;
        case LIN_FM:
          display.print("Linear FM");
          break;
        case EXP_FM:
          display.print("Exponential FM");
          break;
        case AMPLITUDE:
          display.print("Amplitude");
          break;
        case PITCH_1V_OCT:
          display.print("Pitch (1V/oct)");
          break;
      }
      if (i == menuIndex) {
        display.print(" <-");
      }
    }
  } else if (currentMenu == AMPLITUDE_MENU) {
    display.print("Amplitude Control:");
    drawAmplitudeBars();
  } else if (currentMenu == WAVEFORM_MENU) {
    display.print("Select Waveform:");
    for (int i = 0; i < 4; ++i) {
      display.setCursor(0, (i + 1) * 8);
      display.print(waveformNames[i]);
      if (i == menuIndex) {
        display.print(" <-");
      }
    }
  }
  display.display();
}
