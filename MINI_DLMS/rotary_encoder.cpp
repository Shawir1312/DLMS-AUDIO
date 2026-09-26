#include "rotary_encoder.h"

RotaryEncoder rotaryEncoder;

static void IRAM_ATTR encoderIsrTrampoline() {
    rotaryEncoder.handleEncoderIsr();
}

RotaryEncoder::RotaryEncoder()
    : _clkPin(ENCODER_CLK_PIN),
      _dtPin(ENCODER_DT_PIN),
      _swPin(ENCODER_SW_PIN),
      _encoderDelta(0),
      _lastEncoded(0),
      _lastBtnReading(HIGH),
      _btnState(false),
      _lastDebounceTime(0),
      _btnPressStartTime(0),
      _longPressDispatched(false),
      _pendingButtonEvent(BTN_NONE)
{
}

void RotaryEncoder::begin(uint8_t clk_pin, uint8_t dt_pin, uint8_t sw_pin) {
    _clkPin = clk_pin;
    _dtPin  = dt_pin;
    _swPin  = sw_pin;

    pinMode(_clkPin, INPUT_PULLUP);
    pinMode(_dtPin,  INPUT_PULLUP);
    pinMode(_swPin,  INPUT_PULLUP);

    uint8_t msb = digitalRead(_clkPin);
    uint8_t lsb = digitalRead(_dtPin);
    _lastEncoded = (msb << 1) | lsb;

    attachInterrupt(digitalPinToInterrupt(_clkPin), encoderIsrTrampoline, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_dtPin),  encoderIsrTrampoline, CHANGE);

    _lastBtnReading = digitalRead(_swPin);
    _btnState = (_lastBtnReading == LOW);
}

void IRAM_ATTR RotaryEncoder::handleEncoderIsr() {
    uint8_t msb = digitalRead(_clkPin);
    uint8_t lsb = digitalRead(_dtPin);
    uint8_t encoded = (msb << 1) | lsb;
    uint8_t sum = (_lastEncoded << 2) | encoded;

    // Standard Quadrature State Table
    // Transitions that indicate clockwise rotation
    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
        _encoderDelta++;
    }
    // Transitions that indicate counter-clockwise rotation
    else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
        _encoderDelta--;
    }

    _lastEncoded = encoded;
}

void RotaryEncoder::update() {
    // 1. Debounce and read Push Button
    bool reading = digitalRead(_swPin);
    unsigned long now = millis();

    if (reading != _lastBtnReading) {
        _lastDebounceTime = now;
        _lastBtnReading = reading;
    }

    if ((now - _lastDebounceTime) > 35) { // 35ms debounce
        bool isPressed = (reading == LOW);

        if (isPressed && !_btnState) {
            // Button just pressed down
            _btnState = true;
            _btnPressStartTime = now;
            _longPressDispatched = false;
        } else if (isPressed && _btnState) {
            // Button is being held down
            if (!_longPressDispatched && (now - _btnPressStartTime >= 800)) {
                _pendingButtonEvent = BTN_LONG_PRESSED;
                _longPressDispatched = true;
            }
        } else if (!isPressed && _btnState) {
            // Button just released
            _btnState = false;
            if (!_longPressDispatched && (now - _btnPressStartTime >= 30)) {
                _pendingButtonEvent = BTN_CLICKED;
            }
        }
    }
}

int32_t RotaryEncoder::getDelta() {
    noInterrupts();
    int32_t val = _encoderDelta;
    // Most EC11 encoders give 2 or 4 pulses per physical detent click
    // We step per 2 transitions for smooth, 1-click-per-detent feeling
    int32_t steps = val / 2;
    if (steps != 0) {
        _encoderDelta -= steps * 2;
    }
    interrupts();
    return steps;
}

EncoderButtonEvent RotaryEncoder::getButtonEvent() {
    EncoderButtonEvent ev = _pendingButtonEvent;
    _pendingButtonEvent = BTN_NONE;
    return ev;
}
