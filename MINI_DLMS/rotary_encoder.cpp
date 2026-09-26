#include "rotary_encoder.h"

RotaryEncoder rotaryEncoder;

RotaryEncoder::RotaryEncoder()
    : _clkPin(ENCODER_CLK_PIN),
      _dtPin(ENCODER_DT_PIN),
      _swPin(ENCODER_SW_PIN),
      _enabled(ENCODER_PHYSICAL_ATTACHED),
      _subSteps(0),
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

void RotaryEncoder::begin(uint8_t clk_pin, uint8_t dt_pin, uint8_t sw_pin, bool enabled) {
    _clkPin  = clk_pin;
    _dtPin   = dt_pin;
    _swPin   = sw_pin;
    _enabled = enabled;
    _subSteps = 0;
    _encoderDelta = 0;

    if (!_enabled) {
        Serial.println("[ENCODER] Modul fisik di-NONAKTIFKAN (menunggu modul tiba). Menggunakan Kontrol Virtual Web.");
        return;
    }

    // Use internal pull-ups
    pinMode(_clkPin, INPUT_PULLUP);
    pinMode(_dtPin,  INPUT_PULLUP);
    pinMode(_swPin,  INPUT_PULLUP);

    uint8_t msb = digitalRead(_clkPin);
    uint8_t lsb = digitalRead(_dtPin);
    _lastEncoded = (msb << 1) | lsb;

    _lastBtnReading = digitalRead(_swPin);
    _btnState = (_lastBtnReading == LOW);
    Serial.printf("[ENCODER] Modul fisik AKTIF (CLK=%d, DT=%d, SW=%d)\n", _clkPin, _dtPin, _swPin);
}

void RotaryEncoder::handleEncoderIsr() {
    // Kept for backward compatibility if needed
}

void RotaryEncoder::update() {
    // Jika rotary fisik belum dipasang / dinonaktifkan, JANGAN baca pin floating agar tidak memicu pulsa hantu/acak!
    if (!_enabled) return;

    // 1. Noise-rejecting Gray Code state machine (Buxton Algorithm)
    uint8_t msb = digitalRead(_clkPin);
    uint8_t lsb = digitalRead(_dtPin);
    uint8_t encoded = (msb << 1) | lsb;

    if (encoded != _lastEncoded) {
        // 16-state valid transition table: 0 = illegal/noise jump, +1 = CW, -1 = CCW
        static const int8_t ENC_TABLE[16] = {
             0, -1, +1,  0,
            +1,  0,  0, -1,
            -1,  0,  0, +1,
             0, +1, -1,  0
        };
        uint8_t idx = (_lastEncoded << 2) | encoded;
        int8_t step = ENC_TABLE[idx];
        if (step != 0) {
            _subSteps += step;
            // Detent resting position pada EC11 adalah kedua pin HIGH (0b11)
            if (encoded == 0b11) {
                if (_subSteps >= 2) {
                    _encoderDelta++;
                } else if (_subSteps <= -2) {
                    _encoderDelta--;
                }
                _subSteps = 0;
            }
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

    if ((now - _lastDebounceTime) > 40) { // 40ms solid debounce
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
            if (!_longPressDispatched && (now - _btnPressStartTime >= 40)) {
                _pendingButtonEvent = BTN_CLICKED;
            }
        }
    }
}

int32_t RotaryEncoder::getDelta() {
    int32_t val = _encoderDelta;
    _encoderDelta = 0;
    return val;
}

EncoderButtonEvent RotaryEncoder::getButtonEvent() {
    EncoderButtonEvent ev = _pendingButtonEvent;
    _pendingButtonEvent = BTN_NONE;
    return ev;
}
