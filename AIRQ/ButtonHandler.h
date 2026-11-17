#ifndef BUTTONHANDLER_H
#define BUTTONHANDLER_H

#include <Arduino.h>

class ButtonHandler {
public:
    enum Event { NONE, SHORT_PRESS, DOUBLE_CLICK, LONG_PRESS_2S, VERY_LONG_PRESS_4S };

private:
    uint8_t pin;
    bool buttonState = false;
    bool lastFlickerableState = false;

    unsigned long pressedTime = 0;
    unsigned long lastClickTime = 0;
    unsigned long lastDebounceTime = 0;

    bool waitingForDoubleClick = false;
    bool handled4s = false;

    static const unsigned long DEBOUNCE_TIME = 50;
    static const unsigned long DOUBLE_CLICK_GAP = 250;
    static const unsigned long LONG_PRESS_2S_TIME = 2000;
    static const unsigned long VERY_LONG_PRESS_4S_TIME = 4000;

public:
    ButtonHandler(uint8_t buttonPin) : pin(buttonPin) {}

    void begin() { pinMode(pin, INPUT_PULLUP); }

    Event update() {
        Event result = NONE;
        bool reading = (digitalRead(pin) == LOW);
        unsigned long now = millis();

        if (reading != lastFlickerableState) {
            lastDebounceTime = now;
            lastFlickerableState = reading;
        }

        if ((now - lastDebounceTime) > DEBOUNCE_TIME) {
            if (reading != buttonState) {
                buttonState = reading;
                if (buttonState) {
                    pressedTime = now;
                    handled4s = false;
                } else {
                    unsigned long duration = now - pressedTime;
                    if (!handled4s && duration >= LONG_PRESS_2S_TIME) {
                        result = LONG_PRESS_2S;
                        waitingForDoubleClick = false;
                    } else if (!handled4s && duration < LONG_PRESS_2S_TIME) {
                        if (waitingForDoubleClick && (now - lastClickTime <= DOUBLE_CLICK_GAP)) {
                            result = DOUBLE_CLICK;
                            waitingForDoubleClick = false;
                        } else {
                            waitingForDoubleClick = true;
                            lastClickTime = now;
                        }
                    }
                }
            } else if (buttonState) {
                // Se mantiene presionado: Disparar eventos en tiempo real
                unsigned long duration = now - pressedTime;
                // Selección de 2 s al soltar: mantener 4 s no selecciona primero una red.
                if (!handled4s && duration >= VERY_LONG_PRESS_4S_TIME) {
                    handled4s = true;
                    result = VERY_LONG_PRESS_4S;
                    waitingForDoubleClick = false;
                }
            }
        }

        if (waitingForDoubleClick && !buttonState && (now - lastClickTime > DOUBLE_CLICK_GAP)) {
            waitingForDoubleClick = false;
            result = SHORT_PRESS;
        }

        return result;
    }
};

extern ButtonHandler buttonHandler;
#endif
