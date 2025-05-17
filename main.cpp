#include "mbed.h"

// ------------------- Pin Definitions -------------------
DigitalOut latch(D4);
DigitalOut Clock(D7);
DigitalOut data(D8);

DigitalIn buttonReset(A1);   // S1
DigitalIn buttonVoltage(A3); // S3

AnalogIn potentiometer(A0);

// ------------------- Segment and Digit Data -------------------
const uint8_t SEGMENT_PATTERNS[10] = {
    static_cast<uint8_t>(~0x3F), // 0
    static_cast<uint8_t>(~0x06), // 1
    static_cast<uint8_t>(~0x5B), // 2
    static_cast<uint8_t>(~0x4F), // 3
    static_cast<uint8_t>(~0x66), // 4
    static_cast<uint8_t>(~0x6D), // 5
    static_cast<uint8_t>(~0x7D), // 6
    static_cast<uint8_t>(~0x07), // 7
    static_cast<uint8_t>(~0x7F), // 8
    static_cast<uint8_t>(~0x6F)  // 9
};

const uint8_t DIGIT_SELECT[4] = {0x01, 0x02, 0x04, 0x08};

// ------------------- Global State -------------------
volatile int g_seconds = 0;
volatile int g_minutes = 0;
volatile float g_minVoltage = 3.3f;
volatile float g_maxVoltage = 0.0f;

Ticker timerTicker;

// ------------------- Helper Functions -------------------
void shiftOut(uint8_t byte) {
    for (int i = 7; i >= 0; --i) {
        data = (byte >> i) & 0x01;
       Clock = 1;
       Clock = 0;
    }
}

void updateDisplay(uint8_t segmentData, uint8_t digitControl) {
    latch = 0;
    shiftOut(segmentData);
    shiftOut(digitControl);
    latch = 1;
}

void displayFourDigitNumber(int value, bool showDecimal = false, int decimalIndex = -1) {
    int digits[4] = {
        (value / 1000) % 10,
        (value / 100) % 10,
        (value / 10) % 10,
        value % 10
    };

    for (int i = 0; i < 4; ++i) {
        uint8_t seg = SEGMENT_PATTERNS[digits[i]];
        if (showDecimal && i == decimalIndex) {
            seg &= ~0x80; // Turn on decimal point (assuming active low)
        }
        updateDisplay(seg, DIGIT_SELECT[i]);
        ThisThread::sleep_for(0.002);
    }
}

// ------------------- ISR -------------------
void tickClock() {
    g_seconds++;
    if (g_seconds >= 60) {
        g_seconds = 0;
        g_minutes = (g_minutes + 1) % 100;
    }
}

// ------------------- Main -------------------
int main() {
    // Setup
    buttonReset.mode(PullUp);
    buttonVoltage.mode(PullUp);
    timerTicker.attach(&tickClock, 1); // Timer fires every second

    while (true) {
        // Handle reset
        if (!buttonReset.read()) {
            g_seconds = 0;
            g_minutes = 0;
            ThisThread::sleep_for(0.2); // Debounce
        }

        // Read voltage and update bounds
        float voltage = potentiometer.read() * 3.3f;
        g_minVoltage = fmin(g_minVoltage, voltage);
        g_maxVoltage = fmax(g_maxVoltage, voltage);

        // Display logic
        if (!buttonVoltage.read()) {
            int voltageValue = static_cast<int>(voltage * 100); // e.g., 3.30V → 330
            displayFourDigitNumber(voltageValue, true, 1);       // X.XX
        } else {
            int timeValue = g_minutes * 100 + g_seconds; // MMSS
            displayFourDigitNumber(timeValue);
        }
    }
}