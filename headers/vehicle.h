#ifndef VEHICLE_H
#define VEHICLE_H
#include "../headers/components.h"
#include "../headers/driver_input.h"
#include <iostream>
using namespace std;

class EV{
    private:
        bool on;
        float mass;
        float dragCoefficient;
        float frontalArea;
        Battery* battery;
        Motor* motor;
        bool obstacle;
        float wheelRadius; //Radius of the vehicle's wheels (meters)

    public:
        EV();
        EV(float wheelRadius);

        void update(float speed, float delta_t);
        EV(const EV& other);

        EV& operator=(const EV& other);
        float get_wheelRadius();

        void powerOn();
        void powerOff();
        bool getOn();
        void setMass(float m);
        void setWheelRadius(float r);
        void setDragCoefficient(float c);
        void setFrontalArea(float a);
    };



#endif