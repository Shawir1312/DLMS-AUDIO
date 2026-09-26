#pragma once
#include <Arduino.h>
#include "config.h"

enum EncoderButtonEvent {
    BTN_NONE = 0,
    BTN_CLICKED,
    BTN_LONG_PRESSED
};

class RotaryEncoder {
public:
    RotaryEncoder();

    void begin(uint8_t clk_pin = ENCODER_CLK_PIN, 
               uint8_t dt_pin = ENCODER_DT_PIN, 
               uint8_t sw_pin = ENCODER_SW_PIN,
               bool enabled = ENCODER_PHYSICAL_ATTACHED);

    // Enable or disable physical encoder polling (stops floating pin noise)
    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }

    // Call inside main loop or periodic task
    void update();

    // Returns rotation delta since last call (+1, -1, +2, -2, etc., or 0)
    int32_t getDelta();

    // Returns button event and clears it
    EncoderButtonEvent getButtonEvent();

    // Direct state check
    bool isButtonPressed() const { return _btnState; }

    // ISR handler (called from static interrupt)
    void handleEncoderIsr();

private:
    uint8_t _clkPin;
    uint8_t _dtPin;
    uint8_t _swPin;
    bool    _enabled;
    int8_t  _subSteps;

    volatile int32_t _encoderDelta;
    volatile uint8_t _lastEncoded;

    bool _lastBtnReading;
    bool _btnState;
    unsigned long _lastDebounceTime;
    unsigned long _btnPressStartTime;
    bool _longPressDispatched;
    EncoderButtonEvent _pendingButtonEvent;
};

extern RotaryEncoder rotaryEncoder;
