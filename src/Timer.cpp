#include "Timer.h"
#include <Arduino.h>

void Timer::startTimer(int msec) {
            timerStart = millis();
            timerTarget = msec;
        }
bool Timer::isTimerReady() {
            return (millis() - timerStart) > timerTarget;
        }
void LedFromColor::Initialisation(){
            pinMode(LedPIN, OUTPUT);
        }

void LedFromColor::Configure(short int PIN, short int Broche){
    LedPIN = PIN;
    BrocheColor = Broche;
}

short int LedFromColor::GetPINLed(){
    return BrocheColor;
}

