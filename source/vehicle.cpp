#include "../headers/vehicle.h"
#include "../headers/components.h"

EV::EV(){
    wheelRadius = 0.5;
    this->on = true;
}

//constructor that checks if user chose their own parameters or if they chose default value
EV::EV(float wheelRadius){
    if(wheelRadius == -1){
        this->wheelRadius = 0.5;
    } else {
        this->wheelRadius = wheelRadius;
    }
    this->on = true;
}

float EV::get_wheelRadius(){
    return wheelRadius;
}

EV::EV(const EV& other) : wheelRadius(other.wheelRadius) {
    on = true;
}

void EV::update(float speed, float delta_t) {
    battery->discharge(speed, delta_t);
}

EV& EV::operator=(const EV& other){
    if (this != &other){
    wheelRadius = other.wheelRadius;
    this->on = other.on;
    }
    return *this;
}

//setters
//these could be more useful in the future, but right now we decided not to use them
void EV::powerOn() { on = true; }
void EV::powerOff() { on = false; }
void EV::setMass(float m) { mass = m; }
void EV::setWheelRadius(float r) { wheelRadius = r; }
void EV::setDragCoefficient(float c) { dragCoefficient = c; }
void EV::setFrontalArea(float a) { frontalArea = a; }


bool EV::getOn() { return on; }