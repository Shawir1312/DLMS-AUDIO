#include "rotary_encoder.h"

RotaryEncoder rotaryEncoder;

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

    // Use internal pull-ups
    pinMode(_clkPin, INPUT_PULLUP);
    pinMode(_dtPin,  INPUT_PULLUP);
    pinMode(_swPin,  INPUT_PULLUP);

    uint8_t msb = digitalRead(_clkPin);
    uint8_t lsb = digitalRead(_dtPin);
    _lastEncoded = (msb << 1) | lsb;

    _lastBtnReading = digitalRead(_swPin);
    _btnState = (_lastBtnReading == LOW);
}

void RotaryEncoder::handleEncoderIsr() {
    // Kept for backward compatibility if needed
}

void RotaryEncoder::update() {
    // 1. Safe polling-based quadrature decoding (Zero CPU crash/interrupt storm risk)
    uint8_t msb = digitalRead(_clkPin);
    uint8_t lsb = digitalRead(_dtPin);
    uint8_t encoded = (msb << 1) | lsb;

    if (encoded != _lastEncoded) {
        uint8_t sum = (_lastEncoded << 2) | encoded;
        if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
            _encoderDelta++;
        } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
            _encoderDelta--;
        }
        _lastEncoded = encoded;
    }

    // 2. Debounce and read Push Button
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
    int32_t val = _encoderDelta;
    // Step per 2 transitions for smooth, 1-click-per-detent feeling
    int32_t steps = val / 2;
    if (steps != 0) {
        _encoderDelta -= steps * 2;
    }
    return steps;
}

EncoderButtonEvent RotaryEncoder::getButtonEvent() {
    EncoderButtonEvent ev = _pendingButtonEvent;
    _pendingButtonEvent = BTN_NONE;
    return ev;
}
