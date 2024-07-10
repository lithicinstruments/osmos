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

#include "Display.h"
#include <Arduino.h> // For random function

extern Adafruit_SSD1305 display;
extern float harmonicAmplitudes[];
extern float harmonicPanning[];
extern float baseFrequency;
extern int harmonicIndex;
extern int menuIndex;
extern MenuMode currentMenu;
extern float modulationMatrix[7][7];
extern CVMode cvAssignments[];
extern bool xySwapped;
extern float xyBiasX;
extern float xyBiasY;
extern const char *scaleNames[];
extern const char *waveformNames[];

struct Particle
{
    int x, y;
    int dx, dy;
    uint16_t color;
};

#define MAX_PARTICLES 50
Particle particles[MAX_PARTICLES];

struct Ripple
{
    int x, y;
    float radius;
    float speed;
    float amplitude;
    float life;
};

#define MAX_RIPPLES 10
Ripple ripples[MAX_RIPPLES];

void initDisplay()
{
    display.begin(SSD1305_SWITCHCAPVCC, OLED_ADDRESS);
    display.display(); // Initialize with a blank display
    delay(1000);       // Pause for 1 second
    display.clearDisplay();

    // Initialize particles
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        particles[i] = {random(128), random(64), random(3) - 1, random(3) - 1, WHITE};
    }

    // Initialize ripples
    for (int i = 0; i < MAX_RIPPLES; ++i)
    {
        ripples[i] = {random(128), random(64), 0, random(1, 5) / 10.0, harmonicAmplitudes[i % 7], 1.0};
    }
}

void drawPopupMenu(int index)
{
    display.clearDisplay();
    const char *popupItems[] = {"Option 1", "Option 2", "Option 3", "Option 4", "Option 5", "Option 6", "Option 7"};
    for (int i = 0; i < 7; ++i)
    {
        display.setCursor(0, i * 8);
        if (i == index)
        {
            display.print("> ");
        }
        else
        {
            display.print("  ");
        }
        display.print(popupItems[i]);
    }
    display.display();
}

void handlePopupSelection(int index)
{
    // Handle the selected option from the popup menu
    // Add your option handling code here
}

void drawWaveforms()
{
    display.clearDisplay();

    // Draw the combined waveform
    for (int x = 0; x < 128; ++x)
    {
        float sample = 0.0;
        for (int i = 0; i < 7; ++i)
        {
            sample += harmonicAmplitudes[i] * sin(2.0 * PI * (i + 1) * x / 128.0);
        }
        int y = 32 + (int)(sample * 16); // Center at 32, scale to 16 pixels
        display.drawPixel(x, y, WHITE);
    }

    // Display harmonic amplitudes and the current scale
    for (int i = 0; i < 7; ++i)
    {
        display.setCursor(0, i * 8);
        display.print("H");
        display.print(i + 1);
        display.print(": ");
        display.print(harmonicAmplitudes[i], 1);
        if (i == harmonicIndex)
        {
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

void drawAmplitudeBars()
{
    display.clearDisplay();

    for (int i = 0; i < 7; ++i)
    {
        int barHeight = (int)(harmonicAmplitudes[i] * 64);
        display.fillRect(i * 18, 64 - barHeight, 16, barHeight, WHITE);
        display.setCursor(i * 18, 64 - barHeight - 8);
        display.print(i + 1);
    }
    display.display();
}

void drawMenu()
{
    display.clearDisplay();
    display.setCursor(0, 0);

    if (currentMenu == SCALE_MENU)
    {
        display.print("Select Scale:");
        for (int i = 0; i < 4; ++i)
        {
            display.setCursor(0, (i + 1) * 8);
            display.print(scaleNames[i]);
            if (i == menuIndex)
            {
                display.print(" <-");
            }
        }
    }
    else if (currentMenu == FREQUENCY_MENU)
    {
        display.print("Select Base Freq:");
        for (int i = 0; i < 4; ++i)
        {
            display.setCursor(0, (i + 1) * 8);
            display.print(baseFrequencies[i], 1);
            if (i == menuIndex)
            {
                display.print(" <-");
            }
        }
    }
    else if (currentMenu == MODULATION_MENU)
    {
        display.print("Modulate H");
        display.print(harmonicIndex + 1);
        display.print(" with:");
        for (int i = 0; i < 7; ++i)
        {
            display.setCursor(0, (i + 1) * 8);
            display.print("H");
            display.print(i + 1);
            display.print(": ");
            display.print(modulationMatrix[i][harmonicIndex], 1);
            if (i == menuIndex)
            {
                display.print(" <-");
            }
        }
    }
    else if (currentMenu == PANNING_MENU)
    {
        display.print("Panning H");
        display.print(harmonicIndex + 1);
        for (int i = 0; i < 7; ++i)
        {
            display.setCursor(0, (i + 1) * 8);
            display.print("H");
            display.print(i + 1);
            display.print(": ");
            display.print(harmonicPanning[i], 1);
            if (i == menuIndex)
            {
                display.print(" <-");
            }
        }
    }
    else if (currentMenu == CV_MENU)
    {
        display.print("CV Assignments:");
        for (int i = 0; i < 4; ++i)
        {
            display.setCursor(0, (i + 1) * 8);
            display.print("CV");
            display.print(i + 1);
            display.print(": ");
            switch (cvAssignments[i])
            {
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
            if (i == menuIndex)
            {
                display.print(" <-");
            }
        }
    }
    else if (currentMenu == AMPLITUDE_MENU)
    {
        display.print("Amplitude Control:");
        drawAmplitudeBars();
    }
    else if (currentMenu == WAVEFORM_MENU)
    {
        display.print("Select Waveform:");
        for (int i = 0; i < 4; ++i)
        {
            display.setCursor(0, (i + 1) * 8);
            display.print(waveformNames[i]);
            if (i == menuIndex)
            {
                display.print(" <-");
            }
        }
    }
    else if (currentMenu == XY_DISPLAY)
    {
        display.print("XY Oscilloscope:");
        display.setCursor(0, 8);
        display.print("Swap Channels: ");
        display.print(xySwapped ? "On" : "Off");
        display.setCursor(0, 16);
        display.print("Bias X: ");
        display.print(xyBiasX, 1);
        display.setCursor(0, 24);
        display.print("Bias Y: ");
        display.print(xyBiasY, 1);
    }
    else if (currentMenu == RIPPLE_DISPLAY)
    {
        display.print("Ripple Effect:");
    }
    else if (currentMenu == OSCILLOSCOPE_DISPLAY)
    {
        display.print("Oscilloscope:");
    }
    display.display();
}

void drawParticles()
{
    display.clearDisplay();

    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        // Update particle position
        particles[i].x += particles[i].dx * harmonicAmplitudes[i % 7] * 2;
        particles[i].y += particles[i].dy * harmonicAmplitudes[i % 7] * 2;

        // Bounce off edges
        if (particles[i].x < 0 || particles[i].x >= 128)
            particles[i].dx = -particles[i].dx;
        if (particles[i].y < 0 || particles[i].y >= 64)
            particles[i].dy = -particles[i].dy;

        // Draw particle
        display.drawPixel(particles[i].x, particles[i].y, particles[i].color);
    }
    display.display();
}

void drawXYOscilloscope()
{
    display.clearDisplay();

    float x_signal[numSamples];
    float y_signal[numSamples];

    for (int i = 0; i < numSamples; ++i)
    {
        x_signal[i] = sin(2.0 * PI * i / numSamples);
        y_signal[i] = cos(2.0 * PI * i / numSamples);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        int x = (x_signal[i] + xyBiasX) * (128 / 2) + (128 / 2);
        int y = (y_signal[i] + xyBiasY) * (64 / 2) + (64 / 2);

        if (xySwapped)
        {
            int temp = x;
            x = y;
            y = temp;
        }

        if (x >= 0 && x < 128 && y >= 0 && y < 64)
        {
            display.drawPixel(x, y, WHITE);
        }
    }

    display.display();
}

void drawRippleEffect()
{
    display.clearDisplay();

    for (int i = 0; i < MAX_RIPPLES; ++i)
    {
        Ripple &ripple = ripples[i];

        ripple.radius += ripple.speed;
        ripple.life -= 0.05;

        if (ripple.life <= 0)
        {
            ripple.radius = 0;
            ripple.x = random(128);
            ripple.y = random(64);
            ripple.speed = random(1, 5) / 10.0;
            ripple.amplitude = harmonicAmplitudes[random(7)];
            ripple.life = 1.0;
        }

        for (int angle = 0; angle < 360; ++angle)
        {
            float rad = radians(angle);
            int x = int(ripple.x + ripple.radius * cos(rad));
            int y = int(ripple.y + ripple.radius * sin(rad));
            if (x >= 0 && x < 128 && y >= 0 && y < 64)
            {
                display.drawPixel(x, y, WHITE * ripple.life);
            }
        }
    }

    display.display();
}

void drawWaveformOscilloscope()
{
    display.clearDisplay();

    for (int x = 0; x < 128; ++x)
    {
        float sample = 0.0;
        for (int i = 0; i < 7; ++i)
        {
            sample += harmonicAmplitudes[i] * sin(2.0 * PI * (i + 1) * x / 128.0);
        }
        int y = 32 + (int)(sample * 16); // Center at 32, scale to 16 pixels
        display.drawPixel(x, y, WHITE);
    }

    display.display();
}

// Function to draw the bitmap on the display
void drawBitmap(const unsigned char *bitmap, uint8_t w, uint8_t h) {
    display.clearDisplay();
    display.drawBitmap((display.width() - w) / 2, (display.height() - h) / 2, bitmap, w, h, SSD1305_YELLOW);
    display.display();
}
