#include "timekeeping.h"

// Initialize static member
Timekeeping::Time Timekeeping::currentTime_ = {0, 0};

void Timekeeping::init(uint8_t initialHour, uint8_t initialMinute) {
    currentTime_.hours = initialHour % 12; // Ensure 0-11 for 12-hour system
    if (currentTime_.hours == 0) currentTime_.hours = 12; // Adjust 0 to 12 for display
    currentTime_.minutes = initialMinute % 60;
}

Timekeeping::Time Timekeeping::getCurrentTime() {
    return currentTime_;
}

void Timekeeping::incrementMinute() {
    currentTime_.minutes++;
    if (currentTime_.minutes >= 60) {
        currentTime_.minutes = 0;
        currentTime_.hours++;
        if (currentTime_.hours > 12) { // 12-hour system
            currentTime_.hours = 1;
        }
    }
}
