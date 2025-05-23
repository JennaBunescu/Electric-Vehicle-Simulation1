#include "../headers/driver_input.h"

//Constructor (Initialize throttle and brake positions to zero (no input))
DriverInput::DriverInput() {
    throttlePosition = 0;
    brakePosition = 0;
}

//Getters
float DriverInput::get_throttle() {
    return throttlePosition;
}

float DriverInput::get_brake() {
    return brakePosition;
}

//Setter for throttle position with input validation
void DriverInput::set_throttle(float intensity) {
    if (intensity > 1) {
        throttlePosition = 1; //Cap maximum throttle at 1
    } else if (intensity < 0) {
        throttlePosition = 0; //Minimum throttle at 0
    } else {
        throttlePosition = intensity; 
    }
}

//Setter for brake position
void DriverInput::set_brake(float intensity) {
    if (intensity > 1) {
        brakePosition = 1;//Cap maximum brake at 1
    } else if (intensity < 0) {
        brakePosition = 0; //Minimum brake at 0
    } else {
        brakePosition = intensity;
    }
}
