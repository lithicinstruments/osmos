/*
 * File: dac.cpp
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

#include "dac.h"

// Create MCP4725 DAC instances
Adafruit_MCP4725 dac1;
Adafruit_MCP4725 dac2;
Adafruit_MCP4725 dacStereo;
Adafruit_MCP4725 dacWave[7]; // DACs for individual wave outputs

void initDACs() {
  dac1.begin(0x60); // Default I2C address for MCP4725
  dac2.begin(0x61); // Second I2C address for MCP4725
  dacStereo.begin(0x62); // Third I2C address for MCP4725
  for (int i = 0; i < 7; ++i) {
    dacWave[i].begin(0x63 + i); // Assuming sequential I2C addresses for individual wave outputs
  }
}

void outputToDACs(float leftSample, float rightSample, float stereoSample, float waveSamples[]) {
  // Convert the sample values to DAC output range
  int dacValueLeft = (int)((leftSample + 1.0) * 127.5); // Convert to 0-255 range
  int dacValueRight = (int)((rightSample + 1.0) * 127.5); // Convert to 0-255 range
  int dacValueStereo = (int)((stereoSample + 1.0) * 2047.5); // Convert to 0-4095 range

  // Output the sample values to the DACs
  dac_output_voltage(DAC_PIN_1, dacValueLeft);
  dac_output_voltage(DAC_PIN_2, dacValueRight);
  dacStereo.setVoltage(dacValueStereo, false);

  // Output individual wave samples to DACs
  for (int i = 0; i < 7; ++i) {
    int dacValueWave = (int)((waveSamples[i] + 1.0) * 2047.5); // Convert to 0-4095 range
    dacWave[i].setVoltage(dacValueWave, false);
  }
}
